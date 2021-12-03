#include "fire/Processor.h"

// LDMX
#include "fire/Process.h"
#include "fire/RunHeader.h"

namespace fire {

Processor::Processor(const std::string &name, Process &process)
    : process_{process}, name_{name}, theLog_{logging::makeLogger(name)} {}

Conditions &Processor::getConditions() const {
  return process_.getConditions();
}

const ldmx::EventHeader &Processor::getEventHeader() const {
  return *(process_.getEventHeader());
}

void Processor::setStorageHint(fire::StorageControlHint hint,
                               const std::string &purposeString) {
  process_.getStorageController().addHint(name_, hint, purposeString);
}

int Processor::getLogFrequency() const { return process_.getLogFrequency(); }

int Processor::getRunNumber() const { return process_.getRunNumber(); }

Producer::Producer(const std::string &name, Process &process)
    : Processor(name, process) {}

Analyzer::Analyzer(const std::string &name, Process &process)
    : Processor(name, process) {}
}  // namespace fire
