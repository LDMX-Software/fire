#include "fire/Processor.hpp"

// LDMX
#include "fire/Process.hpp"
#include "fire/RunHeader.hpp"

namespace fire {

Processor::Processor(const std::string &name, Process &process)
    : process_{process}, name_{name}/*, theLog_{logging::makeLogger(name)}*/ {}

/*
Conditions &Processor::getConditions() const {
  return process_.getConditions();
}
*/

const EventHeader &Processor::getEventHeader() const {
  return process_.eventHeader();
}

/*
void Processor::setStorageHint(fire::StorageControlHint hint,
                               const std::string &purposeString) {
  process_.getStorageController().addHint(name_, hint, purposeString);
}
*/

Producer::Producer(const std::string &name, Process& p)
    : Processor(name,p) {}

Analyzer::Analyzer(const std::string &name, Process& p)
    : Processor(name,p) {}
}  // namespace fire
