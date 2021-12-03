/**
 * @file Process.h
 * @brief Class which represents the process under execution.
 * @author Jeremy Mans, University of Minnesota
 */

#ifndef LDMXSW_FRAMEWORK_PROCESS_H_
#define LDMXSW_FRAMEWORK_PROCESS_H_

// LDMX
#include "fire/Conditions.h"
#include "fire/Configure/Parameters.h"
#include "fire/Exception/Exception.h"
#include "fire/RunHeader.h"
#include "fire/StorageControl.h"

// STL
#include <map>
#include <memory>
#include <vector>

namespace fire {

class EventProcessor;
class EventFile;
class Event;

/**
 * @class Process
 * @brief Class which represents the process under execution.
 */
class Process {
 public:
  /**
   * Class constructor.
   * @param configuration Parameters to configure process with
   */
  Process(const config::Parameters &configuration);

  /**
   * Get the processing pass label.
   * @return The processing pass label.
   */
  const std::string &getPassName() const { return passname_; }

  /**
   * Get the current run number or the run number to be used when initiating new
   * events from the job
   * @return int Run number
   */
  int getRunNumber() const;

  /**
   * Get the pointer to the current event header, if defined
   */
  const ldmx::EventHeader *getEventHeader() const { return eventHeader_; }

  /**
   * Get the pointer to the current run header, if defined
   */
  const ldmx::RunHeader *getRunHeader() const { return runHeader_; }

  /**
   * Get a reference to the conditions system
   */
  Conditions &getConditions() { return conditions_; }

  /**
   * Get the frequency with which the event information is printed.
   * @return integer log frequency (negative if turned off)
   */
  int getLogFrequency() const { return logFrequency_; }

  /**
   * Run the process.
   */
  void run();

  /**
   * Request that the processing finish with this event
   */
  void requestFinish() { eventLimit_ = 0; }

  /**
   * Access the storage control unit for this process
   */
  StorageControl &getStorageController() { return storageController_; }

  /**
   * Set the pointer to the current event header, used only for tests
   */
  void setEventHeader(ldmx::EventHeader *h) { eventHeader_ = h; }

  /**
   * Get a dummy process
   *
   * This function returns an instance of this class without
   * any configuration. This is only helpful in the use case
   * where the user is writing a test for a processor and
   * needs to pass a Process object to the processor's constructor.
   *
   * @return Process without any configuration
   */
  static Process getDummy() { return std::move(Process()); }

 private:
  /**
   * Private dummy constructor
   * We hide it here because it shouldn't be used anywhere else.
   */
  Process() : conditions_{*this} {}

  /**
   * Process the input event through the sequence
   * of processors
   *
   * The input counter for number of events processed is
   * only used to print the status.
   *
   * @param[in] n counter for number of events processed
   * @param[in,out] event reference to event we are going to process
   * @returns true if event was full processed (false if aborted)
   */
  bool process(int n,Event& event) const;

  /**
   * Run through the processors and let them know
   * that we are starting a new run.
   *
   * @param[in] header RunHeader for the new run
   */
  void newRun(ldmx::RunHeader& header);

 private:
  /// The parameters used to configure this class.
  config::Parameters config_; 

  /** Processing pass name. */
  std::string passname_;

  /** Limit on events to process. */
  int eventLimit_;

  /** The frequency with which event info is printed. */
  int logFrequency_;

  /** Integer form of logging level to print to terminal */
  int termLevelInt_;

  /** Integer form of logging level to print to file */
  int fileLevelInt_;

  /** Name of file to print logging to */
  std::string logFileName_;

  /** Maximum number of attempts to make before giving up on an event */
  int maxTries_;

  /** Storage controller */
  StorageControl storageController_;

  /** Ordered list of EventProcessors to execute. */
  std::vector<std::unique_ptr<EventProcessor>> sequence_;

  /** Set of ConditionsProviders */
  Conditions conditions_;

  /** List of input files to process.  May be empty if this Process will
   * generate new events. */
  std::vector<std::string> inputFiles_;

  /** Output file name. Required. */
  std::string outputFile_;

  /** Set of drop/keep rules. */
  std::vector<std::string> dropKeepRules_;

  /** Run number to use if generating events. */
  int runForGeneration_{1};

  /** Pointer to the current EventHeader, used for Conditions information */
  const ldmx::EventHeader *eventHeader_{0};

  /** Pointer to the current RunHeader, used for Conditions information */
  const ldmx::RunHeader *runHeader_{0};

  /// Turn on logging for our process
  enableLogging("Process");
};  // Process

/**
 * A handle to the current process
 * Used to pass a process from ConfigurePython
 * to fire.cxx
 */
using ProcessHandle = std::unique_ptr<Process>;
}  // namespace fire

#endif
