#ifndef FIRE_CONDITIONS_INTERVALOFVALIDITY_HPP
#define FIRE_CONDITIONS_INTERVALOFVALIDITY_HPP

/*~~~~~~~~~~~*/
/*   Event   */
/*~~~~~~~~~~~*/
#include <iostream>
#include "fire/exception/Exception.hpp"
#include "fire/EventHeader.hpp"

namespace fire {
namespace conditions {

/**
 * @class IntervalOfValidity
 *
 * @brief Class which defines the run/event/type range for which a given
 * condition is valid, including for all time
 */
class IntervalOfValidity {
 public:
  /**
   * Constructor for null validity
   */
  IntervalOfValidity() noexcept
      : firstRun_{0}, lastRun_{0}, validForData_{false}, validForMC_{false} {}

  /**
   * Constructor for a unlimited validity
   */
  IntervalOfValidity(bool validForData, bool validForMC) noexcept
      : firstRun_(-1),
        lastRun_(-1),
        validForData_{validForData},
        validForMC_{validForMC} {}

  /**
   * Constructor for a run-limited validity
   * @arg firstRun should be -1 if valid from beginning of time
   * @arg lastRun should be -1 if valid to end of time
   */
  IntervalOfValidity(int firstRun, int lastRun, bool validForData = true,
                bool validForMC = true) noexcept
      : firstRun_(firstRun),
        lastRun_(lastRun),
        validForData_{validForData},
        validForMC_{validForMC} {}

  /** Checks to see if this condition is valid for the given event using
   * information from the header */
  inline bool validForEvent(const EventHeader& eh) const {
    return (eh.getRun() >= firstRun_ || firstRun_ == -1) &&
         (eh.getRun() <= lastRun_ || lastRun_ == -1) &&
         ((eh.isRealData()) ? (validForData_) : (validForMC_));
  }

  /** Checks to see if this IOV overlaps with the given IOV */
  inline bool overlaps(const IntervalOfValidity& iov) const {
    if (iov.validForData_ != validForData_ && iov.validForMC_ != validForMC_)
      return false;
    if (iov.firstRun_ < lastRun_) return false;  // starts after this IOV
    if (iov.lastRun_ < firstRun_) return false;  // ends before this IOV
    return true;
  }

  friend std::ostream& operator<<(std::ostream& s, const IntervalOfValidity& iov) {
    s << "IOV(" << firstRun_ << "->";
    if (lastRun_ < 0)
      s << "infty";
    else
      s << lastRun_;
    if (validForData_) s << ", DATA";
    if (validForMC_) s << ", MC";
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
}  // namespace conditions
}  // namespace fire

#endif  // FRAMEWORK_CONDITIONSIOV_H_
