#ifndef FIRE_IO_H5_PARAMETERSTORAGE_H
#define FIRE_IO_H5_PARAMETERSTORAGE_H

// STL
#include <boost/core/demangle.hpp>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include "fire/io/Data.h"
#include "fire/io/Reader.h"
#include "fire/exception/Exception.h"

namespace fire::io {

/**
 * Provides dynamic parameter storage by interfacing between a map
 * to variants storing parameters and a h5::Data secialization.
 *
 * This class does some pretty absurd nonsense in order to be able to
 * store a dynamic set of parameters of various types. The type casting
 * necessary to load and save the data stored within this object 
 * does negatively impact performance, so this class should be used
 * sparingly. Moreover, the h5::Data<ParameterStorage>::load
 * mechanic only reads in new parameters into the map from disk
 * on the first load call (it continues to load values). This
 * prevents ParameterStorage from being usable by normal event
 * objects whose data set names may change between input files
 * due to changing pass names.
 *
 * Currently, it is only used within the fire::EventHeader and
 * the fire::RunHeader and users of fire are discouraged from
 * using it within their own classes.
 *
 * ParameterStorage only supports three atomic types (int,
 * float, and std::string). This is just to keep the code relatively
 * simple. 
 */
class ParameterStorage {
 public:
  /**
   * Get a parameter corresponding to the input name.
   *
   * @throws Exception if requested type differs from the type stored
   * @throws Exception if parameter is not found
   *
   * ### usage
   * It is recomended to use auto to avoid code repitition.
   * ```cpp
   * // ps is a ParameterStorage instance that already has "one" in it.
   * auto one = ps.get<float>("one");
   * ```
   *
   * @tparam ParameterType type of parameter that is being requested
   * @param[in] name name of parameter to get
   * @return const reference to parameter named name
   */
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

  /**
   * Set a parameter to be a specific value
   *
   * @note We do not check if another parameter is being
   * overwritten.
   *
   * ### usage
   * With C++17's argument type deduction feature,
   * the template parameter can be left out.
   * ```cpp
   * ParameterStorage ps;
   * // these two are the same
   * ps.set("one",1.0);
   * ps.set<float>("one",1.0);
   * // this will not compile because double's aren't supported
   * ps.set<double>("one",1.0);
   * ```
   *
   * @tparam The type of the parameter that is being set.
   * @param[in] name Name of the parameter to set
   * @param[in] val value of the parameter to keep in storage
   */
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
   *
   * This is to help minimize the save/load type deduction processes that
   * are necessary within h5::Data<ParameterStorage>.
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
 * h5::Data specialization for ParameterStorage
 *
 * We use HDF5's introspection capability to determine the types
 * of parameters when loading and we use the std::visit method
 * in combination with decltype to determine parameter types when
 * saving. This is a delicate procedure and so should be used with
 * care. Since ParameterStorage makes no attempt to keep parameters
 * of a specific name a specific type, the user is excepted to
 * put in checks that subsequent ParameterStorage::set calls with
 * the same parameter name input the same type.
 */
template <>
class Data<ParameterStorage> : public AbstractData<ParameterStorage> {
 public:
  /**
   * Define the full in-file path to the data set and provide the
   * address of a ParameterStorage object already controled.
   *
   * We do not provide the usual `nullptr` default for the handle
   * because we want to remove the possiblity of users of fire
   * being able to add and get ParameterStorage objects directly.
   *
   * @param[in] path full in-file path to the data set
   * @param[in] handle address of ParameterStorage object to save/load
   */
  Data(const std::string& path, ParameterStorage* handle);

  /**
   * load the next entry of ParameterStorage from disk into memory
   *
   * On the first call to load, we determine the parameters in
   * this object by using our path member to list the objects
   * in the group (h5::Reader::list). Then for each member of this
   * list, we get its type (h5::Reader::getDataSetType) from its
   * path and create a new object in the variant map of the
   * ParameterStorage pointed to by our handle. Then we use
   * the attach method to create a new dataset to track the
   * parameter.
   *
   * After the first load, we just call load on all of our attached
   * datasets like any other user class.
   *
   * @note This load mechanic does not support changing pass names.
   * This limits us to only using this type of dataset in the event
   * and run headers.
   *
   * @param[in] r h5::Reader to load from
   */
  void load(h5::Reader& r) final override;

#ifdef USE_ROOT
  void load(root::Reader& r) final override {
  }
#endif

  /**
   * save the current entry of ParameterStorage into the file
   *
   * We go through all entries in the variant map held by
   * the object pointed to by our handle. If the entry is
   * not in our own map of data sets, we use std::vist
   * to deduce the parameter type and use attach to create
   * a new dataset for it. Then (even if the entry already
   * has its own dataset), we pass the writer onto the
   * individula entry dataset to save.
   *
   * @param[in] w h5::Writer to save to
   */
  void save(Writer& w) final override;

 private:
  /**
   * Attach the input parameter name as a new dataset in our own parameter map
   * using the entry in the handle's parameter variable map as the reference.
   *
   * This is very similar to the standard attachment mechanism used by
   * the general h5::Data class. We do need to specialize it in order to handle
   * the oddness of std::variant.
   *
   * @note Undefined behavior if the input ParameterType does not match
   * the type of parameter stored in the variant in the handle's parameter
   * map.
   *
   * @tparam ParameterType type of parameter to attach to us
   * @param[in] name of parameter to attach that already exists in the
   *  handle's parameter map
   */
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

}

#endif
