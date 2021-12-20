#ifndef FIRE_EVENTHEADER_HPP
#define FIRE_EVENTHEADER_HPP

// STL
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include <boost/core/demangle.hpp>

#include "fire/h5/DataSet.hpp"
#include "fire/h5/Reader.hpp"

namespace fire {

/**
 * @class EventHeader
 * @brief Provides header information an event such as event number and
 * timestamp
 */
class EventHeader {
 public:
  /**
   * Name of EventHeader branch
   *  Defined in Reader so that it knows where to look for the number of events in a file.
   */
  static const std::string NAME;

  friend std::ostream& operator<<(std::ostream& s, const EventHeader& eh) {
    std::string_view label{eh.isRealData_ ? "DATA" : "MC"};
    std::time_t t = eh.timestamp_;
    return s << "EventHeader {"
      << " number: " << eh.number_
      << ", run: " << eh.run_
      << ", weight: " << eh.weight_
      << ", " <<  label
      << ", timestamp: " << std::asctime(std::localtime(&t))
      << " }";
  }

  /**
   * Return the event number.
   * @return The event number.
   */
  int getEventNumber() const { return number_; }

  /**
   * Return the run number.
   * @return The run number.
   */
  int getRun() const { return run_; }

  /**
   * Get the event weight (default of 1.0).
   * @return The event weight.
   */
  double getWeight() const { return weight_; }

  /**
   * Is this a real data event?
   * @return True if this is a real data event.
   */
  bool isRealData() const { return isRealData_; }

  /**
   * Set the event number.
   * @param number The event number.
   */
  void setEventNumber(int number) { this->number_ = number; }

  /**
   * Set the run number.
   * @param run The run number.
   */
  void setRun(int run) { this->run_ = run; }

  /**
   * Set the timestamp.
   * @param timestamp The timestamp.
   */
  void setTimestamp() {
    this->timestamp_ = std::time(nullptr);
  }

  /**
   * Set the event weight.
   * @param weight The event weight.
   */
  void setWeight(double weight) { this->weight_ = weight; }

  /// clear the event header, required by serialization technique
  void clear() {
    weight_ = 1.;
  }

  /// get a parameter
  template<typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    try {
      return std::get<ParameterType>(parameters_.at(name));
    } catch(const std::bad_variant_access&) {
      throw std::runtime_error("Event parameter named "+name+" is not type "+boost::core::demangle(typeid(ParameterType).name()));
    } catch(const std::out_of_range&) {
      throw std::runtime_error("Event parameter named "+name+" not found.");
    }
  }

  /// set a parameter
  template<typename ParameterType>
  void set(const std::string& name, const ParameterType& val) {
    static_assert(
        std::is_same_v<ParameterType,int> ||
        std::is_same_v<ParameterType,float> ||
        std::is_same_v<ParameterType,std::string>,
        "EventHeader parameters are only allowed to be float, int, or std::string.");
    parameters_[name] = val;
  }

 private:
  /// allow data set access for reading/writing
  friend class h5::DataSet<EventHeader>;

  /**
   * The event number.
   */
  int number_{-1};

  /**
   * The run number.
   */
  int run_{-1};

  /**
   * The event timestamp
   */
  int timestamp_;

  /**
   * The event weight.
   */
  double weight_{1.0};

  /**
   * Is this event real data?
   */
  bool isRealData_{false};

  /**
   * Event parameters
   *  three types of parameters are allowed: int, float, string
   */
  std::unordered_map<std::string, std::variant<int,float,std::string>> parameters_;
};

namespace h5 {

/**
 * DataSet specialization of EventHeader
 */
template<>
class DataSet<EventHeader> : public AbstractDataSet<EventHeader> {
 public:
  DataSet(EventHeader* handle);
  void load(Reader& r, long unsigned int i);
  void save(Writer& w, long unsigned int i);
 private:
  /// the hard list of members that are definitely load/saved
  std::vector<std::unique_ptr<BaseDataSet>> members_;
  /// the dynamic parameter listing (parallel to parameters_ member variable)
  std::unordered_map<std::string, std::unique_ptr<BaseDataSet>> parameters_;
};

}

}  // namespace fire

#endif 
