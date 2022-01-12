#ifndef FIRE_CONDITIONS_H
#define FIRE_CONDITIONS_H

/*~~~~~~~~~~~*/
/*   Event   */
/*~~~~~~~~~~~*/
#include "fire/exception/Exception.h"
#include "fire/EventHeader.h"
#include "fire/exception/Exception.h"
#include "fire/ConditionsProvider.h"
#include "fire/config/Parameters.h"
#include "fire/logging/Logger.h"

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <map>

namespace fire {

class Process;

/**
 * @class Conditions
 * @brief Container and cache for conditions and conditions providers
 */
class Conditions {
 public:
  /**
   * Constructor
   */
  Conditions(const config::Parameters& ps, Process& p);

  /**
   * Primary request action for a conditions object If the
   * object is in the cache and still valid (IOV), the
   * cached object will be returned.  If it is not in the cache,
   * or is out of date, the ConditionsObjectProvider::getCondition method
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
   * Primary request action for a conditions object If the
   * object is in the cache and still valid (IOV), the
   * cached object will be returned.  If it is not in the cache,
   * or is out of date, the ConditionsObjectProvider::getCondition method
   * will be called to provide the object.
   *
   * @see getConditionPtr
   * @tparam T type to cast condition object to
   * @param[in] condition_name name of condition to retrieve
   * @returns const reference to conditions object
   */
  template <class T>
  const T& get(const std::string& condition_name) {
    return dynamic_cast<const T&>(*getConditionPtr(condition_name));
  }

  /**
   * Access the IOV for the given condition
   *
   * @param[in] condition_name name of condition to get IOV for
   * @returns Interval Of Validity for the input condition name
   */
  ConditionsIntervalOfValidity getConditionIOV(const std::string& condition_name) const;

  /**
   * Calls onProcessStart for all ConditionsObjectProviders
   */
  void onProcessStart();

  /**
   * Calls onProcessEnd for all ConditionsObjectProviders
   */
  void onProcessEnd();

  /**
   * Calls onNewRun for all ConditionsObjectProviders
   */
  void onNewRun(RunHeader&);

 private:
  /** Handle to the Process. */
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
