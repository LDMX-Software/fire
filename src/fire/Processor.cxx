#include "fire/Processor.h"

#include "fire/Process.h"

namespace fire {

Processor::Processor(const config::Parameters& ps)
    : theLog_{logging::makeLogger(ps.get<std::string>("name"))},
      name_{ps.get<std::string>("name")} {}

void Processor::setStorageHint(StorageControl::Hint hint,
                               const std::string& purpose) const {
  assert(process_);
  process_->addStorageControlHint(hint, purpose, name_);
}

Conditions &Processor::getConditions() const {
  assert(process_);
  return process_->conditions();
}

}  // namespace fire
