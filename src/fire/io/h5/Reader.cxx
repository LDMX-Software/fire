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
      .getDimensions().at(0)} {}

const std::string& Reader::name() const { return file_.getName(); }

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

}  // namespace fire::h5
