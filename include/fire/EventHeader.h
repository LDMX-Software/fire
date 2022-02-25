#ifndef FIRE_EVENTHEADER_H
#define FIRE_EVENTHEADER_H

// STL
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include "fire/io/Constants.h"
#include "fire/io/ParameterStorage.h"

namespace fire {

/**
 * Header information of an event such as event number and timestamp
 */
class EventHeader {
 public:
  /**
   * Name of EventHeader branch
   *
   * Defined in fire::h5 so that the serialization method
   * knows where to look for the number of events in a file.
   */
  static const std::string NAME;

  /**
   * Stream an event header
   *
   * Streaming the header only involves printing the information
   * we know exists for all event headers (i.e. everything not
   * in ParameterStorage).
   *
   * @param[in] s ostream to write to
   * @param[in] eh header to stream out
   * @return modified ostream
   */
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
   * Return the event number.
   * @return The event number.
   */
  int number() const { return number_; }

  /**
   * Return the run number.
   * @return The run number.
   */
  int getRun() const { return run_; }

  /**
   * Return the run number.
   * @return The run number.
   */
  int run() const { return run_; }

  /**
   * Get the event weight (default of 1.0).
   * @return The event weight.
   */
  double getWeight() const { return weight_; }

  /**
   * Get the event weight (default of 1.0).
   * @return The event weight.
   */
  double weight() const { return weight_; }

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

  /**
   * get a parameter from storage
   * @see h5::ParameterStorage::get
   * @tparam ParameterType type of parameter
   * @param[in] name parameter name
   * @return parameter value
   */
  template<typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    return parameters_.get<ParameterType>(name);
  }

  /**
   * set a parameter in storage
   * @see h5::ParameterStorage::set
   * @tparam ParameterType type of parameter
   * @param[in] name parameter name
   * @param[in] val parameter value
   */
  template<typename ParameterType>
  void set(const std::string& name, const ParameterType& val) {
    parameters_.set(name,val);
  }

 private:
  /// allow data set access for reading/writing
  friend class io::Data<EventHeader>;
  /**
   * attach to the serializing h5::Data wrapper
   *
   * We use h5::constants::NUMBER_NAME so that the serialization
   * method can deduce the number of events in a file using the
   * size of our number_ dataset.
   *
   * @param[in] d h5::Data to attach to
   */
  void attach(io::Data<EventHeader>& d) {
    // make sure we use the name for this variable that the reader expects
    d.attach(io::constants::NUMBER_NAME,number_);
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
  io::ParameterStorage parameters_;
};

}  // namespace fire

#endif 
