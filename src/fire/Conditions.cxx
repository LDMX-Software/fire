#include "fire/Conditions.h"

#include <sstream>

#include "fire/Process.h"

namespace fire {

Conditions::Conditions(const config::Parameters& ps, Process& p) : process_{p} {
  auto providers{ps.get<std::vector<config::Parameters>>("providers",{})};
  for (const auto& provider : providers) {
    auto cp{ConditionsProvider::Factory::get().make(
        provider.get<std::string>("class_name"), provider)};
    std::string provides{cp->getConditionObjectName()};
    if (providers_.find(provides) != providers_.end()) {
      throw config::Parameters::Exception(
          "Multiple ConditonsObjectProviders configured to provide " +
          provides);
    }
    cp->attach(this);
    providers_[provides] = cp;
  }
}

void Conditions::onProcessStart() {
  for (auto& [_, cp] : providers_) cp->onProcessStart();
}

void Conditions::onProcessEnd() {
  for (auto& [_, cp] : providers_) cp->onProcessEnd();
}

void Conditions::onNewRun(RunHeader& rh) {
  for (auto& [_, cp] : providers_) cp->onNewRun(rh);
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
    auto cpptr = providers_.find(condition_name);

    if (cpptr == providers_.end()) {
      throw Exception("No provider is available for : " + condition_name);
    }

    const auto& [co, iov] = cpptr->second->getCondition(context);
    if (!co) {
      throw Exception("Null condition returned for requested item : " +
                      condition_name);
    }

    // first request, create a cache entry
    CacheEntry ce;
    ce.iov = iov;
    ce.obj = co;
    ce.provider = cpptr->second;
    cache_[condition_name] = ce;
    return co;
  } else {
    /// if still valid, we return what we have
    if (cacheptr->second.iov.validForEvent(context))
      return cacheptr->second.obj;
    else {
      // if not, we release the old object
      cacheptr->second.provider->release(cacheptr->second.obj);
      // now ask for a new one
      const auto& [co, iov] = cacheptr->second.provider->getCondition(context);

      if (!co) {
        std::stringstream s;
        s << "Unable to update condition '" << condition_name << "' for event "
          << context.getEventNumber() << " run " << context.getRun();
        if (context.isRealData())
          s << " DATA";
        else
          s << " MC";
        throw Exception(s.str());
      }
      cacheptr->second.iov = iov;
      cacheptr->second.obj = co;
      return co;
    }
  }
}

}  // namespace fire::conditions
