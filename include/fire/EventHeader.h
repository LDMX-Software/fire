#ifndef FIRE_EVENTHEADER_H
#define FIRE_EVENTHEADER_H

// STL
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include "fire/h5/ParameterStorage.h"

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
  int number() const { return number_; }

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
    isRealData_ = false;
    timestamp_ = 0.;
    run_ = -1;
    number_ = -1;
    parameters_.clear();
  }

  /// get a parameter
  template<typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    return parameters_.get<ParameterType>(name);
  }

  /// set a parameter
  template<typename ParameterType>
  void set(const std::string& name, const ParameterType& val) {
    parameters_.set(name,val);
  }

 private:
  /// allow data set access for reading/writing
  friend class h5::Data<EventHeader>;
  void attach(h5::Data<EventHeader>& d) {
    // make sure we use the name for this variable that the reader expects
    d.attach(h5::Reader::EVENT_HEADER_NUMBER,number_);
    d.attach("run",run_);
    d.attach("timestamp",timestamp_);
    d.attach("weight",weight_);
    d.attach("isRealData",isRealData_);
    d.attach("parameters",parameters_);
  }

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
  long int timestamp_;

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
  h5::ParameterStorage parameters_;
};

}  // namespace fire

#endif 
