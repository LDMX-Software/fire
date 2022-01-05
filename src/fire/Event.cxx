#include "fire/Event.h"

namespace fire {

std::vector<ProductTag> Event::search(const std::string& namematch,
                                      const std::string& passmatch,
                                      const std::string& typematch) const {
  std::regex name_reg{namematch.empty() ? ".*" : namematch,
                      std::regex::extended | std::regex::nosubs};
  std::regex pass_reg{passmatch.empty() ? ".*" : passmatch,
                      std::regex::extended | std::regex::nosubs};
  std::regex type_reg{typematch.empty() ? ".*" : typematch,
                      std::regex::extended | std::regex::nosubs};
  std::vector<ProductTag> matches;
  std::copy_if(products_.begin(), products_.end(), std::back_inserter(matches),
               [&](const ProductTag& pt) {
                 return pt.match(name_reg, pass_reg, type_reg);
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

Event::Event(const std::string& pass,
             const std::vector<config::Parameters>& dk_rules)
    : pass_{pass},
      input_file_{nullptr},
      i_entry_{0},
      header_{std::make_unique<EventHeader>()} {
  /// register our event header with a data set for save/load
  //    we own the pointer in this special case so we can return both mutable
  //    and const references
  auto& obj{objects_[EventHeader::NAME]};
  obj.set_ = std::make_unique<h5::DataSet<EventHeader>>(
        EventHeader::NAME, header_.get()),
  obj.should_save_ = true;  // always save event header
  obj.should_load_ = false;  // don't load unless input files are passed
  // construct rules from rule configuration parameters
  //   TODO check for regex construction failures
  for (const auto& rule : dk_rules) {
    drop_keep_rules_.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(
            rule.get<std::string>("regex"),
            std::regex::extended | std::regex::icase | std::regex::nosubs),
        std::forward_as_tuple(rule.get<bool>("keep")));
  }
}

void Event::save(h5::Writer& w, unsigned long int i) {
  for (auto& [_, obj] : objects_)
    if (obj.should_save_) obj.set_->save(w, i);
}

void Event::load(h5::Reader& r, unsigned long int i) {
  for (auto& [_, obj] : objects_)
    if (obj.should_load_) obj.set_->load(r, i);
}

void Event::setInputFile(h5::Reader& r) {
  input_file_ = &r;

  // there are input file, so mark the event header as should_load
  objects_[EventHeader::NAME].should_load_ = true;

  // TODO search through file and import the products that are there
  // products_.clear();
}

void Event::next() {
  i_entry_++;
  for (auto& [_, obj] : objects_) obj.set_->clear();
}

}  // namespace fire
