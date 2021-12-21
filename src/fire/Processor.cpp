#include "fire/Processor.hpp"

// LDMX
#include "fire/RunHeader.hpp"

namespace fire {

Processor::Processor(const config::Parameters &ps)
    : name_{ps.get<std::string>("name")}/*, theLog_{logging::makeLogger(name)}*/ {}

/*
Conditions &Processor::getConditions() const {
  return process_.getConditions();
}
*/

/*
void Processor::setStorageHint(fire::StorageControlHint hint,
                               const std::string &purposeString) {
  process_.getStorageController().addHint(name_, hint, purposeString);
}
*/

Producer::Producer(const config::Parameters& ps)
    : Processor(ps) {}

Analyzer::Analyzer(const config::Parameters& ps)
    : Processor(ps) {}
}  // namespace fire
