#include "fire/h5/Writer.h"

#include "fire/h5/Constants.h"

namespace fire::h5 {

Writer::Writer(const int& event_limit, const config::Parameters& ps)
    : file_(ps.get<std::string>("name"),
            HighFive::File::Create | HighFive::File::Truncate),
      create_props_{},
      space_(std::vector<std::size_t>({0}), 
          std::vector<std::size_t>({HighFive::DataSpace::UNLIMITED})) {
  // down here with = to allow implicit cast from 'int' to 'std::size_t'
  entries_ = event_limit;
  rows_per_chunk_ = ps.get<int>("rows_per_chunk");
  // copy creation properties into HighFive structure
  create_props_.add(HighFive::Chunking({rows_per_chunk_}));
  if (ps.get<bool>("shuffle")) create_props_.add(HighFive::Shuffle());
  create_props_.add(HighFive::Deflate(ps.get<int>("compression_level")));
}

Writer::~Writer() { this->flush(); }

void Writer::flush() {
  for (auto& [path, buff] : buffers_) {
    buff->flush();
  }
  file_.flush();
}

const std::string& Writer::name() const { return file_.getName(); }

void Writer::setTypeName(const std::string& full_obj_name, const std::string& type) { 
  std::string full_path = constants::EVENT_GROUP+"/"+full_obj_name;
  // if full_path goes to a dataset
  if (file_.getObjectType(full_path) == HighFive::ObjectType::Dataset) {
    file_.getDataSet(full_path).createAttribute(constants::TYPE_ATTR_NAME, type);
  } else {
    file_.getGroup(full_path).createAttribute(constants::TYPE_ATTR_NAME, type);
  }
}

}  // namespace fire::h5
