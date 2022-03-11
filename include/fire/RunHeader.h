#ifndef FIRE_RUNHEADER_H
#define FIRE_RUNHEADER_H

#include <map>
#include <string>

#include "fire/version/Version.h"
#include "fire/io/Constants.h"
#include "fire/io/Data.h"
#include "fire/io/ParameterStorage.h"

#ifdef fire_USE_ROOT
#include "TObject.h"
namespace ldmx {
#else
namespace fire {
#endif

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
   * Start the run. Provide the run number and record the timestamp.
   *
   * @param[in] run the run number
   */
  void runStart(const int run) { 
    runStart_ = std::time(nullptr);
    runNumber_ = run;
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
    runNumber_ = -1;
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
  friend class fire::io::Data<RunHeader>;

  /**
   * Attach to our fire::io::Data
   *
   * We attach all of our parameters.
   *
   * @param[in] d fire::io::Data to attach to
   */
  void attach(fire::io::Data<RunHeader>& d);

 private:
  /** Run number. */
  int runNumber_{0};

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
  std::string softwareTag_{fire::version::GIT_SHA1};

  /// run parameteres
  fire::io::ParameterStorage parameters_;

#ifdef fire_USE_ROOT
  /**
   * member only used for reading ROOT files
   */
  std::map<std::string,int> intParameters_;

  /**
   * member only used for reading ROOT files
   */
  std::map<std::string,float> floatParameters_;

  /**
   * member only used for reading ROOT files
   */
  std::map<std::string,std::string> stringParameters_;
  /**
   * ROOT class definition
   *
   * version is one more than what it was in the latest ROOT-based Framework
   */
  ClassDef(RunHeader, 5);
#endif
};  // RunHeader
}  // namespace fire/ldmx

#ifdef fire_USE_ROOT
namespace fire {

/// alias RunHeader into the fire namespace
using RunHeader = ldmx::RunHeader;

namespace io {

/**
 * Data specialization for RunHeaders when ROOT reading is available
 *
 * @note Further developments of this class or the base Data class
 * need to be done with **extreme** caution.
 *
 * If fire is built without ROOT reading, then this class not necessary.
 * All of the functions excep the load(root::Reader&) function are
 * identical to the ones from the general Data class.
 *
 * When reading RunHeaders from ROOT, you will see the following error.
 * ```
 * Warning in <TStreamerInfo::Build>: ldmx::RunHeader: fire::io::ParameterStorage has no streamer or dictionary, data member "parameters_" will not be saved
 * ```
 * This can be ignored because we will not persist the RunHeader through ROOT.
 */
template<>
class Data<ldmx::RunHeader> : public AbstractData<ldmx::RunHeader> {
 public:
  /**
   * Construct a Data wrapper around the event header.
   *
   * @param[in] path Path to run header
   * @param[in] rh handle to run header to manipulate
   */
  explicit Data(const std::string& path, ldmx::RunHeader* rh = nullptr);

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
   * into the new ones. This allows us to evolve the parameter sets into a 
   * more user friendly structure.
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
        std::make_unique<Data<MemberType>>(this->path_ + "/" + name, &m));
  }

 private:
  /// list of members in this dataset
  std::vector<std::unique_ptr<BaseData>> members_;
};  // Data<ldmx::RunHeader>

}  // namespace io
}  // namespace fire
#endif // fire_USE_ROOT
#endif // FIRE_RUNHEADER_H
