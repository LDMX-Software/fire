/**
 * @file RandomNumberSeedService.h
 * @brief Conditions object for random number seeds
 * @author Jeremy Mans, University of Minnesota
 */

#ifndef FIRE_RANDOMNUMBERSEEDSERVICE_H_ 
#define FIRE_RANDOMNUMBERSEEDSERVICE_H_ 

#include "fire/ConditionsObject.h"
#include "fire/ConditionsObjectProvider.h"

namespace fire {

/**
 * @class RandomNumberSeedService
 * System for consistent seeding of random number generators
 *
 * The system can be configured in a number of different ways.  In general,
 * seeds are constructed based on a root seed.  That root seed can be
 * constructed in a number of ways, chosen in the python configuration. (a) The
 * root seed can be the run number of the first run observed by the service
 * ("Run") (b) The root seed can be based on the current time ("Time") (c) The
 * root seed can be provided in the python configuration ("External")
 *
 * Individual seeds are then constructed using the root seed and a simple hash
 * based on the name of the seed. Seeds can also be specified in the python
 * file, in which case no autoseeding will be performed.
 */
class RandomNumberSeedService : public ConditionsObject,
                                public ConditionsObjectProvider {
 public:
  /// Conditions object name
  static const std::string CONDITIONS_OBJECT_NAME;

  /**
   * Create the random number seed service conditions object
   *
   * This is where we decide what seed mode we will be running in.
   *
   * @TODO allow loading of hand-provided seeds from python
   *
   * @param[in] name the name of this provider
   * @param[in] tagname the name of the tag generation of this condition
   * @param[in] parameters configuration parameters from python
   * @param[in] process reference to the running process object
   */
  RandomNumberSeedService(const fire::config::Parameters& parameters);

  /**
   * Configure the seed service when a new run starts
   *
   * If we are using the run number as the root seed,
   * then we get the run number and set the root seed to it.
   *
   * No matter what, we put the root seed into the RunHeader
   * to be persisted into the output file.
   *
   * @param[in,out] header RunHeader for the new run that is starting
   */
  virtual void onNewRun(RunHeader& header);

  /**
   * Access a given seed by name
   *
   * Checks the cache for the input name.
   * If the input name is not in the cache,
   * it generates the seed for that name by combining
   * the root seed with a hash of the name.
   *
   * @param[in] name Name of seed
   * @return seed derived from root seed using the input name
   */
  uint64_t getSeed(const std::string& name) const;

  /**
   * Get a list of all the known seeds
   *
   * @returns vector of seed names that have already been requested (in cache)
   */
  std::vector<std::string> getSeedNames() const;

  /**
   * Access the root seed
   *
   * @returns root seed that is used to derive all other seeds
   */
  uint64_t getRootSeed() const { return rootSeed_; }

  /**
   * Get the seed service as a conditions object
   *
   * This object is both the provider of the seed service and the conditions
   * object itself, so after checking if it has been initialized, we return a
   * refernce to ourselves with the unlimited interval of validity.
   *
   * @param[in] context EventHeader for the event context
   * @returns reference to ourselves and unlimited interval of validity
   */
  virtual std::pair<const ConditionsObject*, ConditionsIOV> getCondition(
      const EventHeader& context) final override;

  /**
   * This object is both the provider of the seed service and the conditions
   * object itself, so it does nothing when asked to release the object.
   *
   * @param[in] co ConditionsObject to release, unused
   */
  virtual void release(const ConditionsObject* co) final override {}

  /**
   * Stream the configuration of this object to the input ostream
   *
   * @param[in,out] s output stream to print this object to
   */
  void stream(std::ostream& s) const;

  /**
   * Output streaming operator
   *
   * @see fire::RandomNumberSeedService::stream
   * @param[in] s output stream to print to
   * @param[in] o seed service to print
   * @returns modified output stream
   */
  friend std::ostream& operator<<(std::ostream& s,
                                  const RandomNumberSeedService& o) {
    o.stream(s);
    return s;
  }

 private:
  /// whether the root seed has been initialized
  bool initialized_{false};

  /// what mode of root seed are we using
  int mode_{0};

  /// what the root seed actually is
  uint64_t root_{0};

  /// cache of seeds by name
  mutable std::map<std::string, uint64_t> seeds_;
};

}  // namespace fire

#endif  // FRAMEWORK_RANDOMNUMBERSEEDSERVICE_H_
