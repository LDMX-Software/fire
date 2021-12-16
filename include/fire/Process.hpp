#ifndef FIRE_PROCESS_HPP
#define FIRE_PROCESS_HPP


#include "fire/Exception/Exception.h"
#include "fire/Event.hpp"

namespace fire {

class Process {
 public:
  Process(const config::Parameters& configuration);
  void run(); 
  ~Process() {
    logging::close();
  }

  const EventHeader& eventHeader() const {
    return event_.header();
  }

  EventHeader& eventHeader() {
    return event_.header();
  }

  const RunHeader& runHeader() const {
    return *run_header_;
  }

  RunHeader& runHeader() {
    return *run_header_;
  }

 private:
  /// initialize a new run during the process
  void newRun(RunHeader& rh);

  /**
   * process the event
   *
   * If the event is not aborted, the storage controller is given the choice
   * on whether the event should be kept. If it decides to "keep" the event,
   * then the event is saved to the output file and the i_output_file index
   * is incremented by one.
   */
  bool process(const std::size_t& n_events_processed, std::size_t& i_output_file);

 private:
  /// limit on number of events to process
  int event_limit_;

  /// frequency with which event info is printed
  int log_frequency_;

  /// number of attempts to make before giving up on an event PRODUCTION MODE
  int max_tries_;

  /// run number to use PRODUCTION MODE
  int run_;

  /// input file listing, PRODUCTION MODE if empty
  std::vector<std::string> input_files_;

  /// output file we are writing to
  h5::Writer output_file_;

  /// object used to determine if an event should be saved or not
  StorageController storage_controller_;

  /// handle to conditions system
  Conditions conditions_;

  /// event object
  Event event_;

  /// the current run header
  RunHeader* run_header_;
};  // Process

}  // namespace fire

#endif  // FIRE_PROCESS_HPP
