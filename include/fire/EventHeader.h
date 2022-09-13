#ifndef FIRE_EVENTHEADER_H
#define FIRE_EVENTHEADER_H

// STL
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include "fire/version/Version.h"
#include "fire/io/Constants.h"
#include "fire/io/Data.h"
#include "fire/io/ParameterStorage.h"

#ifdef fire_USE_ROOT
#include "TObject.h"
#include "TTimeStamp.h"
namespace ldmx {
#else
namespace fire {
#endif

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
    std::time_t t = eh.time_;
    return s << "EventHeader {"
      << " number: " << eh.eventNumber_
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
  int getEventNumber() const { return eventNumber_; }

  /**
   * Return the event number.
   * @return The event number.
   */
  int number() const { return eventNumber_; }

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
  void setEventNumber(int number) { this->eventNumber_ = number; }

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
    this->time_ = std::time(nullptr);
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
    time_ = 0.;
    run_ = -1;
    eventNumber_ = -1;
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
  friend class fire::io::Data<EventHeader>;

  /**
   * attach to the serializing h5::Data wrapper
   *
   * We use h5::constants::NUMBER_NAME so that the serialization
   * method can deduce the number of events in a file using the
   * size of our eventNumber_ dataset.
   *
   * @param[in] d h5::Data to attach to
   */
  void attach(fire::io::Data<EventHeader>& d);

  /**
   * The event number.
   */
  int eventNumber_{-1};

  /**
   * The run number.
   */
  int run_{-1};

  /**
   * The event timestamp
   */
  long int time_;

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
  fire::io::ParameterStorage parameters_;

#ifdef fire_USE_ROOT
  /**
   * member used only for reading from ROOT input files
   */
  TTimeStamp timestamp_;

  /**
   * member used only for reading from ROOT input files
   */
  std::map<std::string,int> intParameters_;

  /**
   * member used only for reading from ROOT input files
   */
  std::map<std::string,float> floatParameters_;

  /**
   * member used only for reading from ROOT input files
   */
  std::map<std::string,std::string> stringParameters_;

  /**
   * ROOT class definition
   *
   * version is one more than it was in ROOT-based Framework
   */
  ClassDef(EventHeader, 4);
#endif
};

}  // namespace fire

#ifdef fire_USE_ROOT
namespace fire {

/// alias EventHeader in fire namespace
using EventHeader = ldmx::EventHeader;

namespace io {

/**
 * Data specialization for EventHeaders when ROOT reading is available
 *
 * @note Further developments of this class or the base Data class
 * need to be done with **extreme** caution.
 *
 * If fire is built without ROOT reading, then this class not necessary.
 * All of the functions excep the load(root::Reader&) function are
 * identical to the ones from the general Data class.
 *
 * When reading EventHeaders from ROOT, you will see the following error.
 * ```
 * Warning in <TStreamerInfo::Build>: ldmx::EventHeader: fire::io::ParameterStorage has no streamer or dictionary, data member "parameters_" will not be saved
 * ```
 * This can be ignored because we will not persist the EventHeader through ROOT.
 */
template<>
class Data<ldmx::EventHeader> : public AbstractData<ldmx::EventHeader> {
 public:
  /**
   * Construct a Data wrapper around the event header.
   *
   * We _require_ an input handle because the event header is supposed to
   * be handled by the Event class completely.
   *
   * @param[in] path Path to event header
   * @param[in] eh handle to event header to manipulate
   */
  explicit Data(const std::string& path, Reader* input_file, ldmx::EventHeader* eh);

  /**
   * copied from general Data class
   *
   * @param[in] r h5::Reader to load from
   */
  void load(h5::Reader& r) final override;

  /**
   * copied from general Data class
   *
   * AND THEN we translate the old containers for parameters and timestamp
   * into the new ones. This allows us to get rid of TTimeStamp
   * and evolve the parameter sets into a more user friendly structure.
   * 
   * @param[in] r root::Reader to load from
   */
  void load(root::Reader& r) final override;

  /**
   * copied from general Data class
   *
   * @param[in] w Writer to write to
   */
  void save(Writer& w) final override;

  /**
   * copied from general Data class
   * @param[in] w Writer to write to
   */
  void structure(Writer& w) final override;

  /**
   * Attach a member object from the our data handle
   *
   * We create a new child Data so that we can recursively
   * handle complex member variable types.
   *
   * @tparam MemberType type of member variable we are attaching
   * @param[in] name name of member variable
   * @param[in] m reference of member variable
   */
  template <typename MemberType>
  void attach(const std::string& name, MemberType& m) {
    members_.push_back(
        std::make_unique<Data<MemberType>>(this->path_ + "/" + name, input_file_, &m));
  }

 private:
  /// list of members in this dataset
  std::vector<std::unique_ptr<BaseData>> members_;
  /// pointer to the input file being read form (if it exists)
  Reader* input_file_;
}; // Data<ldmx::EventHeader>

} // namespace fire
} // namespace io
#endif // fire_USE_ROOT
#endif // FIRE_EVENTHEADER_H
