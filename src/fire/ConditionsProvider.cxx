#include "fire/ConditionsProvider.h"
#include "fire/Conditions.h"

namespace fire {

ConditionsProvider::ConditionsProvider(const fire::config::Parameters& ps)
    : objectName_{ps.get<std::string>("obj_name")},
      tagname_{ps.get<std::string>("tag_name")},
      theLog_{logging::makeLogger(ps.get<std::string>("obj_name"))} {}

std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
ConditionsProvider::requestParentCondition(const std::string& name,
                                           const EventHeader& context) {
  assert(conditions_);
  return std::make_pair(conditions_->getConditionPtr(name), conditions_->getConditionIOV(name));
}

}  // namespace fire
