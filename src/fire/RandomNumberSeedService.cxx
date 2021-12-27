#include "fire/RandomNumberSeedService.h"
#include <time.h>
#include "fire/EventHeader.h"
#include "fire/Process.h"
#include "fire/RunHeader.h"

namespace fire {

const std::string RandomNumberSeedService::CONDITIONS_OBJECT_NAME =
    "RandomNumberSeedService";

static const int SEED_EXTERNAL = 2;
static const int SEED_RUN = 3;
static const int SEED_TIME = 4;

void RandomNumberSeedService::stream(std::ostream& s) const {
  s << "RandomNumberSeedService(";
  if (mode_ == SEED_RUN) s << "Seed on RUN";
  if (mode_ == SEED_TIME) s << "Seed on TIME";
  if (mode_ == SEED_EXTERNAL) s << "Seeded EXTERNALLY";
  s << ") Root seed = " << root_ << std::endl;
  for (auto i : seeds_) {
    s << " " << i.first << "=>" << i.second << std::endl;
  }
}

RandomNumberSeedService::RandomNumberSeedService(const fire::config::Parameters& parameters)
    : ConditionsObject(CONDITIONS_OBJECT_NAME),
      ConditionsProvider(parameters) {
  auto seeding = parameters.get<std::string>("mode", "run");
  if (!strcasecmp(seeding.c_str(), "run")) {
    mode_ = SEED_RUN;
  } else if (!strcasecmp(seeding.c_str(), "external")) {
    mode_ = SEED_EXTERNAL;
    root_ = parameters.get<int>("root");
    initialized_ = true;
  } else if (!strcasecmp(seeding.c_str(), "time")) {
    root_ = time(0);
    mode_ = SEED_TIME;
    initialized_ = true;
  }
}

void RandomNumberSeedService::onNewRun(RunHeader& rh) {
  if (mode_ == SEED_RUN) {
    root_ = rh.getRunNumber();
    initialized_ = true;
  }
  rh.set<int>("RandomNumberRootSeed", root_);
}

uint64_t RandomNumberSeedService::getSeed(const std::string& name) const {
  uint64_t seed(0);
  std::map<std::string, uint64_t>::const_iterator i = seeds_.find(name);
  if (i == seeds_.end()) {
    // hash is sum of characters shifted by position, mod 8
    for (size_t j = 0; j < name.size(); j++)
      seed += (uint64_t(name[j]) << (j % 8));
    seed += root_;
    // break const here only to cache the seed
    seeds_[name] = seed;
  } else
    seed = i->second;
  return seed;
}

std::vector<std::string> RandomNumberSeedService::getSeedNames() const {
  std::vector<std::string> rv;
  for (auto i : seeds_) {
    rv.push_back(i.first);
  }
  return rv;
}

std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
RandomNumberSeedService::getCondition(const EventHeader& context) {
  if (!initialized_) {
    if (mode_ == SEED_RUN) {
      root_ = context.getRun();
    }
    initialized_ = true;
  }
  return std::make_pair(this, ConditionsIntervalOfValidity(true, true));
}

}  // namespace fire
DECLARE_CONDITIONS_PROVIDER_NS(fire, RandomNumberSeedService)
