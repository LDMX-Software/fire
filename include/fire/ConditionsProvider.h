#ifndef FIRE_CONDITIONSPROVIDER_H
#define FIRE_CONDITIONSPROVIDER_H

#include <map>

#include "fire/ConditionsIntervalOfValidity.h"
#include "fire/ConditionsObject.h"
#include "fire/config/Parameters.h"
#include "fire/exception/Exception.h"
#include "fire/factory/Factory.h"
#include "fire/logging/Logger.h"
#include "fire/RunHeader.h"

namespace fire {

class Conditions;

/**
 * @class ConditionsProvider
 * @brief Base class for all providers of conditions objects
 */
class ConditionsProvider {
 public:
  /**
   * The factory type that can create this class
   *
   * We provide the class type, the type of pointer for this class,
   * and the arguments to the constructor.
   */
  using Factory =
      factory::Factory<ConditionsProvider, std::shared_ptr<ConditionsProvider>,
                       config::Parameters const&>;
  /**
   * Class constructor.
   * @param ps Parameters to configure the provider
   * @param conditions The central Conditions management system, provided by the
   * fire.
   */
  ConditionsProvider(const fire::config::Parameters& ps);

  /**
   * Class destructor.
   */
  virtual ~ConditionsProvider() = default;

  /**
   * Pure virtual getCondition function.
   * Must be implemented by any Conditions providers.
   */
  virtual std::pair<const ConditionsObject*, ConditionsIntervalOfValidity> getCondition(
      const EventHeader& context) = 0;

  /**
   * Called by conditions system when done with a conditions object, appropriate
   * point for cleanup.
   * @note Default behavior is to delete the object!
   */
  virtual void release(const ConditionsObject* co) { delete co; }

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events starts.
   */
  virtual void onProcessStart() {}

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events finishes, such as closing
   * database connections.
   */
  virtual void onProcessEnd() {}

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events starts for a given run.
   */
  virtual void onNewRun(RunHeader&) {}

  /**
   * Get the list of conditions objects available from this provider.
   */
  const std::string& getConditionObjectName() const { return objectName_; }

  /**
   * Access the tag name
   */
  const std::string& getTagName() const { return tagname_; }

  /**
   * Attach the central conditions system to this provider
   */
  virtual void attach(Conditions* c) final { conditions_ = c; }

 protected:
  /** Request another condition needed to construct this condition */
  std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
  requestParentCondition(const std::string& name,
                         const EventHeader& context);

  /// The logger for this ConditionsProvider
  mutable logging::logger theLog_;

 private:
  /** Handle to the central conditions system. */
  Conditions* conditions_{nullptr};

  /** The name of the object provided by this provider. */
  std::string objectName_;

  /** The tag name for the ConditionsProvider. */
  std::string tagname_;
};

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
#define DECLARE_CONDITIONS_PROVIDER(CLASS)                                \
  std::shared_ptr<fire::ConditionsProvider> CLASS##_fire_make(            \
      const fire::config::Parameters& params) {                           \
    return std::make_shared<CLASS>(params);                               \
  }                                                                       \
  __attribute__((constructor)) static void CLASS##_fire_declare() {       \
    fire::ConditionsProvider::Factory::get().declare(#CLASS,              \
                                                     &CLASS##_fire_make); \
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
  std::shared_ptr<fire::ConditionsProvider> CLASS##_fire_make(              \
      const fire::config::Parameters& params) {                             \
    return std::make_shared<NS::CLASS>(params);                             \
  }                                                                         \
  __attribute__((constructor)) static void CLASS##_fire_declare() {         \
    fire::ConditionsProvider::Factory::get().declare(                       \
        std::string(#NS) + "::" + std::string(#CLASS), &CLASS##_fire_make); \
  }                                                                         \
  }

#endif
