#ifndef FRAMEWORK_CONFIGURE_PARAMETERS_H
#define FRAMEWORK_CONFIGURE_PARAMETERS_H

#include "fire/config/Parameters.h"

namespace framework::config {
class Parameters {
  fire::config::Parameters p_;
 public:
  Parameters(const fire::config::Parameters& p) : p_{p} {}
  template <typename T>
  void addParameter(const std::string& name, const T& val) {
    p_.add(name, val);
  }

  template <typename T>
  T getParameter(const std::string& name) const {
    return p_.get<T>(name);
  }

  template <typename T>
  T getParameter(const std::string& name, const T& def) const {
    return p_.get<T>(name, def);
  }
};
}

#endif
