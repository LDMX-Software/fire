#include "fire/h5/Writer.hpp"

namespace fire::h5 {

Writer::Writer(const config::Parameters& ps)
    : file_(ps.get<std::string>("name"),
            HighFive::File::Create | HighFive::File::Truncate),
      rows_per_chunk_{ps.get<int>("rows_per_chunk")} {
  entries_ = ps.get<int>("event_limit");
}

Writer::~Writer() {
  file_.flush();
}

const std::string& Writer::name() const {
  return file_.getName();
}

}  // namespace fire::h5
