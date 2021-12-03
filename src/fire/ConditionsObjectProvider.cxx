#include "fire/ConditionsObjectProvider.h"

// LDMX
#include "fire/Process.h"

namespace fire {

ConditionsObjectProvider::ConditionsObjectProvider(
    const std::string& objname, const std::string& tagname,
    const fire::config::Parameters& params, Process& process)
    : process_{process},
      objectName_{objname},
      tagname_{tagname},
      theLog_{logging::makeLogger(objname)} {}

std::pair<const ConditionsObject*, ConditionsIOV>
ConditionsObjectProvider::requestParentCondition(
    const std::string& name, const ldmx::EventHeader& context) {
  const ConditionsObject* obj = process_.getConditions().getConditionPtr(name);
  ConditionsIOV iov = process_.getConditions().getConditionIOV(name);
  return std::make_pair(obj, iov);
}

}  // namespace fire
