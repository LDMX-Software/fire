#ifndef FRAMEWORK_CONDITIONSOBJECTPROVIDER_H
#define FRAMEWORK_CONDITIONSOBJECTPROVIDER_H

#include "fire/ConditionsProvider.h"
#include "Framework/Configure/Parameters.h"

namespace framework {
using ConditionsIOV = fire::ConditionsIntervalOfValidity;
using Process = fire::Process;

class ConditionsObjectProvider : public fire::ConditionsProvider {
  static fire::config::Parameters wrap(const std::string& name, const std::string& tagname,
              const framework::config::Parameters& parameters) {
    fire::config::Parameters wrapped = parameters;
    wrapped.add("obj_name", name);
    wrapped.add("tag_name", tagname);
    return wrapped;
  }
 public:
  ConditionsObjectProvider(const std::string& name, const std::string& tagname,
                           const framework::config::Parameters& parameters,
                           framework::Process&)
    : fire::ConditionsProvider(wrap(name, tagname, parameters)) {}
};

#define DECLARE_CONDITIONS_PROVIDER_NS(NS,CLASS) DECLARE_CONDITIONS_PROVIDER(NS::CLASS)

}

#endif
