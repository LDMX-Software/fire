#include "fire/UserReader.h"
#include "fire/io/Open.h"

namespace fire {

/**
 * Drop keep rule list that drops all events
 *
 * we need this for our event bus so we don't accidentally
 * attempt to write to a NULL writer
 */
static std::vector<config::Parameters> drop_all() {
  std::vector<config::Parameters> da(1);
  da[0].add<std::string>("regex",".*");
  da[0].add("keep",false);
  return da;
}

UserReader::UserReader(bool wrap_around) 
  : event_{nullptr,"readonly",drop_all()}, 
    i_entry_{0}, in_file_{false},
    wrap_around_{wrap_around} {}

UserReader::UserReader(const std::string& fn, unsigned long int n, bool wrap_around)
  : UserReader(wrap_around) {
    open(fn, n);
  }

void UserReader::open(const std::string& fn, unsigned long int n) {
  i_entry_ = 0;
  in_file_ = false;
  reader_ = io::open(fn);
  event_.setInputFile(reader_.get());
  for (int i{0}; i < n; i++) next();
}

bool UserReader::next() {
  if (not reader_) {
    throw fire::Exception("BadConf",
        "Cannot go to next entry in a file when the reader has not opened a file!",
        false);
  }

  // within range of file, just load next entry
  if (i_entry_ < reader_->entries()) {
    if (in_file_) event_.next();
    i_entry_++;
    event_.load();
    in_file_ = true;
    return true;
  } else if (wrap_around_) {
    // re-initialize
    open(reader_->name(), 0);
    return next();
  } else {
    return false;
  }
}

unsigned long int UserReader::entries() const {
  return reader_ ? reader_->entries() : 0;
}

bool UserReader::is_open() const {
  return bool(reader_);
}

}
