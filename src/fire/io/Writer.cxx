#include "fire/io/Writer.h"

#include "fire/io/Constants.h"

namespace fire::io {

Writer::Writer(const int& event_limit, const config::Parameters& ps)
    : create_props_{},
      space_(std::vector<std::size_t>({0}), 
          std::vector<std::size_t>({HighFive::DataSpace::UNLIMITED})) {
  auto filename{ps.get<std::string>("name")};
  if (filename.empty()) {
    throw fire::Exception("NoOutputFile",
        "No output file was provided to fire\n"
        "         Use `p.output_file = 'my-file.h5'` in your python config.",
        false);
  }
  file_ = std::make_unique<HighFive::File>(filename,
        HighFive::File::Create | HighFive::File::Truncate);
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
  file_->flush();
}

const std::string& Writer::name() const { return file_->getName(); }

void Writer::structure(const std::string& full_path, const std::string& type, int version) {
  auto grp = file_->createGroup(full_path);
  grp.createAttribute(constants::TYPE_ATTR_NAME, type);
  grp.createAttribute(constants::VERS_ATTR_NAME, version);
}

}  // namespace fire::h5
