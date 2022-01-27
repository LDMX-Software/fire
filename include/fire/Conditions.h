#ifndef FIRE_CONDITIONS_H
#define FIRE_CONDITIONS_H

#include "fire/exception/Exception.h"
#include "fire/EventHeader.h"
#include "fire/exception/Exception.h"
#include "fire/ConditionsProvider.h"
#include "fire/config/Parameters.h"
#include "fire/logging/Logger.h"

#include <map>

namespace fire {

/// forward declaration for constructor
class Process;

/**
 * Container and cache for conditions and conditions providers
 *
 * ## Conditions System
 * This class and the classes in its immediate ecosystem
 * (ConditionsProvider, ConditionsObject, and ConditionsIntervalOfValidity)
 * are helpful for defining a "condition system".
 * As the name implies, this system provides information
 * about the conditions or circumstances in which a particular
 * event's data was taken. For example, if we are taking data using
 * a device that depends on the temperature, we would want to record
 * the temperature that a certain series of events were taken at.
 *
 * Since (most of the time) conditions information is changing
 * slowly relative to the events (i.e. we expect one piece of conditions
 * information to be valid for many events in a row), this system
 * is separate from the Event which holds the data.
 *
 * ## Design
 * The Conditions system holds all registered instances of ConditionsProvider
 * in memory, waiting for a Processor to request a specific condition.
 * If a provider that can provide the requested condition is available,
 * the system caches that provider and returns the object that it provides.
 * The system also requres instances of ConditionsProvider to define 
 * the range of validity that the condition can be used for so that the
 * system can automatically update the condition when events with different
 * conditions are encountered. This design makes it safe and efficient
 * for a Processor to use Processor::getCondition _on every event_ since
 * this will only cause the construction of a new object on events where
 * the conditions change according to their changing validity.
 *
 * A reasonable question to ask is why even have a separation between 
 * providers and objects? The answer is because many of the provided
 * objects could be memory intensive (for example, a big table of read
 * out chip settings to be used for later reconstruction of the data),
 * so having this memory only used when specifically requested makes the
 * system easier and safer to use.
 *
 * ## Usage
 * Users of fire will inevitably want to define their own conditions
 * with their own providers. Conditions are defined simply by deriving from
 * ConditionsObject so that the system can cache them. Providers are defined
 * similarly as how a Processor is defined: deriving from the parent class
 * ConditionsProvider and declaring it using a macro in the source file.
 */
class Conditions {
 public:
  /**
   * Configure the conditions system and attach the current process
   *
   * This is where the registered instances of ConditionsProvider 
   * are constructed.
   *
   * @throws Exception if two registerned providers provide the
   * same condition.
   *
   * @param[in] ps config::Parameters to configure
   * @param[in] p handle to current Process
   */
  Conditions(const config::Parameters& ps, Process& p);

  /**
   * Core request action for a conditions object
   *
   * If the object is in the cache and still valid (IOV), the
   * cached object will be returned.
   * If it is not in the cache, or is out of date, 
   * the ConditionsProvider::getCondition method
   * will be called to provide the object.
   *
   * @throws Exception if condition object or provider for that object is not
   * found.
   *
   * @param[in] condition_name name of condition to retrieve
   * @returns pointer to conditions object with input name
   */
  const ConditionsObject* getConditionPtr(const std::string& condition_name);

  /**
   * Primary request action for a conditions object
   *
   * This is a small wrapper around getConditionPtr which does the
   * necessary type conversion.
   *
   * @throws Exception if requested condition object cannot be
   * converted to the requested type
   *
   * @see getConditionPtr for how the requests are handled
   * @tparam T type to cast condition object to
   * @param[in] condition_name name of condition to retrieve
   * @returns const reference to conditions object
   */
  template <class ConditionType>
  const ConditionType& get(const std::string& condition_name) {
    try {
      return dynamic_cast<const ConditionType&>(*getConditionPtr(condition_name));
    } catch (const std::bad_cast&) {
      throw Exception("BadCast",
          "Condition '"+condition_name+"' is not the input type '"
          +boost::core::demangle(typeid(ConditionType).name())+"'.");
    }
  }

  /**
   * Access the interval of validity for the given condition
   *
   * @param[in] condition_name name of condition to get IOV for
   * @returns Interval Of Validity for the input condition name
   */
  ConditionsIntervalOfValidity getConditionIOV(const std::string& condition_name) const;

  /**
   * Calls onProcessStart for all instances of ConditionsProvider
   */
  void onProcessStart();

  /**
   * Calls onProcessEnd for all instances of ConditionsProvider
   */
  void onProcessEnd();

  /**
   * Calls onNewRun for all instances of ConditionsProvider
   *
   * @param[in] rh current RunHeader
   */
  void onNewRun(RunHeader& rh);

 private:
  /** 
   * Handle to the Process. 
   *
   * We need this so we can provide it to all instances of ConditionsProvider
   * and use it to obtain a handle to the current event header.
   */
  Process& process_;

  /** Map of who provides which condition */
  std::map<std::string, std::shared_ptr<ConditionsProvider>> providers_;

  /**
   * An entry to store an already loaded conditions object
   */
  struct CacheEntry {
    /// Interval Of Validity for this entry in the cache
    ConditionsIntervalOfValidity iov;
    /// Provider that gave us the conditions object
    std::shared_ptr<ConditionsProvider> provider;
    /// Const pointer to the retrieved conditions object
    const ConditionsObject* obj;
  };

  /** Conditions cache */
  std::map<std::string, CacheEntry> cache_;
};

}  // namespace fire

#endif
