#include "fire/Event.h"

namespace fire {

std::vector<Event::EventObjectTag> Event::search(const std::string& namematch,
                                      const std::string& passmatch,
                                      const std::string& typematch) const {
  std::regex name_reg{namematch.empty() ? ".*" : namematch,
                      std::regex::extended | std::regex::nosubs};
  std::regex pass_reg{passmatch.empty() ? ".*" : passmatch,
                      std::regex::extended | std::regex::nosubs};
  std::regex type_reg{typematch.empty() ? ".*" : typematch,
                      std::regex::extended | std::regex::nosubs};
  std::vector<EventObjectTag> matches;
  std::copy_if(available_objects_.begin(), available_objects_.end(), std::back_inserter(matches),
               [&](const EventObjectTag& tag) {
                 return tag.match(name_reg, pass_reg, type_reg);
               });
  return matches;
}

bool Event::keep(const std::string& full_name, bool def) const {
  // search through list BACKWARDS, meaning the last rule that applies will be
  // the decision
  auto rule_it{std::find_if(drop_keep_rules_.rbegin(), drop_keep_rules_.rend(),
                            [&](const auto& rule_pair) {
                              return std::regex_match(full_name,
                                                      rule_pair.first);
                            })};
  // no rules applied
  return rule_it == drop_keep_rules_.rend() ? def : rule_it->second;
}

Event::Event(io::Writer& output_file,
             const std::string& pass,
             const std::vector<config::Parameters>& dk_rules)
    : output_file_{output_file},
      pass_{pass},
      input_file_{nullptr},
      i_entry_{0},
      header_{std::make_unique<EventHeader>()} {
  /// register our event header with a data set for save/load
  //    we own the pointer in this special case so we can return both mutable
  //    and const references
  auto& obj{objects_[EventHeader::NAME]};
  obj.data_ = std::make_unique<io::Data<EventHeader>>(EventHeader::NAME,
                                                        header_.get()),
  obj.should_save_ = true;   // always save event header
  obj.should_load_ = false;  // don't load unless input files are passed
  obj.updated_ = false;      // not used for EventHeader
  // construct rules from rule configuration parameters
  for (const auto& rule : dk_rules) {
    auto regex{rule.get<std::string>("regex")};
    try {
      drop_keep_rules_.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(regex,
            std::regex::extended | std::regex::icase | std::regex::nosubs),
        std::forward_as_tuple(rule.get<bool>("keep")));
    } catch (const std::regex_error&) {
      throw Exception("Config",
          "Drop/Keep regex '"+regex+"' not a proper regex.",false);
    }
  }
}

void Event::save() {
  for (auto& [_, obj] : objects_)
    if (obj.should_save_) obj.data_->save(output_file_);

  for (const auto& tag : available_objects_) {
    if (tag.keep() and not tag.loaded()) {
      // need to copy this event object from the input file
      // into the output file because it is supposed to be kept
      // but hasn't been loaded by the user
      input_file_->copy(i_entry_, 
          io::constants::EVENT_GROUP+"/"+fullName(tag.name(), tag.pass()), 
          output_file_);
    }
  }
}

void Event::load() {
  assert(input_file_);
  for (auto& [_, obj] : objects_)
    if (obj.should_load_) input_file_->load_into(*obj.data_);
}

void Event::setInputFile(io::Reader* r) {
  static const bool READ_KEEP_DEFAULT = false;
  input_file_ = r;

  // there are input file, so mark the event header as should_load
  objects_[EventHeader::NAME].should_load_ = true;

  // search through file and import the available objects that are there
  available_objects_.clear();
  known_lookups_.clear();
  for (const auto& [name, pass] : input_file_->availableObjects()) {
    std::string full_name{fullName(name, pass)};
    auto [ type, vers ] = input_file_->type(full_name);
    available_objects_.emplace_back(
        name, pass, type, vers,
        keep(full_name, READ_KEEP_DEFAULT));
  }
}

void Event::next() {
  i_entry_++;
  for (auto& [_, obj] : objects_) obj.clear();
}

void Event::done() {
  // copy type traits into output as attributes
  for (const EventObjectTag& p : available_objects_) {
    auto fn = fullName(p.name(), p.pass());
    if (objects_.find(fn) != objects_.end() and objects_.at(fn).should_save_) {
      // this product has been written to output file so we tell it we are done
      objects_.at(fn).data_->done(output_file_);
    }
  }

  // persist event header structure
  objects_[EventHeader::NAME].data_->done(output_file_);

  // make sure writer is flushed
  output_file_.flush();
}

}  // namespace fire
