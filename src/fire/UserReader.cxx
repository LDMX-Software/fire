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

UserReader::UserReader() 
  : event_{nullptr,"readonly",drop_all()}, i_entry_{0} {}

void UserReader::open(const std::string& fn, unsigned long int n) {
  i_entry_ = 0;
  reader_ = io::open(fn);
  event_.setInputFile(reader_.get());
  for (int i{0}; i < n; i++) next();
}

void UserReader::next() {
  // within range of file, just load next entry
  if (i_entry_+1 < reader_->entries()) {
    i_entry_++;
    event_.load();
    return;
  }

  // going to reach number of entries in file, what do?
  if (wrap_around_) {
    // re-initialize
    open(reader_->name(), 0);
  } else {
    throw Exception("EndOfFile",
        "File '"+reader_->name()+"' has no more entires.");
  }
}

unsigned long int UserReader::entries() const {
  return reader_ ? reader_->entries() : 0;
}

bool UserReader::is_open() const {
  return bool(reader_);
}

}
