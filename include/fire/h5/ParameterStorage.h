#ifndef FIRE_PARAMETERSTORAGE_H
#define FIRE_PARAMETERSTORAGE_H

// STL
#include <boost/core/demangle.hpp>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include "fire/h5/Data.h"
#include "fire/h5/Reader.h"
#include "fire/exception/Exception.h"

namespace fire::h5 {

/**
 * @class ParameterStorage
 * @brief Provides dynamic parameter storage by interfacing between a map
 * to variants storing parameters and a specialized Data
 */
class ParameterStorage {
 public:
  /// get a parameter
  template <typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    try {
      return std::get<ParameterType>(parameters_.at(name));
    } catch (const std::bad_variant_access&) {
      throw Exception("BadType",
          "Parameter named " + name + " is not type " +
          boost::core::demangle(typeid(ParameterType).name()));
    } catch (const std::out_of_range&) {
      throw Exception("NotFound","Parameter named " + name + " not found.");
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

  /**
   * clear the parameters
   *
   * We don't clear the container of parameters, we clear them individually
   * by setting them to the numeric_limits minimum or clearing the std::string.
   */
  void clear();

 private:
  /// allow data set access for reading/writing
  friend class Data<ParameterStorage>;

  /**
   * three types of parameters are allowed: int, float, string
   */
  std::unordered_map<std::string, std::variant<int, float, std::string>>
      parameters_;
};

/**
 * Data specialization for ParameterStorage
 */
template <>
class Data<ParameterStorage> : public AbstractData<ParameterStorage> {
 public:
  Data(const std::string& path, ParameterStorage* handle);
  /**
   * @note This load mechanic does not support changing pass names.
   * This limits us to only using this type of dataset in the event
   * and run headers.
   */
  void load(Reader& r) final override;
  void save(Writer& w) final override;

 private:
  template <typename ParameterType>
  void attach(const std::string& name) {
    parameters_[name] = std::make_unique<Data<ParameterType>>(
        this->path_ + "/" + name, 
        std::get_if<ParameterType>(&(this->handle_->parameters_[name])));
  }

 private:
  /// the dynamic parameter listing (parallel to parameters_ member variable)
  std::unordered_map<std::string, std::unique_ptr<BaseData>> parameters_;
};

}  // namespace fire::h5

#endif
