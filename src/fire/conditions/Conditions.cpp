#include "fire/conditions/Conditions.hpp"
#include <sstream>
#include "fire/Process.hpp"

namespace fire::conditions {

void Conditions::createConditionsObjectProvider(
    const std::string& classname, const std::string& objname,
    const std::string& tagname, const fire::config::Parameters& params) {

  std::unique_ptr<ConditionsObjectProvider> cop;
  try {
    cop = ConditionsObjectProvider::Factory::get().make(
          classname, objname, tagname, params, process_);
  } catch(const Exception& e) {
    EXCEPTION_RAISE(
        "Conditions", "No ConditionsObjectProvider registered as " + classname);
  }

  std::string provides = cop->getConditionObjectName();
  if (providers_.find(provides) != providers_.end()) {
    EXCEPTION_RAISE(
        "ConditionAmbiguityException",
        "Multiple ConditonsObjectProviders configured to provide " + provides);
  }
  providers_[provides] = cop;
}

void Conditions::onProcessStart() {
  for (auto& [_,cop] : providers_) cop->onProcessStart();
}

void Conditions::onProcessEnd() {
  for (auto& [_,cop] : providers_) cop->onProcessEnd();
}

void Conditions::onNewRun(ldmx::RunHeader& rh) {
  for (auto& [_,cop] : providers_) cop->onNewRun(rh);
}

ConditionsIOV Conditions::getConditionIOV(
    const std::string& condition_name) const {
  auto cacheptr = cache_.find(condition_name);
  if (cacheptr == cache_.end())
    return ConditionsIOV();
  else
    return cacheptr->second.iov;
}

const ConditionsObject* Conditions::getConditionPtr(
    const std::string& condition_name) {
  const ldmx::EventHeader& context = *(process_.getEventHeader());
  auto cacheptr = cache_.find(condition_name);

  if (cacheptr == cache_.end()) {
    auto copptr = providers_.find(condition_name);

    if (copptr == providers_.end()) {
      EXCEPTION_RAISE(
          "ConditionUnavailable",
          std::string("No provider is available for : " + condition_name));
    }

    std::pair<const ConditionsObject*, ConditionsIOV> cond =
        copptr->second->getCondition(context);

    if (!cond.first) {
      EXCEPTION_RAISE(
          "ConditionUnavailable",
          std::string("Null condition returned for requested item : " +
                      condition_name));
    }

    // first request, create a cache entry
    CacheEntry ce;
    ce.iov = cond.second;
    ce.obj = cond.first;
    ce.provider = copptr->second;
    cache_[condition_name] = ce;
    return ce.obj;
  } else {
    /// if still valid, we return what we have
    if (cacheptr->second.iov.validForEvent(context))
      return cacheptr->second.obj;
    else {
      // if not, we release the old object
      cacheptr->second.provider->releaseConditionsObject(cacheptr->second.obj);
      // now ask for a new one
      std::pair<const ConditionsObject*, ConditionsIOV> cond =
          cacheptr->second.provider->getCondition(context);

      if (!cond.first) {
        std::stringstream s;
        s << "Unable to update condition '" << condition_name << "' for event "
          << context.getEventNumber() << " run " << context.getRun();
        if (context.isRealData())
          s << " DATA";
        else
          s << " MC";
        EXCEPTION_RAISE("ConditionUnavailable", s.str());
      }
      cacheptr->second.iov = cond.second;
      cacheptr->second.obj = cond.first;
      return cond.first;
    }
  }
}

}  // namespace fire
