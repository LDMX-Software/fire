#include "fire/io/h5/Reader.h"

#include "fire/io/Constants.h"
#include "fire/io/Data.h"

namespace fire::io::h5 {

Reader::Reader(const std::string& name) 
  : file_{name},
    entries_{file_.getDataSet(
        constants::EVENT_GROUP + "/" 
      + constants::EVENT_HEADER_NAME + "/" 
      + constants::NUMBER_NAME).getDimensions().at(0)},
    runs_{file_.getDataSet(
        constants::RUN_HEADER_NAME+"/"+
        constants::NUMBER_NAME)
      .getDimensions().at(0)},
   ::fire::io::Reader(name) {}

void Reader::load_into(BaseData& d) {
  d.load(*this);
}

std::string Reader::name() const { return file_.getName(); }

std::vector<std::string> Reader::list(const std::string& group_path) const {
  // just return empty list of group does not exist
  if (not file_.exist(group_path)) return {};
  return file_.getGroup(group_path).listObjectNames();
}

HighFive::DataTypeClass Reader::getDataSetType(
    const std::string& dataset) const {
  return file_.getDataSet(dataset).getDataType().getClass();
}

HighFive::ObjectType Reader::getH5ObjectType(const std::string& path) const {
  return file_.getObjectType(path);
}

std::string Reader::getTypeName(const std::string& full_obj_name) const {
  std::string path = constants::EVENT_GROUP + "/" + full_obj_name;
  HighFive::Attribute attr = 
    getH5ObjectType(path) == HighFive::ObjectType::Dataset
          ? file_.getDataSet(path).getAttribute(constants::TYPE_ATTR_NAME)
          : file_.getGroup(path).getAttribute(constants::TYPE_ATTR_NAME);
  std::string type;
  attr.read(type);
  return type;
}

std::vector<std::array<std::string,3>> Reader::availableObjects() {
  std::vector<std::array<std::string,3>> objs;
  std::vector<std::string> passes = list(io::constants::EVENT_GROUP);
  for (const std::string& pass : passes) {
    // skip the event header
    if (pass == io::constants::EVENT_HEADER_NAME) continue;
    // get a list of objects in this pass group
    std::vector<std::string> object_names = list(io::constants::EVENT_GROUP + "/" + pass);
    for (const std::string& obj_name : object_names) {
      std::array<std::string,3> obj = {obj_name, pass,
          getTypeName(pass+"/"+obj_name) };
      objs.push_back(obj);
    }
  }
  return objs;
}

void Reader::copy(unsigned int long i_entry, const std::string& full_name, Writer& output) {
  /**
   * STRATEGY:
   *
   * ## Initialization
   * 1. Recurse into the 'full_name' event object obtaining a full list of all DataSets
   * 2. While doing this, categorize the DataSets such that we can deduce if there are
   *    "commanders" (i.e. DataSets named `size`) who will control the number of entries
   *    read from other DataSets.
   * 3. Set-up these atomic data sets in a structure that mimics the on-disk structure in-memory.
   *
   * ## Every Copy
   * Connect the Reader::load handles immediately to the Writer::save call.
   */

  // this is where recursing into the subgroups of full_name occurs
  // if this mirror object hasn't been created yet
  if (mirror_objects_.find(full_name) == mirror_objects_.end()) {
    std::string path = constants::EVENT_GROUP + "/" + full_name;
    mirror_objects_.emplace(std::make_pair(full_name,
          std::make_unique<MirrorObject>(path, *this)));
  }
  // do the copying
  mirror_objects_[full_name]->copy(i_entry, 1, output);
}

Reader::MirrorObject::MirrorObject(const std::string& path, Reader& reader) 
  : reader_{reader} {
  if (reader_.getH5ObjectType(path) == HighFive::ObjectType::Dataset) {
    // simple atomic event object
    auto type{reader_.getDataSetType(path)};
    if (type == HighFive::DataTypeClass::Integer) {
      data_ = std::make_unique<io::Data<int>>(path);
    } else if (type == HighFive::DataTypeClass::Float) {
      data_ = std::make_unique<io::Data<float>>(path);
    } else {
      data_ = std::make_unique<io::Data<std::string>>(path);
    }
    // TODO make sure this is full
    //  - bool <-> Bool
    //  - size of ints/floats?
  } else {
    // event object is a H5 group meaning it is more complicated
    // than a simple atomic type
    auto subobjs = reader_.list(path);
    for (auto& subobj : subobjs) {
      std::string sub_path{path + "/" + subobj};
      if (subobj == "size") {
        size_member_ = std::make_unique<io::Data<std::size_t>>(sub_path);
      } else {
        obj_members_.emplace_back(std::make_unique<MirrorObject>(sub_path, reader_));
      }
    }
  }
}

void Reader::MirrorObject::copy(unsigned long int i_entry, unsigned long int n, Writer& output) {
  unsigned long int num_to_advance{i_entry <= last_entry_ ? 0 : i_entry - last_entry_ - 1}, 
                    num_to_save{n};
  last_entry_ = i_entry;

  // if we have a data member, the data member is the only part of this
  // mirror object
  if (data_) {
    // load until one before desired entry
    for (std::size_t i{0}; i < num_to_advance; i++) data_->load(reader_);
    // load and save desired entries
    for (std::size_t i{0}; i < num_to_save; i++) {
      data_->load(reader_);
      data_->save(output);
    }
    return;
  }

  /// if there is a member determining the size of each entry,
  /// we need to follow its lead
  if (size_member_) {
    unsigned long int new_num_to_advance{0};
    for (std::size_t i{0}; i < num_to_advance; i++) {
      size_member_->load(reader_);
      num_to_advance += dynamic_cast<Data<std::size_t>&>(*size_member_).get();
    }
    unsigned long int new_num_to_save = 0;
    for (std::size_t i{0}; i < num_to_save; i++) {
      size_member_->load(reader_);
      num_to_save += dynamic_cast<Data<std::size_t>&>(*size_member_).get();
      size_member_->save(output);
    }

    num_to_advance = new_num_to_advance;
    num_to_save = new_num_to_save;
  }

  for (auto& obj  : obj_members_) obj->copy(num_to_advance, num_to_save, output);
}

}  // namespace fire::io::h5

/// register this reader with the reader factory
namespace {
auto v = fire::io::Reader::Factory::get().declare<fire::io::h5::Reader>();
}
