/**
 * @file EventProcessor.h
 * Re-interpretation of ROOT-based framework code
 * into fire
 */
#ifndef FIRE_FRAMEWORK_EVENTPROCESSOR_H
#define FIRE_FRAMEWORK_EVENTPROCESSOR_H

#include "fire/Processor.h"

/**
 * Namespace for interop with 
 * [ROOT-based framework](https://github.com/LDMX-Software/Framework)
 * styled processors
 */
namespace framework {

/// alias config::Parameters into this namespace
namespace config {
using Parameters = fire::config::Parameters;
}

/// alias Event into this namespace
using Event = fire::Event;

/// alias Process into this namespace
using Process = fire::Process;

/**
 * Wrapper class for fire::Processor which does
 * the necessary modifications on the constructors
 * and adds the old configure method.
 */
class EventProcessor : public fire::Processor {
 public:
  EventProcessor(const std::string& name, framework::Process& p) {}
  EventProcessor(const config::Parameters& ps) 
    : fire::Processor(ps), 
      EventProcessor(ps.get<std::string>("name"),/*nullref somehow*/) {
        this->configure(ps);
  }
  virtual void configure(const config::Parameters& ps) {}
};

/// alias for old-style producers
class Producer : public EventProcessor {};

/// alias for old-style analyzers
class Analyzer : public EventProcessor {};

}

/**
 * Producer declaration macro following old input style.
 * Puts the namespace and class back together and gives
 * it to the new declaration macro.
 *
 * @param[in] NS namespace producer class is in
 * @param[in] CLASS name of producer class
 */
#define DECLARE_PRODUCER_NS(NS,CLASS) DECLARE_PROCESSOR(NS::CLASS)

/**
 * Producer declaration macro following old input style.
 *
 * @param[in] CLASS name of producer class
 */
#define DECLARE_PRODUCER(CLASS) DECLARE_PROCESSOR(CLASS)

/**
 * Analyzer declaration macro following old input style.
 * Puts the namespace and class back together and gives
 * it to the new declaration macro.
 *
 * @param[in] NS namespace analyzer class is in
 * @param[in] CLASS name of analyzer class
 */
#define DECLARE_ANALYZER_NS(NS,CLASS) DECLARE_PROCESSOR(NS::CLASS)

/**
 * Analyzer declaration macro following old input style.
 *
 * @param[in] CLASS name of analyzer class
 */
#define DECLARE_ANALYZER(CLASS) DECLARE_PROCESSOR(CLASS)

#endif  // FIRE_FRAMEWORK_EVENTPROCESSOR_H
