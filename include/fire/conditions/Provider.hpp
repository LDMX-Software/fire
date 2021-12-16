#ifndef FIRE_CONDITIONS_PROVIDER_HPP
#define FIRE_CONDITIONS_PROVIDER_HPP

#include <map>

#include "fire/conditions/IntervalOfValidity.hpp"
#include "fire/conditions/Base.hpp"
#include "fire/config/Parameters.hpp"
#include "fire/exception/Exception.hpp"
#include "fire/Logger.hpp"
#include "fire/factory/Factory.hpp"

namespace fire {
namespace conditions {

class Process;

/**
 * @class Provider
 * @brief Base class for all providers of conditions objects
 */
class Provider {
 public:
  /**
   * The factory type that can create this class
   *
   * We provide the class type, the type of pointer for this class,
   * and the arguments to the constructor.
   */
  using Factory = factory::Factory<Provider,
                                   std::unique_ptr<Provider>,
                                   std::string const&, std::string const&,
                                   config::Parameters const&, Process&>;
  /**
   * Class constructor.
   * @param name Name for this instance of the class.
   * @param tagName The tag for the database entry (should not include
   * whitespace)
   * @param process The Process class associated with Provider,
   * provided by the fire.
   *
   * @note The name provided to this function should not be
   * the class name, but rather a logical label for this instance of
   * the class, as more than one copy of a given class can be loaded
   * into a Process with different parameters.  Names should not include
   * whitespace or special characters.
   */
  Provider(const std::string& objname,
           const std::string& tagname,
           const fire::config::Parameters& parameters,
           Process& process) noexcept
    : process_{process},
      objectName_{objname},
      tagname_{tagname},
      theLog_{logging::makeLogger(objname)} {}


  /**
   * Class destructor.
   */
  virtual ~Provider() = default;

  /**
   * Pure virtual getCondition function.
   * Must be implemented by any Conditions providers.
   */
  virtual std::pair<const Base*, ConditionsIOV> getCondition(
      const ldmx::EventHeader& context) = 0;

  /**
   * Called by conditions system when done with a conditions object, appropriate
   * point for cleanup.
   * @note Default behavior is to delete the object!
   */
  virtual void release(const ConditionsObject* co) {
    delete co;
  }

  /**
   * Callback for the Provider to take any necessary
   * action when the processing of events starts.
   */
  virtual void onProcessStart() {}

  /**
   * Callback for the Provider to take any necessary
   * action when the processing of events finishes, such as closing
   * database connections.
   */
  virtual void onProcessEnd() {}

  /**
   * Callback for the Provider to take any necessary
   * action when the processing of events starts for a given run.
   */
  virtual void onNewRun(ldmx::RunHeader&) {}

  /**
   * Get the list of conditions objects available from this provider.
   */
  const std::string& getConditionObjectName() const { return objectName_; }

  /**
   * Access the tag name
   */
  const std::string& getTagName() const { return tagname_; }

 protected:
  /** Request another condition needed to construct this condition */
  std::pair<const Base*, ConditionsIOV> requestParentCondition(
      const std::string& name, const ldmx::EventHeader& context);

  /// The logger for this Provider
  logging::logger theLog_;

  /** Get the process handle */
  const Process& process() const { return process_; }

 private:
  /** Handle to the Process. */
  Process& process_;

  /** The name of the object provided by this provider. */
  std::string objectName_;

  /** The tag name for the Provider. */
  std::string tagname_;
};

}  // namespace conditions
}  // namespace fire

/**
 * @def DECLARE_CONDITIONS_PROVIDER(CLASS)
 * @param CLASS The name of the class to register, which must not be in a
 * namespace.  If the class is in a namespace, use
 * DECLARE_CONDITIONS_PROVIDER_NS()
 * @brief Macro which allows the fire to construct a COP given its
 * name during configuration.
 * @attention Every COP class must call this macro or
 * DECLARE_CONDITIONS_PROVIDER_NS() in the associated implementation (.cxx)
 * file.
 */
#define DECLARE_CONDITIONS_PROVIDER(CLASS)                              \
  std::unique_ptr<fire::conditions::Provider> CLASS##_ldmx_make(    \
      const std::string& name, const std::string& tagname,              \
      const fire::config::Parameters& params, fire::Process& process) { \
    return std::make_unique<CLASS>(name, tagname, params, process);     \
  }                                                                     \
  __attribute__((constructor)) static void CLASS##_ldmx_declare() {     \
    fire::conditions::Provider::Factory::get().declare(             \
        #CLASS, &CLASS##_ldmx_make);                                    \
  }

/**
 * @def DECLARE_CONDITIONS_PROVIDER_NS(NS,CLASS)
 * @param NS The full namespace specification for the Producer
 * @param CLASS The name of the class to register
 * @brief Macro which allows the fire to construct a producer given its
 * name during configuration.
 * @attention Every Producer class must call this macro or
 * DECLARE_CONDITIONS_PROVIDER() in the associated implementation (.cxx) file.
 */
#define DECLARE_CONDITIONS_PROVIDER_NS(NS, CLASS)                           \
  namespace NS {                                                            \
  std::unique_ptr<fire::conditions::Provider> CLASS##_ldmx_make(        \
      const std::string& name, const std::string& tagname,                  \
      const fire::config::Parameters& params, fire::Process& process) {     \
    return std::make_unique<NS::CLASS>(name, tagname, params, process);     \
  }                                                                         \
  __attribute__((constructor)) static void CLASS##_ldmx_declare() {   \
    fire::conditions::Provider::Factory::get().declare(                 \
        std::string(#NS) + "::" + std::string(#CLASS), &CLASS##_ldmx_make); \
  }                                                                         \
  }

#endif
