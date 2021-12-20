#ifndef FIRE_RUNHEADER_HPP
#define FIRE_RUNHEADER_HPP

#include <map>
#include <string>

//#include "fire/Version.hpp"
#include "fire/h5/DataSet.hpp"
#include "fire/h5/ParameterStorage.hpp"

namespace fire {

class RunHeader {
 public:
  static const std::string NAME;

  /** @return The run number. */
  int getRunNumber() const { return number_; }

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
  void runStart(const int run) { 
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
   */
  void runEnd() { 
    runEnd_ = std::time(nullptr); 
  }

  template <typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    return parameters_.get<ParameterType>(name);
  }

  template <typename ParameterType>
  void set(const std::string& name, const ParameterType& val) {
    parameters_.set(name,val);
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
    set.attach("number",number_);
    set.attach("start",runStart_);
    set.attach("end",runEnd_);
    set.attach("detectorName",detectorName_);
    set.attach("description",description_);
    set.attach("softwareTag",softwareTag_);
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

  /// run parameteres
  h5::ParameterStorage parameters_;

};  // RunHeader
}  // namespace fire 

#endif
