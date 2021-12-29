#include "fire/h5/Writer.h"

namespace fire::h5 {

Writer::Writer(const int& event_limit, const config::Parameters& ps)
    : file_(ps.get<std::string>("name"),
            HighFive::File::Create | HighFive::File::Truncate),
      rows_per_chunk_{ps.get<int>("rows_per_chunk")},
      compression_level_{ps.get<int>("compression_level")},
      shuffle_{ps.get<bool>("shuffle")},
      entries_{event_limit} {}

Writer::~Writer() {
  file_.flush();
}

const std::string& Writer::name() const {
  return file_.getName();
}

}  // namespace fire::h5
