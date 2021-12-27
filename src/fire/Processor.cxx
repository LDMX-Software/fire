#include "fire/Processor.hpp"

#include "fire/Process.hpp"

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

Producer::Producer(const config::Parameters& ps) : Processor(ps) {}

Analyzer::Analyzer(const config::Parameters& ps) : Processor(ps) {}
}  // namespace fire
