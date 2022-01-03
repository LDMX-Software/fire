#include "fire/h5/Writer.h"

namespace fire::h5 {

Writer::Writer(const int& event_limit, const config::Parameters& ps)
    : file_(ps.get<std::string>("name"),
            HighFive::File::Create | HighFive::File::Truncate),
      shuffle_{ps.get<bool>("shuffle")},
      compression_level_{ps.get<int>("compression_level")} {
  entries_ = event_limit;
  rows_per_chunk_ = ps.get<int>("rows_per_chunk");
}

Writer::~Writer() { this->flush(); }

void Writer::flush() {
  for (auto& [path, buff] : buffers_) {
    buff->flush(file_,path);
  }
  file_.flush();
}

const std::string& Writer::name() const { return file_.getName(); }

}  // namespace fire::h5
