#ifndef FIRE_PARAMETERSTORAGE_H
#define FIRE_PARAMETERSTORAGE_H

// STL
#include <boost/core/demangle.hpp>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include "fire/h5/DataSet.h"
#include "fire/h5/Reader.h"

namespace fire::h5 {

/**
 * @class ParameterStorage
 * @brief Provides dynamic parameter storage by interfacing between a map
 * to variants storing parameters and a specialized DataSet
 */
class ParameterStorage {
 public:
  /// get a parameter
  template <typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    try {
      return std::get<ParameterType>(parameters_.at(name));
    } catch (const std::bad_variant_access&) {
      throw h5::Exception(
          "Parameter named " + name + " is not type " +
          boost::core::demangle(typeid(ParameterType).name()));
    } catch (const std::out_of_range&) {
      throw h5::Exception("Parameter named " + name + " not found.");
    }
  }

  /// set a parameter
  template <typename ParameterType>
  void set(const std::string& name, const ParameterType& val) {
    static_assert(
        std::is_same_v<ParameterType, int> ||
            std::is_same_v<ParameterType, float> ||
            std::is_same_v<ParameterType, std::string>,
        "Parameters are only allowed to be float, int, or std::string.");
    parameters_[name] = val;
  }

 private:
  /// allow data set access for reading/writing
  friend class DataSet<ParameterStorage>;

  /**
   * three types of parameters are allowed: int, float, string
   */
  std::unordered_map<std::string, std::variant<int, float, std::string>>
      parameters_;
};

/**
 * DataSet specialization for ParameterStorage
 */
template <>
class DataSet<ParameterStorage> : public AbstractDataSet<ParameterStorage> {
 public:
  DataSet(const std::string& name, bool should_save, ParameterStorage* handle);
  void load(Reader& r, long unsigned int i);
  void save(Writer& w, long unsigned int i);

 private:
  template <typename ParameterType>
  void attach(const std::string& name) {
    parameters_[name] = std::make_unique<DataSet<ParameterType>>(
        this->name_ + "/" + name, true,
        std::get_if<ParameterType>(&(this->handle_->parameters_[name])));
  }

 private:
  /// the dynamic parameter listing (parallel to parameters_ member variable)
  std::unordered_map<std::string, std::unique_ptr<BaseDataSet>> parameters_;
};

}  // namespace fire::h5

#endif