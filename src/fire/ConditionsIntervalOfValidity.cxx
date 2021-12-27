#include "fire/ConditionsIntervalOfValidity.h"

namespace fire {

ConditionsIntervalOfValidity::ConditionsIntervalOfValidity() noexcept
    : firstRun_{0}, lastRun_{0}, validForData_{false}, validForMC_{false} {}

ConditionsIntervalOfValidity::ConditionsIntervalOfValidity(
    bool validForData, bool validForMC) noexcept
    : firstRun_(-1),
      lastRun_(-1),
      validForData_{validForData},
      validForMC_{validForMC} {}

ConditionsIntervalOfValidity::ConditionsIntervalOfValidity(
    int firstRun, int lastRun, bool validForData,
    bool validForMC) noexcept
    : firstRun_(firstRun),
      lastRun_(lastRun),
      validForData_{validForData},
      validForMC_{validForMC} {}

bool ConditionsIntervalOfValidity::validForEvent(const EventHeader& eh) const {
  return (eh.getRun() >= firstRun_ || firstRun_ == -1) &&
       (eh.getRun() <= lastRun_ || lastRun_ == -1) &&
       ((eh.isRealData()) ? (validForData_) : (validForMC_));
}

bool ConditionsIntervalOfValidity::overlaps(const ConditionsIntervalOfValidity& iov) const {
  if (iov.validForData_ != validForData_ && iov.validForMC_ != validForMC_)
    return false;
  if (iov.firstRun_ < lastRun_) return false;  // starts after this IOV
  if (iov.lastRun_ < firstRun_) return false;  // ends before this IOV
  return true;
}
}  // namespace fire
