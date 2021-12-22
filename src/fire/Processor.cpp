#include "fire/Processor.hpp"
#include "fire/Process.hpp"

namespace fire {

Processor::Processor(const config::Parameters &ps)
    : name_{ps.get<std::string>("name")}/*, theLog_{logging::makeLogger(name)}*/ {}

/*
Conditions &Processor::getConditions() const {
  return process_.getConditions();
}
*/


void Processor::setStorageHint(StorageControl::Hint hint,
                               const std::string &purpose) {
  assert(process_);
  process_->addStorageControlHint(hint, purpose, name_);
}

Producer::Producer(const config::Parameters& ps)
    : Processor(ps) {}

Analyzer::Analyzer(const config::Parameters& ps)
    : Processor(ps) {}
}  // namespace fire
