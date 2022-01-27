#ifndef FIRE_CONDITIONSPROVIDER_H
#define FIRE_CONDITIONSPROVIDER_H

#include <map>

#include "fire/ConditionsIntervalOfValidity.h"
#include "fire/ConditionsObject.h"
#include "fire/RunHeader.h"
#include "fire/config/Parameters.h"
#include "fire/exception/Exception.h"
#include "fire/factory/Factory.h"
#include "fire/logging/Logger.h"

namespace fire {

/// forward declaration for attachment
class Conditions;

/**
 * Base class for all providers of conditions objects
 *
 * Besides defining the necessary virtual callbacks,
 * we also provide the factory infrastructure for dynamically
 * creating providers from loaded libraries and we define
 * a requestParentCondition function that can be used by
 * other providers to obtain conditions other conditions
 * depend on.
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
   * Configure the registered provider
   * @param[in] ps Parameters to configure the provider
   */
  ConditionsProvider(const fire::config::Parameters& ps);

  /**
   * default destructor, virtual so derived types can be destructed
   */
  virtual ~ConditionsProvider() = default;

  /**
   * Pure virtual getCondition function.
   *
   * Must be implemented by any Conditions providers.
   *
   * @param[in] context EventHeader for the condition
   * @return pair of condition and its interval of validity
   */
  virtual std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
  getCondition(const EventHeader& context) = 0;

  /**
   * Called by conditions system when done with a conditions object, 
   * appropriate point for cleanup.
   *
   * @note Default behavior is to delete the object!
   *
   * @param[in] co condition to cleanup
   */
  virtual void release(const ConditionsObject* co) { delete co; }

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events starts.
   */
  virtual void onProcessStart() {}

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events finishes, 
   * such as closing database connections.
   */
  virtual void onProcessEnd() {}

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events starts for a given run.
   */
  virtual void onNewRun(RunHeader&) {}

  /**
   * Get the condition object available from this provider.
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
  /** 
   * Request another condition needed to construct this condition
   *
   * This is where we use the handle to the central conditions system
   * and this allows us to recursively depend on other instances 
   * ConditionsProvider.
   *
   * @param[in] name condition name that is needed
   * @param[in] context EventHeader for which to get condition
   * @return pair of parent condition and its interval of validity
   */
  std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
  requestParentCondition(const std::string& name, const EventHeader& context);

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
 * @param CLASS The name of the class to register including namespaces
 * @brief Macro which allows fire to construct a provider given its
 * name during configuration.
 * @attention Every ConditionsProvider class must call this macro in
 * the associated implementation (.cxx) file.
 *
 * If you are getting a 'redefinition of v' compilation error from
 * this macro, then that means you have more than one ConditionsProvider
 * defined within a single compilation unit. This is not a problem,
 * you just need to expand the macro yourself:
 * ```cpp
 * // in source file
 * //   the names of the variables don't matter as long as they are different
 * namespace {
 * auto v0 = ::fire::ConditionsProvider::Factory.get().declare<One>("One");
 * auto v1 = ::fire::ConditionsProvider::Factory.get().declare<Two>("Two");
 * }
 * ```
 */
#define DECLARE_CONDITIONS_PROVIDER(CLASS)                                  \
  namespace {                                                               \
  auto v = fire::ConditionsProvider::Factory::get().declare<CLASS>(#CLASS); \
  }

#endif
