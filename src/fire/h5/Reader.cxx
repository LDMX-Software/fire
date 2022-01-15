#include "fire/h5/Reader.h"

namespace fire::h5 {

const std::string Reader::EVENT_HEADER_NAME = "EventHeader";
const std::string Reader::EVENT_HEADER_NUMBER = "number";
const std::string Reader::EVENT_GROUP = "events";
const std::string Reader::RUN_HEADER_NAME = "runs";
const std::string Reader::TYPE_ATTR_NAME = "type";

Reader::Reader(const std::string& name) : file_{name} {
  entries_ = file_.getDataSet(EVENT_GROUP + "/" 
      + EVENT_HEADER_NAME + "/" 
      + EVENT_HEADER_NUMBER)
                 .getDimensions()
                 .at(0);
}

const std::string& Reader::name() const { return file_.getName(); }

std::size_t Reader::runs() const {
  return file_.getDataSet(RUN_HEADER_NAME + "/number").getDimensions().at(0);
}

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
  std::string path = EVENT_GROUP + "/" + full_obj_name;
  HighFive::Attribute attr =
      file_.getObjectType(path) == HighFive::ObjectType::Dataset
          ? file_.getDataSet(path).getAttribute(TYPE_ATTR_NAME)
          : file_.getGroup(path).getAttribute(TYPE_ATTR_NAME);
  std::string type;
  attr.read(type);
  return type;
}

}  // namespace fire::h5
