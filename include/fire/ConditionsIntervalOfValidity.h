#ifndef FIRE_CONDITIONSINTERVALOFVALIDITY_H
#define FIRE_CONDITIONSINTERVALOFVALIDITY_H

#include <iostream>
#include "fire/EventHeader.h"

namespace fire {

/**
 * Defines the run/event/type range for which a given
 * condition is valid, including for all time
 */
class ConditionsIntervalOfValidity {
 public:
  /**
   * Null IOV
   */
  ConditionsIntervalOfValidity() noexcept;

  /**
   * Unlimited IOV for data, MC, or both.
   * @param[in] validForData true if condition is valid for real data
   * @param[in] validForMC true if condition is valid for MC data
   */
  ConditionsIntervalOfValidity(bool validForData, bool validForMC) noexcept;

  /**
   * Run-limited IOV
   * @param[in] firstRun should be -1 if valid from beginning of time
   * @param[in] lastRun should be -1 if valid to end of time
   * @param[in] validForData true if condition is valid for real data
   * @param[in] validForMC true if condition is valid for MC data
   */
  ConditionsIntervalOfValidity(int firstRun, int lastRun, bool validForData = true,
                bool validForMC = true) noexcept;

  /** 
   * Checks to see if this condition is valid for the given event using
   * information from the header 
   *
   * @param[in] eh EventHeader to check
   * @return true if our IOV includes the event described by the input header
   */
  bool validForEvent(const EventHeader& eh) const;

  /** Checks to see if this IOV overlaps with the given IOV */
  bool overlaps(const ConditionsIntervalOfValidity& iov) const;

  /**
   * Stream the IOV to an ostream in a pretty way
   *
   * The output looks like
   * ```
   * "IOV(firstRun->lastRun, type)"
   * ```
   * where `lastRun` is replaced by `infty` if it is negative
   * and `type` is `DATA` or `MC` or both.
   *
   * @param[in] s ostream to stream to
   * @param[in] iov IOV to stream out
   * @return modified ostream
   */
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
