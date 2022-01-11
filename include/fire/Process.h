#ifndef FIRE_PROCESS_H
#define FIRE_PROCESS_H

#include "fire/logging/Logger.h"
#include "fire/StorageControl.h"
#include "fire/Conditions.h"
#include "fire/Event.h"
#include "fire/Processor.h"
#include "fire/RunHeader.h"

namespace fire {

class Process {
 public:
  Process(const config::Parameters& configuration);
  void run(); 
  ~Process();

  const EventHeader& eventHeader() const {
    return event_.header();
  }

  EventHeader& eventHeader() {
    return event_.header();
  }

  const RunHeader& runHeader() const {
    assert(run_header_);
    return *run_header_;
  }

  RunHeader& runHeader() {
    assert(run_header_);
    return *run_header_;
  }

  void addStorageControlHint(
      StorageControl::Hint hint,
      const std::string& purpose,
      const std::string& processor
      ) {
    storage_control_.addHint(hint,purpose,processor);
  }

  Conditions& conditions() {
    return conditions_;
  }

 private:
  /// initialize a new run during the process
  void newRun(RunHeader& rh);

  /**
   * process the event
   *
   * If the event is not aborted, the storage controller is given the choice
   * on whether the event should be kept. If it decides to "keep" the event,
   * then the event is saved to the output file 
   */
  bool process(const std::size_t& n_events_processed);

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

  /// the sequence of processors to run
  std::vector<std::unique_ptr<Processor>> sequence_;

  /// object used to determine if an event should be saved or not
  StorageControl storage_control_;

  /// handle to conditions system
  Conditions conditions_;

  /// event object
  Event event_;

  /// the current run header
  RunHeader* run_header_;

  /// log through the 'Process' channel
  ENABLE_LOGGING(Process);
};  // Process

}  // namespace fire

#endif  // FIRE_PROCESS_H
