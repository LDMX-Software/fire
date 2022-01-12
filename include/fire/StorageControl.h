#ifndef FIRE_STORAGECONTROL_H_
#define FIRE_STORAGECONTROL_H_

#include <regex>
#include <string>
#include <vector>

#include "fire/config/Parameters.h"

namespace fire {

/**
 * Isolation of voting system deciding if events should be kept
 *
 * Any Processor can provide a hint as to whether a given
 * event should be kept or dropped. The hint is cached in the
 * StorageControl object until the end of the event. At that
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
   * Configure the various options for how the storage contol behaves.
   *
   * @throws Exception if required parameters are not found
   * @throws Exception if one of the listening rule regex's is malformed
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
   * Add a storage hint for a given processor
   *
   * The hint needs to match at least one of the listening rules in order
   * to be considered.
   *
   * @note This means if no listing rules are provided then no storage
   * hints are considered!
   *
   * @param[in] hint The storage control hint to apply for the given event
   * @param[in] purpose A purpose string which can be used in the skim control
   * configuration
   * @param[in] processor_name Name of the event processor
   */
  void addHint(Hint hint, const std::string& purpose,
               const std::string& processor_name);

  /**
   * Determine if the current event should be kept, based on the defined rules
   *
   * @note Currently, no weighting system is implemented.
   *
   * Both "should" and "must" type of hints are given equal weighting.
   * The "keep" and "drop" votes are counted and then
   * the following logic is applied.
   * 1. If there are no votes either way, we return the default decision.
   * 2. If there are strictly more keep votes, we decide to keep.
   * 3. If there are strictly more drop votes, we decide to drop.
   * 4. If there are an equal number of votes, we return the default decision.
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
   * 1. a processor regex to match hints coming 
   *    from processors named a certain way
   * 2. a purpose regex to match hints 
   *    from all processors with a specific purpose
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
