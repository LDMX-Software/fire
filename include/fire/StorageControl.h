#ifndef FIRE_STORAGECONTROL_H_
#define FIRE_STORAGECONTROL_H_

#include <regex>
#include <string>
#include <vector>

#include "fire/config/Parameters.hpp"

namespace fire {

/**
 * @class StorageControl
 * @brief Class which encapsulates storage control functionality, used by the
 * Process class
 *
 * Any Processor can provide a hint as to whether a given
 * event should be kept or dropped.  The hint is cached in the
 * StorageControl object until the end of the event.  At that
 * point, the process queries the StorageControl to determine if
 * the event should be stored in the output file.
 */
class StorageControl {
 public:
  /**
   * Hints that can be provided by processors to the storage controller
   *
   * Integer values of the hints are currently not used for anything,
   * although one could imagine a "weighting" system being implemented
   * where different Hints are weighted based on how "strong" the hint is.
   */
  enum class Hint {
    NoOpinion = 0,
    Undefined = -1,
    ShouldKeep = 1,
    MustKeep = 10,
    ShouldDrop = 2,
    MustDrop = 20
  };  // enum Hint

 public:
  /**
   * Constructor
   * Configure the various options for how the storage contol behaves.
   * @throws config::Exception if required parameters are not found
   * @throws regex exception if malformed regex
   *
   * @param[in] ps Parameters used to configure storage controller
   */
  StorageControl(const config::Parameters& ps);

  /**
   * Reset the event-by-event state by removing any hints
   * provided by processors during the previous event.
   */
  void resetEventState();

  /**
   * Add a storage hint for a given module
   *
   * The hint needs to match at least one of the listening rules in order
   * to be considered.
   *
   * @note This means if no listing rules are provided then no storage
   * hints are considered!
   *
   * @param hint The storage control hint to apply for the given event
   * @param purpose A purpose string which can be used in the skim control
   * configuration
   * @param processor_name Name of the event processor
   */
  void addHint(Hint hint, const std::string& purpose,
               const std::string& processor_name);

  /**
   * Determine if the current event should be kept, based on the defined rules
   *
   * @returns true if we should store the current event into the output file
   */
  bool keepEvent() const;

 private:
  /**
   * Default state for storage control
   */
  bool default_keep_{true};

  /**
   * Collection of rules allowing certain processors
   * or purposes to be considered ("listened to") during
   * the storage decision.
   *
   * Each rule has two entries:
   * 1. a processor regex to match hints coming from processors named a certain
   * way
   * 2. a purpose regex to match hints form all processors with a specific
   * purpose
   */
  std::vector<std::pair<std::regex, std::regex>> rules_;

 private:
  /**
   * Collection of hints from the event processors
   */
  std::vector<Hint> hints_;
};
}  // namespace fire

#endif
