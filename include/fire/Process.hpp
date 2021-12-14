#ifndef FIRE_PROCESS_HPP
#define FIRE_PROCESS_HPP

#include <exception>

#include "fire/Event.hpp"

namespace fire {

class Process {
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

 private:
  /// processing pass name
  std::string pass_;

  /// limit on number of events to process
  int event_limit_;

  /// frequency with which event info is printed
  int log_frequency_;

  /// integer form of logging level to print to terminal
  int term_level_;

  /// integer form of logging level to print to file
  int file_level_;

  /// name of file to print logging to
  std::string log_file_;

  /// number of attempts to make before giving up on an event
  int max_tries_;

  /// run number to use if generating events
  int run_;

  /// input file listing, maybe empty
  std::vector<h5::File> input_files_;

  /// output file we are writing to
  h5::File output_file_;

  /// object used to determine if an event should be saved or not
  StorageController storage_controller_;

  /// handle to conditions system
  Conditions conditions_;

  /// current entry index
  long unsigned int i_entry_;

  /// number of entries we have
  long unsigned int entries_;

  /// event object
  Event event_;
};  // Process

}  // namespace fire

#endif  // FIRE_PROCESS_HPP
