#include "fire/Conditions.hpp"

#include <sstream>

#include "fire/Process.hpp"

namespace fire {

Conditions::Conditions(const config::Parameters& ps, Process& p) : process_{p} {
  auto providers{ps.get<std::vector<config::Parameters>>("providers",{})};
  for (const auto& provider : providers) {
    auto cop{ConditionsProvider::Factory::get().make(
        provider.get<std::string>("class_name"), provider)};
    std::string provides{cop->getConditionObjectName()};
    if (providers_.find(provides) != providers_.end()) {
      throw config::Parameters::Exception(
          "Multiple ConditonsObjectProviders configured to provide " +
          provides);
    }
    cop->attach(this);
    providers_[provides] = cop;
  }
}

void Conditions::onProcessStart() {
  for (auto& [_, cop] : providers_) cop->onProcessStart();
}

void Conditions::onProcessEnd() {
  for (auto& [_, cop] : providers_) cop->onProcessEnd();
}

void Conditions::onNewRun(RunHeader& rh) {
  for (auto& [_, cop] : providers_) cop->onNewRun(rh);
}

ConditionsIntervalOfValidity Conditions::getConditionIOV(
    const std::string& condition_name) const {
  auto cacheptr = cache_.find(condition_name);
  if (cacheptr == cache_.end())
    return ConditionsIntervalOfValidity();
  else
    return cacheptr->second.iov;
}

const ConditionsObject* Conditions::getConditionPtr(
    const std::string& condition_name) {
  const EventHeader& context = process_.eventHeader();
  auto cacheptr = cache_.find(condition_name);

  if (cacheptr == cache_.end()) {
    auto copptr = providers_.find(condition_name);

    if (copptr == providers_.end()) {
      throw Exception("No provider is available for : " + condition_name);
    }

    std::pair<const ConditionsObject*, ConditionsIntervalOfValidity> cond =
        copptr->second->getCondition(context);

    if (!cond.first) {
      throw Exception("Null condition returned for requested item : " +
                      condition_name);
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
      cacheptr->second.provider->release(cacheptr->second.obj);
      // now ask for a new one
      std::pair<const ConditionsObject*, ConditionsIntervalOfValidity> cond =
          cacheptr->second.provider->getCondition(context);

      if (!cond.first) {
        std::stringstream s;
        s << "Unable to update condition '" << condition_name << "' for event "
          << context.getEventNumber() << " run " << context.getRun();
        if (context.isRealData())
          s << " DATA";
        else
          s << " MC";
        throw Exception(s.str());
      }
      cacheptr->second.iov = cond.second;
      cacheptr->second.obj = cond.first;
      return cond.first;
    }
  }
}

}  // namespace fire::conditions
