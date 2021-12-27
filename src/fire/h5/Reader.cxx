#include "fire/h5/Reader.h"

namespace fire::h5 {

const std::string Reader::EVENT_HEADER_NAME = "EventHeader";
const std::string Reader::EVENT_GROUP = "events";
const std::string Reader::RUN_HEADER_NAME = "runs";

Reader::Reader(const std::string& name) : file_{name} {
  // TODO unify this with the event header constant
  entries_ =
      file_.getDataSet(EVENT_GROUP+"/"+EVENT_HEADER_NAME+"/number").getDimensions().at(0);
}

const std::string& Reader::name() const { return file_.getName(); }

std::size_t Reader::runs() const {
  return file_.getDataSet(RUN_HEADER_NAME+"/number").getDimensions().at(0);
}

std::vector<std::string> Reader::list(const std::string& group_path) const {
  // just return empty list of group does not exist
  if (not file_.exist(group_path)) return {};
  return file_.getGroup(group_path).listObjectNames();
}

HighFive::DataTypeClass Reader::getDataSetType(const std::string& dataset) const {
  return file_.getDataSet(dataset).getDataType().getClass();
}

}  // namespace fire::h5
