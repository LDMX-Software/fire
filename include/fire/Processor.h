#ifndef FIRE_PROCESSOR_H
#define FIRE_PROCESSOR_H

#include "fire/exception/Exception.h"
#include "fire/Conditions.h"
#include "fire/Event.h"
#include "fire/config/Parameters.h"
#include "fire/exception/Exception.h"
#include "fire/logging/Logger.h"
#include "fire/RunHeader.h"
#include "fire/StorageControl.h"
#include "fire/factory/Factory.h"

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/

namespace fire {

/**
 * Forward declaration of Process class.
 * Each processor holds a reference to the current process class
 * so that they can access central systems like storage control
 * and conditions.
 */
class Process;

/**
 * @class Processor
 * @brief Base class for all event processing components
 */
class Processor {
 public:
  /**
   * @class AbortEventException
   * @brief Specific exception used to abort an event.
   * This inherits directly from std exception to try to 
   * keep it light. It should never be seen outside.
   */
  class AbortEventException : public std::exception {
   public:
    AbortEventException() noexcept : std::exception() {}
  };

 public:
  /**
   * Exceptions thrown by processors
   * We don't use the ENABLE_EXCEPTIONS macro here because
   * we want to add an extra parameter - the processor's name.
   */
  class Exception : public fire::exception::Exception {
   public:
    Exception(const std::string& name, const std::string &what) noexcept 
      : fire::exception::Exception(what), name_{name} {}
    const std::string& name() const noexcept {
      return name_;
    }
   private:
    /// name of processor which threw this exception
    std::string name_;
  };

 public:
  /**
   * Class constructor.
   *
   * The parameters a processor has access to are the member variables
   * of the python class in the sequence that has className equal to
   * the Processor class name.
   *
   * @param[in] ps Parameter set to be used to configure this processor
   * @param[in] p handle to process
   */
  Processor(const config::Parameters &ps);

  /**
   * Class destructor.
   */
  virtual ~Processor() = default;

  /**
   * Callback for the processor to take any necessary
   * actions before the run will be changed.
   * @note Only available to producers.
   * @param run header
   */
  virtual void beforeNewRun(RunHeader &runHeader) = 0;

  /**
   * Callback for the Processor to take any necessary
   * action when the run being processed changes.
   * @param runHeader The RunHeader containing run information.
   */
  virtual void onNewRun(const RunHeader &runHeader) {}

  /**
   * Callback for the Processor to take any necessary
   * action when a new input event file is opened.
   * @param filename Input event file name.
   * @note This callback is rarely used.
   */
  virtual void onFileOpen(const std::string &file_name) {}

  /**
   * Callback for the Processor to take any necessary
   * action when a event input file is closed.
   * @param filename Input event file name
   * @note This callback is rarely used.
   */
  virtual void onFileClose(const std::string &file_name) {}

  /**
   * Callback for the Processor to take any necessary
   * action when the processing of events starts, such as
   * creating histograms.
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
  const std::string& getName() const { return name_; }

  /**
   * The type of factory that can be used to create processors
   */
  using Factory = factory::Factory<Processor, std::unique_ptr<Processor>,
                                   const config::Parameters &>;

  /// have the derived processors do what they need to do
  virtual void process(Event &event) = 0;

  /**
   * Attach the current process to this processor.
   * Marked 'final' to prevent derived classes from redefining
   * this function and potentially abusing the handle to the current process.
   */
  virtual void attach(Process* p) final {
    process_ = p;
  }

 protected:
  /** 
   * Mark the current event as having the given storage control hint from this
   * processor and the given purpose string
   *
   * @param hint The storage control hint to apply for the given event
   * @param purpose A purpose string which can be used in the skim control
   * configuration to select which hints to "listen" to
   */
  void setStorageHint(StorageControl::Hint hint,
                      const std::string &purpose = "") const;

  /**
   * Access a conditions object for the current event
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
   */
  void fatalError(const std::string& msg) {
    throw Exception(name_, msg);
  }

