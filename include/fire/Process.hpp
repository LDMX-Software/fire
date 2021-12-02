#ifndef FIRE_PROCESS_HPP
#define FIRE_PROCESS_HPP

#include <exception>

#include "fire/Event.hpp"

namespace fire {

class Process {
  h5::File h5_file_;
  bool write_;
  long unsigned int i_entry_;
  long unsigned int entries_;
  Event event_;

 public:
  Process(const std::string& pass, const std::string& file_name, bool write = false)
      : h5_file_{file_name, write},
        write_{write},
        event_{pass},
        i_entry_{0},
        entries_{0} {
    if (not write) {
      // need to grab number of entries from specific data set
      event_.setInputFile(h5_file_);
      entries_ = H5Easy::getSize(h5_file_, "i_entry");
    }
  }

  /// stand-in for processors 
  Event& event() { return event_; }

  bool next(bool should_save = true) {
    i_entry_++;
    if (write_) {
      if (should_save) {
        event_.save(h5_file_,i_entry_-1);
      }
      entries_++;
      return true;
    } else {
      if (i_entry_ < entries_) {
        event_.load(h5_file_,i_entry_);
        event_.next();
        return true;
      } else
        return false;
    }
  }
};  // Process

}  // namespace fire

#endif  // FIRE_PROCESS_HPP
