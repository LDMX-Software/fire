/**
 * @file EventHeader.h
 * @brief Class that provides header information about an event such as event
 * number and timestamp
 * @author Jeremy McCormick, SLAC National Accelerator Laboratory
 */

#ifndef EVENT_EVENTHEADER_H_
#define EVENT_EVENTHEADER_H_

// STL
#include <ctime>
#include <iostream>
#include <map>
#include <string>

namespace ldmx {

/**
 * @class EventHeader
 * @brief Provides header information an event such as event number and
 * timestamp
 */
class EventHeader {
 public:
  /**
   * Name of EventHeader branch
   */
  static const std::string NAME;

  friend std::ostream& operator<<(std::ostream& s, const EventHeader& eh) {
    return s << "EventHeader {"
      << " number: " << number_
      << ", run: " << run_
      << ", weight: " << weight_
      << ", " << isRealData_ ? "DATA" : "MC"
      << ", timestamp: " << std::asctime(std::localtime(&timestamp_))
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

  /**
   * Get an int parameter value.
   * @param name The name of the parameter.
   * @return The parameter value.
   */
  int getIntParameter(const std::string& name) { return intParameters_[name]; }

  /**
   * Set an int parameter value.
   * @param name The name of the parameter.
   * @param value The value of the parameter.
   * @return The parameter value.
   */
  void setIntParameter(const std::string& name, int value) {
    intParameters_[name] = value;
  }

  /**
   * Get a float parameter value.
   * @param name The name of the parameter.
   * @return value The parameter value.
   */
  float getFloatParameter(const std::string& name) {
    return floatParameters_[name];
  }

  /**
   * Set a float parameter value.
   * @param name The name of the parameter.
   * @return value The parameter value.
   */
  void setFloatParameter(const std::string& name, float value) {
    floatParameters_[name] = value;
  }

  /**
   * Get a string parameter value.
   * @param name The name of the parameter.
   * @return value The parameter value.
   */
  std::string getStringParameter(const std::string& name) {
    return stringParameters_[name];
  }

  /**
   * Set a string parameter value.
   * @param name The name of the parameter.
   * @return value The parameter value.
   */
  void setStringParameter(const std::string& name, std::string value) {
    stringParameters_[name] = value;
  }

 protected:
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
  std::time_t timestamp_;

  /**
   * The event weight.
   */
  double weight_{1.0};

  /**
   * Is this event real data?
   */
  bool isRealData_{false};

  /**
   * The int parameters.
   */
  std::map<std::string, int> intParameters_;

  /**
   * The float parameters.
   */
  std::map<std::string, float> floatParameters_;

  /**
   * The string parameters.
   */
  std::map<std::string, std::string> stringParameters_;

  /**
   * ROOT class definition.
   */
  ClassDef(EventHeader, 1);
};

}  // namespace ldmx

#endif /* EVENT_EVENTHEADER_H_ */
