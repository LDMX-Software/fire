#ifndef FIRE_RUNHEADER_HPP
#define FIRE_RUNHEADER_HPP

#include <map>
#include <string>

//#include "fire/Version.hpp"
#include "fire/h5/DataSet.hpp"

namespace fire {

class RunHeader {
 public:
  static const std::string NAME;

  /** @return The run number. */
  int getRunNumber() const { return runNumber_; }

  /** @return The name of the detector used to create the events. */
  const std::string &getDetectorName() const { return detectorName_; }

  /** Set the name of the detector that was used in this run */
  void setDetectorName(const std::string &det) { detectorName_ = det; }

  /**
   * @return The git SHA-1 associated with the software tag used
   * to generate this file.
   */
  const std::string &getSoftwareTag() const { return softwareTag_; }

  /** @return A short description of the run. */
  const std::string &getDescription() const { return description_; }

  /** Set the description of this run */
  void setDescription(const std::string &des) { description_ = des; }

  /**
   * Get the start time of the run in seconds since epoch.
   *
   * @return The start time of the run.
   *
   */
  int getRunStart() const { return runStart_; }

  /**
   * Start the run. Provide the run number and recorde the timestamp.
   *
   * @param[in] run the run number
   */
  void setRunStart(const int run) { 
    runStart_ = std::time(nullptr);
    number_ = run;
  }

  /**
   * Get the end time of the run in seconds since epoch.
   *
   * @return The end time of the run.
   */
  int getRunEnd() const { return runEnd_; }

  /**
   * Set the end time of the run in seconds since epoch
   *
   * @param[in] runEnd the end time of the run.
   */
  void setRunEnd(const int runEnd) { runEnd_ = runEnd; }

  /**
   * Get an int parameter value.
   *
   * @param name The name of the parameter.
   * @return The parameter value.
   */
  int getIntParameter(const std::string &name) const {
    return intParameters_.at(name);
  }

  /// Get a const reference to all int parameters
  const std::map<std::string, int> &getIntParameters() const {
    return intParameters_;
  }

  /**
   * Set an int parameter value.
   *
   * @param name The name of the parameter.
   * @param value The value of the parameter.
   */
  void setIntParameter(const std::string &name, int value) {
    intParameters_[name] = value;
  }

  /**
   * Get a float parameter value.
   *
   * @param name The name of the parameter.
   * @return value The parameter value.
   */
  float getFloatParameter(const std::string &name) const {
    return floatParameters_.at(name);
  }

  /// Get a const reference to all float parameters
  const std::map<std::string, float> &getFloatParameters() const {
    return floatParameters_;
  }

  /**
   * Set a float parameter value.
   *
   * @param name The name of the parameter.
   * @param value The parameter value.
   */
  void setFloatParameter(const std::string &name, float value) {
    floatParameters_[name] = value;
  }

  /**
   * Get a string parameter value.
   *
   * @param name The name of the parameter.
   * @return value The parameter value.
   */
  std::string getStringParameter(const std::string &name) const {
    return stringParameters_.at(name);
  }

  /// Get a const reference to all string parameters
  const std::map<std::string, std::string> &getStringParameters() const {
    return stringParameters_;
  }

  /**
   * Set a string parameter value.
   *
   * @param name The name of the parameter.
   * @param value The parameter value.
   */
  void setStringParameter(const std::string &name, std::string value) {
    stringParameters_[name] = value;
  }

  /**
   * Stream this object into the input ostream
   *
   * Includes new-line characters to separate out the different parameter maps
   *
   * @param[in] s ostream to write to
   */
  void stream(std::ostream &s) const;

  /** Print a string desciption of this object. */
  void Print() const;

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

  /**
   * Stream this object to an output stream
   *
   * Needs to be here and labeled as friend for
   * it to be compatible with Boost logging.
   *
   * @see ldmx::RunHeader::stream
   * @param[in] s ostream to write to
   * @param[in] h RunHeader to write out
   * @return modified ostream
   */
  friend std::ostream &operator<<(std::ostream &s, const RunHeader &h) {
    h.stream(s);
    return s;
  }

 private:
  friend class h5::DataSet<RunHeader>;
  void attach(h5::DataSet<RunHeader>& set) {
    set.attach("number",runNumber_);
    set.attach("start",runStart_);
    set.attach("end",runEnd_);
  }

 private:
  /** Run number. */
  int number_{0};

  /** Detector name. */
  std::string detectorName_{""};

  /** Run description. */
  std::string description_{""};

  /// Run start in seconds since epoch
  int runStart_{0};

  /// Run end in seconds since epoch
  int runEnd_{0};

  /**
   * git SHA-1 hash associated with the software tag used to generate
   * this file.
   */
  std::string softwareTag_{"NOTSET"};

  /** Map of int parameters. */
  std::map<std::string, int> intParameters_;

  /** Map of float parameters. */
  std::map<std::string, float> floatParameters_;

  /** Map of string parameters. */
  std::map<std::string, std::string> stringParameters_;

};  // RunHeader

namespace h5 {
/**
 * DataSet specialization of EventHeader
 */
template<>
class DataSet<RunHeader> : public AbstractDataSet<RunHeader> {
 public:
  DataSet(RunHeader* handle);
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
