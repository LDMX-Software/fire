#include "fire/io/h5/Reader.h"

#include "fire/io/Constants.h"
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

std::string Reader::getTypeName(const std::string& full_obj_name) const {
  std::string path = constants::EVENT_GROUP + "/" + full_obj_name;
  HighFive::Attribute attr =
      file_.getObjectType(path) == HighFive::ObjectType::Dataset
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

static void recursive_list(const HighFive::File& file, const std::string& path, 
                           std::vector<HighFive::DataSet> ds = {}) {
  auto datasets{ds};
  if (file.getObjectType(path) == HighFive::ObjectType::Dataset) {
    datasets.push_back(file.getDataSet(path));
  } else {
    auto subobjs = file.getGroup(path).listObjectNames();
    for (auto& subobj : subobjs) recursive_list(file, path+"/"+subobj, datasets);
  }
  return datasets;
}


void Reader::copy(unsigned int long i_entry, const std::string& full_name, Writer& output) const {
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

  /// 1. Obtain Full List of DataSets
  std::vector<HighFive::DataSets> datasets = recursive_list(file_, full_name);


}

}  // namespace fire::io::h5

/// register this reader with the reader factory
namespace {
auto v = fire::io::Reader::Factory::get().declare<fire::io::h5::Reader>();
}