  /**
   * The logger for this Processor
   *  The channel name for this logging stream is set to the
   *  name of the processor as configured.
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
  Process* process_{nullptr};
};

/**
 * @class Producer
 * @brief Base class for a module which produces a data product.
 *
 * @note This class processes a mutable copy of the event so that it can add
 * data to it.
 */
class Producer : public Processor {
 public:
  /**
   * Class constructor.
   * @param name Name for this instance of the class.
   * @param process The Process class associated with Processor, provided
   * by the fire
   *
   * @note Derived classes must have a constructor of the same interface, which
   * is the only constructor which will be called by the fire
   *
   * @note The provided name should not be the class name, but rather a logical
   * label for this instance of the class, as more than one copy of a given
   * class can be loaded into a Process with different parameters.  Names should
   * not include whitespace or special characters.
   */
  Producer(const config::Parameters &ps);

  /**
   * Process the event and put new data products into it.
   * @param event The Event to process.
   */
  virtual void produce(Event &event) = 0;

  /**
   * Handle allowing producers to modify run headers before the run begins
   * @param header RunHeader for Producer to add parameters to
   */
  virtual void beforeNewRun(RunHeader &header) {}

  /**
   * A producer produces when it is told to process
   *
   * @note Internal Function.
   * 'final override' so that downstream processors
   * can't modify how this processor processes
   */
  virtual void process(Event &event) final override { produce(event); }
};

/**
 * @class Analyzer
 * @brief Base class for a module which does not produce a data product.
 *
 * @note This class processes a constant copy of the event which cannot be
 * updated.
 */
class Analyzer : public Processor {
 public:
  /**
   * Class constructor.
   *
   * @param name Name for this instance of the class.
   * @param process The Process class associated with Processor, provided
   * by the fire
   *
   * @note Derived classes must have a constructor of the same interface, which
   * is the only constructor which will be called by the fire
   *
   * @note The provided name should not be the class name, but rather a logical
   * label for this instance of the class, as more than one copy of a given
   * class can be loaded into a Process with different parameters.  Names should
   * not include whitespace or special characters.
   */
  Analyzer(const config::Parameters &ps);

  /**
   * Make sure analyzers don't modify the run header
   * by doing a final override to prevent analyzers
   * from implementing this function.
   */
  virtual void beforeNewRun(RunHeader &) final override {}

  /**
   * Process the event through a const reference
   * @param event The Event to analyze
   */
  virtual void analyze(const Event &event) = 0;

  /**
   * An analyzer analyzes when it is told to process
   *
   * @note Internal Function.
   * 'final override' so that downstream processors
   * can't modify how this processor processes
   */
  virtual void process(Event &event) final override { analyze(event); }
};

}  // namespace fire

/**
 * @def DECLARE_PROCESSOR(CLASS)
 * @param CLASS The name of the class to register, which must not be in a
 * namespace.  If the class is in a namespace, use DECLARE_PROCESSOR_NS()
 * @brief Macro which allows the fire to construct a producer given its
 * name during configuration.
 * @attention Every processor class must call this macro or
 * DECLARE_PROCESSOR_NS() in the associated implementation (.cxx) file.
 */
#define DECLARE_PROCESSOR(CLASS)                                         \
  std::unique_ptr<fire::Processor> CLASS##_ldmx_make(                    \
      const fire::config::Parameters &ps) {                              \
    return std::make_unique<CLASS>(ps);                                  \
  }                                                                      \
  __attribute__((constructor)) static void CLASS##_ldmx_declare() {      \
    fire::Processor::Factory::get().declare(#CLASS, &CLASS##_ldmx_make); \
  }

/**
 * @def DECLARE_PROCESSOR_NS(NS,CLASS)
 * @param NS The full namespace specification for the Producer
 * @param CLASS The name of the class to register
 * @brief Macro which allows the fire to construct a producer given its
 * name during configuration.
 * @attention Every Producer class must call this macro or DECLARE_PROCESSOR()
 * in the associated implementation (.cxx) file.
 */
#define DECLARE_PROCESSOR_NS(NS, CLASS)                                     \
  namespace NS {                                                            \
  std::unique_ptr<fire::Processor> CLASS##_ldmx_make(                       \
      const fire::config::Parameters &ps) {                                 \
    return std::make_unique<CLASS>(ps);                                     \
  }                                                                         \
  __attribute__((constructor)) static void CLASS##_ldmx_declare() {         \
    fire::Processor::Factory::get().declare(                                \
        std::string(#NS) + "::" + std::string(#CLASS), &CLASS##_ldmx_make); \
  }                                                                         \
  }

#endif