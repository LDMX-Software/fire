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
 *
 * ## Usage
 * Defining a new processor can be done to varying levels of complexity;
 * however, they fall into two distinct groups: configurable and 
 * non-configurable. 
 *
 * ### Non-configurable Processors
 * Non-configurable processors cannot be configured
 * at run time from the python configuration.
 * This makes starting them simpler even if they are slightly
 * less powerful.
 *
 * ```cpp
 * // MyProcessor.cpp
 * #include <fire/Processor.h>
 * class MyProcessor : public fire::Processor {
 *  public:
 *   MyProcessor(const fire::config::Parameters& ps)
 *    : fire::Processor(ps) {}
 *   ~MyProcessor() = default;
 *   void process(fire::Event& event) final override {
 *     // process event here!
 *   }
 * };
 * DECLARE_PROCESSOR(MyProcessor);
 * ```
 *
 * This example shows the necessary parts of a new Processor.
 * 1. Inherits from `fire::Processor`
 * 2. Constructor accepts configuration parameters and passes them to
 *    the base class
 * 3. Destructor is defined (even if it is default)
 * 4. Definition of the function to do the processing
 * 5. Calling the declaration macro after class is declared.
 *
 * This processor is already ready to be compiled and added
 * into a Python configuration. Any of the other call backs 
 * in this class can then be defined by your derived processor
 * if you see it as useful.
 *
 * Let's suppose that this processor is compiled into a library
 * called `libMyModule.so` (perhaps with other processors or tools).
 * Then, we can use this processor with fire from a python configuration
 * file as shown below.
 * ```py
 * import fire.cfg
 * p = fire.cfg.Process('example')
 * p.sequence = [
 *   fire.cfg.Processor('eg','MyProcessor',library='/full/path/to/libMyModule.so')
 *   ]
 * ```
 * The full path to `libMyModule.so` only needs to be provided if it is
 * not accessible by `ld` (i.e. it is not in a directory listed in LD_LIBRARY_PATH
 * or a system directory). Moreover, if the library is accessible by `ld`
 * and you are on a Linux system (so that the libraries follow the naming
 * format of the example), you could replace the `library` argument with
 * `module='MyModule'` to make it easier to read. This second option is more
 * common for larger software sets with many modules.
 *
 * You can determine if your library is accessible by the linker using
 * some [fancy command line nonsense](https://unix.stackexchange.com/a/282207).
 *
 * ### Configurable Processor
 * Making a processor configurable from Python is not very complicated on the
 * C++ side, but it introduces many complexities and nuances on the Python side. 
 *
 * On the C++ side, it simply involves expanding the constructor in
 * order to use the passed set of parameters to define member variables.
 * Expanding on the example from above:
 *
 * ```cpp
 * // MyProcessor.cpp
 * #include <fire/Processor.h>
 * class MyProcessor : public fire::Processor {
 *   int my_parameter_;
 *   double my_required_parameter_;
 *  public:
 *   MyProcessor(const fire::config::Parameters& ps)
 *    : fire::Processor(ps) {
 *      my_parameter_ = ps.get<int>("my_parameter",1);
 *      my_required_parameter_ = ps.get<double>("my_required_parameter");
 *    }
 *   ~MyProcessor() = default;
 *   void process(fire::Event& event) final override {
 *     // process event here!
 *
 *   }
 * };
 * DECLARE_PROCESSOR(MyProcessor);
 * ```
 * This constructor will recieves two parameters that are configurable.
 * Exceptions are thrown if the parameter in Python cannot be converted 
 * to the passed type. For example, if `my_parameter` in Python is set 
 * to `2.0` instead of `2`.
 * 1. `my_parameter_` is optional with a default value of `1`.
 * 2. `my_required_parameter` is required - i.e. an exception will be
 *    thrown if a parameter with that name is not found
 *
 * Now, onto the more complicated Python side.
 * There are three main methods for defining parameters on the Python
 * side of configuration. In everything below, `my_proc` is the Python
 * object that would be added to `p.sequence` inside of the configuration
 * script so that the Processor will be used during the run.
 *
 * First, the base configuration class `fire.cfg.Processor` allows
 * the user to define parameters directly. This is helpful for small
 * processors that don't have an entire Python module supporting them.
 * ```py
 * my_proc = fire.cfg.Processor('my_proc','MyProcessor',
 *                              library='/full/path/to/libMyModule.so',
 *                              my_parameter = 2, 
 *                              my_required_parameter = 3.0)
 * ```
 *
 * Next, we could wrap the code above into a function. This is helpful
 * for portability because now we can put this function into a Python
 * module that could be imported in the configuration script. Moreover,
 * this isolates the parameter spelling to one location so that the
 * the user does not have to worry about mis-spelling parameters.
 * ```py
 * def MyProcessor(name, req, opt = 2) :
 *     return fire.cfg.Processor(name,'MyProcessor',
 *                               library='/full/path/to/libMyModule.so',
 *                               my_parameter = opt, 
 *                               my_required_parameter = req)
 * 
 * # later inside the python config
 * my_proc = MyProcessor('my_proc',5.0)
 * ```
 *
 * Finally, we can create a child class of the parent configuration class.
 * This is the most complicated method and should only be used if the
 * determination of parameters requires some extra Python functions.
 * ```py
 * class MyProcessor(fire.cfg.Processor) :
 *     def __init__(name, req) :
 *         super().__init__(name,'MyProcessor',
 *                          library='/full/path/to/libMyModule.so')
 *         self.my_parameter = 2
 *         self.my_required_parameter = req
 * 
 * # later inside the python config
 * my_proc = MyProcessor('my_proc',5.0)
 * ```
 *  
 * @note Python's variable handling is very dynamic.
 *  For us, this means that we need to be very careful that
 *  the parameters in Python are spelled the same as the parameters
 *  in C++. In Python, the variables provided to `fire.cfg.Processor`
 *  (or defined in `__init__` for the last option) need to have 
 *  **exactly** the same name as the parameter names requested
 *  in the constructor of the C++ processor.
 *
 * ## Callback Ordering
 * The ordering of the callback functions is decided by the Process::run
 * and is helpful to document here for users creating new Processors.
 *
 * 1. Constructor - before any processing begins, all of the processors
 *    are constructed and passed their configuration parameters.
 * 2. onProcessStart - before any processing begins but after the conditions
 *    providers are told the process is starting
 * 3. onFileOpen - **only if there are input files**, this is called before any
 *    events in the input file are processed
 * 4. beforeNewRun - called before conditions providers are given the 
 *    run header when a new run is encountered (i.e. before processing begins
 *    when there are not input files or when the event header has a new
 *    run number when there are run numbers)
 * 5. onNewRun - called after conditions providers are given the run header
 * 6. process - called on each event
 * 7. onFileClose - **only if there are input files**, this is called after all 
 *    events in the input file are processed
 * 8. onProcessEnd - all processing is done and the Processors are about
 *    to be destructed.
 * 9. Destructor - the processors are destructed automatically in
 *    the destruction of the core Process object
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
   * This base class assumes the existence of an additional parameter
   * 'name' for which a logger can be constructed and an error message
   * can be labeled.
   *
   * @param[in] ps Parameter set to be used to configure this processor
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
   * within the processor and it can be used to acquire conditions
   * that do not change with respect to changing event contexts.
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
 public:
  /**
   * The special factory used to create processors
   *
   * we need a special factory because it needs to be able to create processors
   * with two different construtor options
   *
   * When "old-style" processors can be abandoned, this redundant code can
   * be removed in favor of the templated factory:
   * ```cpp 
   * using Factory = factory::Factory<Processor, std::unique_ptr<Processor>, 
   *  const config::Parameters &>;
   * ```
   * and then adding the following line to the loop constructing Processors
   * in the Process constructor:
   * ```cpp 
   * sequence_.emplace_back(Processor::Factory::get().make(class_name, proc));
   * sequence_.back()->attach(this);
   * ```
   */
  class Factory {
   public:
    /**
     * The base pointer for processors
     */
    using PrototypePtr = std::unique_ptr<Processor>;

    /**
     * the signature of a function that can be used by this factory
     * to dynamically create a new object.
     *
     * This is merely here to make the definition of the Factory simpler.
     */
    using PrototypeMaker = std::function<PrototypePtr(const config::Parameters&,Process&)>;
  
   public:
    /**
     * get the factory instance
     *
     * Using a static function variable gaurantees that the factory
     * is created as soon as it is needed and that it is deleted
     * before the program completes.
     *
     * @returns reference to single Factory instance
     */
    static Factory& get() {
      static Factory the_factory;
      return the_factory;
    }
  
    /**
     * register a new object to be constructible
     *
     * We insert the new object into the library after
     * checking that it hasn't been defined before.
     *
     * @note This uses the demangled name of the input type
     * as the key in our library of objects. Using the demangled
     * name effectively assumes that all of the libraries being
     * loaded were compiled with the same compiler version.
     * We could undo this assumption by having the key be an
     * input into this function.
     *
     * @param[in] full_name name to use as a reference for the declared object
     * @param[in] maker a pointer to a function that can dynamically create an instance
     * @return value to define a static variable to force running this function
     *  at library load time. It relates to variables so that it cannot be
     *  optimized away.
     */
    template<typename DerivedType>
    uint64_t declare() {
      std::string full_name{boost::core::demangle(typeid(DerivedType).name())};
      library_[full_name] = &maker<DerivedType>;
      return reinterpret_cast<std::uintptr_t>(&library_);
    }
  
    /**
     * make a new object by name
     *
     * We look through the library to find the requested object.
     * If found, we create one and return a pointer to the newly
     * created object. If not found, we raise an exception.
     *
     * @throws Exception if the input object name could not be found
     *
     * The arguments to the maker are determined at compiletime
     * using the template parameters of Factory.
     *
     * @param[in] full_name name of object to create, same name as passed to declare
     * @param[in] maker_args parameter pack of arguments to pass on to maker
     *
     * @returns a pointer to the parent class that the objects derive from.
     */
    PrototypePtr make(const std::string& full_name,
                      const config::Parameters& ps,
                      Process& p) {
      auto lib_it{library_.find(full_name)};
      if (lib_it == library_.end()) {
        throw Exception("Factory","An object named " + full_name +
                         " has not been declared.",false);
      }
      return lib_it->second(ps,p);
    }
  
    /// delete the copy constructor
    Factory(Factory const&) = delete;
  
    /// delete the assignment operator
    void operator=(Factory const&) = delete;
  
   private:
    /**
     * make a DerivedType returning a PrototypePtr
     *
     * We do a constexpr check on which type of processor it is. If it can be constructed
     * from a Parameters alone, then it is a "new" type. Otherewise, it is a "old" type.
     *
     * @tparam DerivedType type of derived object we should create
     * @param[in] args constructor arguments for derived type construction
     */
    template <typename DerivedType>
    static PrototypePtr maker(const config::Parameters& parameters, Process& process) {
      std::unique_ptr<Processor> ptr;
      if constexpr (std::is_constructible<DerivedType, const config::Parameters&>::value) {
        // new type
        ptr = std::make_unique<DerivedType>(parameters);
        ptr->attach(&process);
      } else {
        // old type
        ptr = std::make_unique<DerivedType>(parameters.get<std::string>("name"), process);
        dynamic_cast<DerivedType*>(ptr.get())->configure(parameters);
      }
      return ptr;
    }
  
    /// private constructor to prevent creation
    Factory() = default;
  
    /// library of possible objects to create
    std::unordered_map<std::string, PrototypeMaker> library_;
  };  // Factory

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
 * auto v0 = ::fire::Processor::Factory.get().declare<One>();
 * auto v1 = ::fire::Processor::Factory.get().declare<Two>();
 * }
 * ```
 */
#define DECLARE_PROCESSOR(CLASS)                             \
  namespace {                                                \
  auto v = fire::Processor::Factory::get().declare<CLASS>(); \
  }

#endif
