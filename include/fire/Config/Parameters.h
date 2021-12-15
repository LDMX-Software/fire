#ifndef FRAMEWORK_PARAMETERS_H
#define FRAMEWORK_PARAMETERS_H

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <any>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

/*~~~~~~~~~~~~~~~*/
/*   Exception   */
/*~~~~~~~~~~~~~~~*/
#include "fire/Exception/Exception.h"

namespace fire {
namespace config {

/**
 * Class encapsulating parameters for configuring a processor.
 */
class Parameters {
 public:
  /**
   * Add a parameter to the parameter list.  If the parameter already
   * exists in the list, throw an exception.
   *
   * @param[in] name Name of the parameter.
   * @param[in] value The value of the parameter.
   * @throw Exception if a parameter by that name already exist in
   *  the list.
   */
  template <typename T>
  void add(const std::string& name, const T& value) {
    if (exists(name)) {
      EXCEPTION_RAISE("ParameterExist",
                      "The parameter " + name +
                          " already exists in the list of parameters.");
    }

    parameters_[name] = value;
  }

  /**
   * Check to see if a parameter exists
   *
   * @param[in] name name of parameter to check
   * @return true if parameter exists in configuration set
   */
  bool exists(const std::string& name) const {
    return parameters_.find(name) != parameters_.end();
  }

  /**
   * Retrieve the parameter of the given name.
   *
   * @throw Exception if parameter of the given name isn't found
   *
   * @throw Exception if parameter is found but not of the input type
   *
   * @param T the data type to cast the parameter to.
   *
   * @param[in] name the name of the parameter value to retrieve.
   *
   * @return The user specified parameter of type T.
   */
  template <typename T>
  T get(const std::string& name) const {
    // Check if the variable exists in the map.  If it doesn't,
    // raise an exception.
    if (not exists(name)) {
      EXCEPTION_RAISE(
          "NonExistParam",
          "Parameter '" + name + "' does not exist in list of parameters.");
    }

    try {
      auto parameter = std::any_cast<T>(parameters_.at(name));
      return parameter;
    } catch (const std::bad_any_cast& e) {
      EXCEPTION_RAISE("BadTypeParam",
                      "Parameter '" + name + "' of type '" +
                          parameters_.at(name).type().name() +
                          "' is being cast to incorrect type '" +
                          typeid(T).name() + "'.");
    }
  }

  /**
   * Retrieve a parameter with a default specified.
   *
   * Return the input default if a parameter is not found in map.
   *
   * @return the user parameter of type T
   */
  template <typename T>
  T get(const std::string& name, const T& def) const {
    if (not exists(name)) return def;

    // get here knowing that name exists in parameters_
    return get<T>(name);
  }

  /**
   * Get a list of the keys available.
   * This may be helpful in debugging to make sure the parameters are spelled
   * correctly.
   */
  std::vector<std::string> keys() const {
    std::vector<std::string> key;
    for (auto i : parameters_) key.push_back(i.first);
    return key;
  }

 private:
  /// Parameters
  std::map<std::string, std::any> parameters_;

};  // Parameters
}  // namespace config
}  // namespace fire

#endif  // FRAMEWORK_PARAMETERS_H
