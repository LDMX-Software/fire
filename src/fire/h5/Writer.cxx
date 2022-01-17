#include "fire/h5/Writer.h"
#include "fire/h5/Reader.h"

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
  /**
   * When chunking, HDF5 requires all chunks to be the same size in order
   * to keep the I/O operations efficient. We would like to have our chunks
   * be O(10KB) => rows_per_chunk = O(10k); however, when the number of events
   * is small, it is very likely that the datasets will not achieve the size
   * of even a single chunk. For this reason, we take the minimum of the input
   * rows_per_chunk_ and the event_limit. We do NOT redefine rows_per_chunk_,
   * so that this decision does not affect the size of the buffer.
   */
  create_props_.add(HighFive::Chunking({std::min(rows_per_chunk_,entries_)}));
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
  std::string full_path = Reader::EVENT_GROUP+"/"+full_obj_name;
  // if full_path goes to a dataset
  if (file_.getObjectType(full_path) == HighFive::ObjectType::Dataset) {
    file_.getDataSet(full_path).createAttribute(h5::Reader::TYPE_ATTR_NAME, type);
  } else {
    file_.getGroup(full_path).createAttribute(h5::Reader::TYPE_ATTR_NAME, type);
  }
}

}  // namespace fire::h5
