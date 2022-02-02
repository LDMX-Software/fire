#ifndef FIRE_PROCESS_H
#define FIRE_PROCESS_H

#include "fire/logging/Logger.h"
#include "fire/StorageControl.h"
#include "fire/Conditions.h"
#include "fire/Event.h"
#include "fire/Processor.h"
#include "fire/RunHeader.h"

namespace fire {

/**
 * The central object managing the data processing
 */
class Process {
 public:
  /**
   * Construct the process by configuring the necessary objects
   *
   * The order with which we configure the objects is _very_ important.
   * Some objects require references to other objects (for example,
   * Event depends on an h5::Writer), so the order the objects are
   * declared and therefore the order in which they are constructed
   * should be modified with care.
   *
   * Currently, the order of construction is given below.
   * I skip listing any types provided by STL.
   * - The output file as a h5::Writer
   * - the Event bus
   * - StorageControl system
   *
   * After these initial constructions, the logging is opened.
   *
   * @note The user conditions providers are constructed _before_
   *    logging is open. This will cause some funky behavior is the
   *    user attempts to log messages in their conditions provider
   *    constructors.
   *
   * After the logging is opened, we go through the registered libraries
   * and use factory::loadLibrary to load them dynamically.
   * These libraries provide the complete list of registered conditions
   * providers and processors, so we then construct the conditions system
   * and the sequence of processors (in order).
   *
   * @throws Exception if there is no sequence and the configuration
   *  does not have a parameter named 'testing' set to true.
   *
   * @see factory::loadLibrary for how libraries are dynamically loaded
   * @see Conditions::Conditions for how providers are created
   * @see factory::Factory::make for how processors and providers are made
   * @see logging::open for how the logging is initialized
   *
   * @param[in] configuration complete processing configuration
   */
  Process(const config::Parameters& configuration);
  
  /**
   * close logging before we are done
   *
   * This closes up the logging that we opened in the constructor.
   */
  ~Process();
  
  /**
   * Do the processing run
   */
  void run(); 

  /**
   * Get a constant reference to the event header
   * @return const reference to event header
   */
  const EventHeader& eventHeader() const {
    return event_.header();
  }

  /**
   * Get a non-constant reference to the event header
   * @return reference to event header
   */
  EventHeader& eventHeader() {
    return event_.header();
  }

  /**
   * Get a const reference to the run header
   * @note We use assert to make sure the run header
   * is defined before de-referencing the pointer we have.
   * @return const reference to run header
   */
  const RunHeader& runHeader() const {
    assert(run_header_);
    return *run_header_;
  }

  /**
   * Get a non-const reference to the run header
   * @note We use assert to make sure the run header
   * is defined before de-referencing the pointer we have.
   * @return reference to run header
   */
  RunHeader& runHeader() {
    assert(run_header_);
    return *run_header_;
  }

  /**
   * Add a storage control hint to the StorageControl system
   *
   * @see StorageControl::addHint for how these hints are used
   * @see Processor::setStorageHint for how user Processors are
   *    supposed to call this function.
   *
   * @param[in] hint hint to storage on what to do with this event
   * @param[in] purpose reason for this hint
   * @param[in] processor processor from which this hint came from
   */
  void addStorageControlHint(
      StorageControl::Hint hint,
      const std::string& purpose,
      const std::string& processor
      ) {
    storage_control_.addHint(hint,purpose,processor);
  }

  /**
   * Get a reference to the current conditions system.
   *
   * Since the conditions system is constructed in the Process constructor,
   * we don't use any assertions to check the validty of that pointer.
   *
   * @return reference to conditions system
   */
  Conditions& conditions() {
    return *conditions_;
  }

 private:
  /**
   * Method to declare a new run is beginning
   *
   * We update the pointer to the run header to use the passed reference.
   * This is so asynchronous callers can access the run header via Process.
   *
   * Then we call the following user call backs in order.
   * 1. Processor::beforeNewRun
   * 2. Conditions::onNewRun
   * 3. Processor::onNewRun
   *
   * This allows the user to have access to the run header both before
   * and after the conditions system accesses it.
   *
   * @see Processor::beforeNewRun for base method to user processors
   * @see Processor::onNewRun for base method to user processors
   * @see Conditions::onNewRun for what the conditions system does
   *
   * @param[in] rh RunHeader for new run to be processed
   */
  void newRun(RunHeader& rh);

  /**
   * process the event
   *
   * If the event is not aborted, the storage controller is given the choice
   * on whether the event should be kept. If it decides to "keep" the event,
   * then the event is saved to the output file 
   *
   * We first reset the event state via StorageControl::resetEventState
   * and then we go through the processors in order. If any of the processors
   * abort the event, we return false, otherwise we check with the storage
   * system on whether this event should be kept. Before leaving
   * **and after saving**, we call Event::next to move the event bus 
   * to the next event.
   *
   * @see Event::save for how the event saves in-memory objects to disk
   * @see StorageControl::keepEvent for how the keep decision is made
   * @param[in] n_events_processed number of events we have already processed
   *    only used for printing out a status message
   * @return true if event was successfully processed (i.e. not aborted)
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
  std::unique_ptr<Conditions> conditions_;

  /// event object
  Event event_;

  /// the current run header
  RunHeader* run_header_;

  /// log through the 'Process' channel
  ENABLE_LOGGING(Process);
};  // Process

}  // namespace fire

#endif  // FIRE_PROCESS_H
