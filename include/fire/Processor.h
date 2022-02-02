#ifndef FIRE_PROCESSOR_H
#define FIRE_PROCESSOR_H

#include "fire/Conditions.h"
#include "fire/Event.h"
#include "fire/RunHeader.h"
#include "fire/StorageControl.h"
#include "fire/config/Parameters.h"
#include "fire/exception/Exception.h"
#include "fire/factory/Factory.h"
#include "fire/logging/Logger.h"

namespace fire {

/**
 * Forward declaration of Process class.
 *
 * Each processor holds a reference to the current process class
 * so that they can access central systems like storage control
 * and conditions.
 */
class Process;

/**
 * Base class for all event processing components
 *
 * This is the main interface that users of fire will interact with.
 * In order to perform a new task, a user would define a new Processor
 * which can take data from the event bus, process it in some way, and
 * then add new data onto the event bus. This format is applicable all
 * along the data processing chain from generation (simulation or raw
 * decoding) to reconstruction to analysis.
 */
class Processor {
 public:
  /**
   * Specific exception used to abort an event.
   *
   * This inherits directly from std exception to try to
   * keep it light. It should never be seen outside.
   */
  class AbortEventException : public std::exception {
   public:
    AbortEventException() noexcept : std::exception() {}
  };

 public:
  /**
   * Configure the processor upon construction.
   *
   * The parameters a processor has access to are the member variables
   * of the python class in the sequence that has class_name equal to
   * the Processor class name.
   *
   * @param[in] ps Parameter set to be used to configure this processor
   * @param[in] p handle to process
   */
  Processor(const config::Parameters &ps);

  /**
   * virtual default destructor so derived classes can be destructed
   */
  virtual ~Processor() = default;

  /**
   * Handle allowing processors to modify run headers before the run begins
   *
   * This is called _before_ any conditions providers are given the run
   * header, so it can be used to provide parameters that conditions providers
   * require.
   *
   * @param header RunHeader for Processor to add parameters to
   */
  virtual void beforeNewRun(RunHeader &header) {}

  /**
   * Callback for the Processor to take any necessary
   * action when the run being processed changes.
   *
   * This is called _after_ any conditions providers are configured
   * with the run header, so it can be used for internal book-keeping
   * within the processor.
   *
   * @param runHeader The RunHeader containing run information.
   */
  virtual void onNewRun(const RunHeader &runHeader) {}

  /**
   * Callback for the Processor to take any necessary
   * action when a new _input_ event file is opened.
   *
   * @note This callback is only used with their are input files.
   *
   * @param[in] filename Input event file name.
   */
  virtual void onFileOpen(const std::string &file_name) {}

  /**
   * Callback for the Processor to take any necessary
   * action when a event _input_ file is closed.
   *
   * @note This callback is only used with their are input files.
   *
   * @param[in] filename Input event file name
   */
  virtual void onFileClose(const std::string &file_name) {}

  /**
   * Callback for the Processor to take any necessary
   * action when the processing of events starts.
   */
  virtual void onProcessStart() {}

  /**
   * Callback for the Processor to take any necessary
   * action when the processing of events finishes, such as
   * calculating job-summary quantities.
   */
  virtual void onProcessEnd() {}

  /**
   * Get the processor name
   */
  const std::string &getName() const { return name_; }

  /**
   * The type of factory that can be used to create processors
   */
  using Factory = factory::Factory<Processor, std::unique_ptr<Processor>,
                                   const config::Parameters &>;

  /**
   * have the derived processors do what they need to do
   *
   * @see Event::get for retrieve event data objects
   * @see Event::add for adding new data objects
   *
   * @param[in] event Event holding the data to be processed
   */
  virtual void process(Event &event) = 0;

  /**
   * Attach the current process to this processor.
   *
   * Marked 'final' to prevent derived classes from redefining
   * this function and potentially abusing the handle to the current process.
   *
   * @param[in] p pointer to current Process
   */
  virtual void attach(Process *p) final { process_ = p; }

 protected:
  /**
   * Mark the current event as having the given storage control hint from this
   * processor and the given purpose string
   *
   * @param[in] hint The storage control hint to apply for the given event
   * @param[in] purpose A purpose string which can be used in the skim control
   * configuration to select which hints to "listen" to
   */
  void setStorageHint(StorageControl::Hint hint,
                      const std::string &purpose = "") const;

  /**
   * Access a conditions object for the current event
   *
   * @see Conditions::get and Conditions::getConditionPtr for
   * how conditions objects are retrieved.
   *
   * ## Usage
   * Inside of the process function, this function can be used 
   * following the example below.
   * ```cpp
   * // inside void process(Event& event) for your Processor
   * const auto& co = getCondition<MyObjectType>("ConditionName");
   * ```
   * The `const` and `&` are there because `auto` is not usually
   * able to deduce that they are necessary and without them, a
   * deep copy of the condition object would be made at this point.
   *
   * @tparam T type of condition object
   * @param[in] condition_name Name of condition object to retrieve
   * @return const handle to the condition object
   */
  template <class T>
  const T &getCondition(const std::string &condition_name) {
    return getConditions().get<T>(condition_name);
  }

  /**
   * Abort the event immediately.
   *
   * Skip the rest of the sequence and don't save anything in the event bus.
   */
  void abortEvent() { throw AbortEventException(); }

  /**
   * End processing due to a fatal runtime error.
   *
   * @param[in] msg Error message to printout
   */
  inline void fatalError(const std::string &msg) { throw Exception(name_, msg); }

  /**
   * The logger for this Processor
   *
   * The channel name for this logging stream is set to the
   * name of the processor as configured in the Processor constructor.
   */
  mutable logging::logger theLog_;

 private:
  /**
   * Internal getter for conditions without exposing all of Process
   */
  Conditions &getConditions() const;

  /** The name of the Processor. */
  std::string name_;

  /// Handle to current process
  Process *process_{nullptr};
};

}  // namespace fire

/**
 * @def DECLARE_PROCESSOR(CLASS)
 * @param CLASS The name of the class to register including namespaces
 * @brief Macro which allows the fire to construct a producer given its
 * name during configuration.
 * @attention Every processor class must call this macro in
 * the associated implementation (.cxx) file.
 *
 * If you are getting a 'redefinition of v' compilation error from
 * this macro, then that means you have more than one Processor
 * defined within a single compilation unit. This is not a problem,
 * you just need to expand the macro yourself:
 * ```cpp
 * // in source file
 * namespace {
 * auto v0 = ::fire::Processor::Factory.get().declare<One>("One");
 * auto v1 = ::fire::Processor::Factory.get().declare<Two>("Two");
 * }
 * ```
 */
#define DECLARE_PROCESSOR(CLASS)                                   \
  namespace {                                                      \
  auto v = fire::Processor::Factory::get().declare<CLASS>(#CLASS); \
  }

#endif
