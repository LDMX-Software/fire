#include "fire/conditions/Provider.hpp"

// LDMX
#include "fire/Process.hpp"

namespace fire {

std::pair<const ConditionsObject*, ConditionsIOV>
ConditionsObjectProvider::requestParentCondition(
    const std::string& name, const ldmx::EventHeader& context) {
  const ConditionsObject* obj = process_.getConditions().getConditionPtr(name);
  ConditionsIOV iov = process_.getConditions().getConditionIOV(name);
  return std::make_pair(obj, iov);
}

}  // namespace fire
