#ifndef FIRE_RUNHEADER_H
#define FIRE_RUNHEADER_H

#include <map>
#include <string>

#include "fire/version/Version.h"
#include "fire/h5/Data.h"
#include "fire/h5/ParameterStorage.h"
#include "fire/h5/Constants.h"

namespace fire {

/**
 * Container for run parameters
 *
 * There are several parameters that we hold for all runs
 * and then we also hold a more dynamic ParameterStorage object
 * for holding parameters that users wish to add within
 * Processor::beforeNewRun.
 */
class RunHeader {
 public:
  /// the name of the data holding the run headers
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
   * Start the run. Provide the run number and record the timestamp.
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

  /**
   * Get a run parameter
   *
   * @tparam ParameterType type of parameter to retrieve
   * @param[in] name name of parameter to retrieve
   * @return value of parameter for this run
   */
  template <typename ParameterType>
  const ParameterType& get(const std::string& name) const {
    return parameters_.get<ParameterType>(name);
  }

  /**
   * Set a run parameter
   * 
   * @tparam ParameterType type of parameter to set
   * @param[in] name name of parameter to set
   * @param[in] val value of parameter to set for this run
   */
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
   * clear the run header
   */
  void clear() {
    number_ = -1;
    runStart_ = 0;
    runEnd_ = 0;
    detectorName_.clear();
    description_.clear();
    softwareTag_.clear();
    parameters_.clear();
  }

  /**
   * Stream this object to an output stream
   *
   * Needs to be here and labeled as friend for
   * it to be compatible with Boost logging.
   *
   * @see RunHeader::stream
   * @param[in] s ostream to write to
   * @param[in] h RunHeader to write out
   * @return modified ostream
   */
  friend std::ostream &operator<<(std::ostream &s, const RunHeader &h) {
    h.stream(s);
    return s;
  }

 private:
  /// friends with the h5::Data that can read/write us
  friend class h5::Data<RunHeader>;

  /**
   * Attach to our h5::Data
   *
   * We attach all of our parameters.
   *
   * @param[in] d h5::Data to attach to
   */
  void attach(h5::Data<RunHeader>& d) {
    d.attach(h5::constants::NUMBER_NAME,number_);
    d.attach("start",runStart_);
    d.attach("end",runEnd_);
    d.attach("detectorName",detectorName_);
    d.attach("description",description_);
    d.attach("softwareTag",softwareTag_);
    d.attach("parameters",parameters_);
  }

 private:
  /** Run number. */
  int number_{0};

  /** Detector name. */
  std::string detectorName_{""};

  /** Run description. */
  std::string description_{""};

  /// Run start in seconds since epoch
  long int runStart_{0};

  /// Run end in seconds since epoch
  long int runEnd_{0};

  /**
   * git SHA-1 hash associated with the software tag used to generate
   * this file.
   */
  std::string softwareTag_{version::GIT_SHA1};

  /// run parameteres
  h5::ParameterStorage parameters_;

};  // RunHeader
}  // namespace fire 

#endif
