#ifndef FIRE_CONDITIONSINTERVALOFVALIDITY_HPP
#define FIRE_CONDITIONSINTERVALOFVALIDITY_HPP

/*~~~~~~~~~~~*/
/*   Event   */
/*~~~~~~~~~~~*/
#include <iostream>
#include "fire/EventHeader.hpp"

namespace fire {

/**
 * @class ConditionsIntervalOfValidity
 *
 * @brief Class which defines the run/event/type range for which a given
 * condition is valid, including for all time
 */
class ConditionsIntervalOfValidity {
 public:
  /**
   * Constructor for null validity
   */
  ConditionsIntervalOfValidity() noexcept;

  /**
   * Constructor for a unlimited validity
   */
  ConditionsIntervalOfValidity(bool validForData, bool validForMC) noexcept;

  /**
   * Constructor for a run-limited validity
   * @arg firstRun should be -1 if valid from beginning of time
   * @arg lastRun should be -1 if valid to end of time
   */
  ConditionsIntervalOfValidity(int firstRun, int lastRun, bool validForData = true,
                bool validForMC = true) noexcept;

  /** Checks to see if this condition is valid for the given event using
   * information from the header */
  bool validForEvent(const EventHeader& eh) const;

  /** Checks to see if this IOV overlaps with the given IOV */
  bool overlaps(const ConditionsIntervalOfValidity& iov) const;

  friend std::ostream& operator<<(std::ostream& s, const ConditionsIntervalOfValidity& iov) {
    s << "IOV(" << iov.firstRun_ << "->";
    if (iov.lastRun_ < 0)
      s << "infty";
    else
      s << iov.lastRun_;
    if (iov.validForData_) s << ", DATA";
    if (iov.validForMC_) s << ", MC";
    s << ")";
    return s;
  }

 private:
  /** First run for which this condition is valid */
  int firstRun_;

  /** Last run for which this condition is valid or -1 for infinite validity */
  int lastRun_;

  /** Is this Condition valid for real data? */
  bool validForData_;

  /** Is this Condition valid for simulation? */
  bool validForMC_;
};
}  // namespace fire

#endif
