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
class Event {
  fire::Event& event_;
  Event(fire::Event& e) : event_{e} {}
  template<typename T>
  void add(const std::string& n, const T& o) {
    event_.add(n,o);
  }
  template<typename T>
  const T& getObject(const std::string& n, const std::string& p = "") {
    return event_.get<T>(n,p);
  }
  template<typename T>
  const std::vector<T>& getCollection(const std::string& n, const std::string& p = "") {
    return getObject<std::vector<T>>(n,p);
  }
  template<typename K, typename V>
  const std::map<K,V>& getCollection(const std::string& n, const std::string& p = "") {
    return getObject<std::map<K,V>>(n,p);
  }
};

/// alias Process into this namespace
using Process = fire::Process;

/**
 * Wrapper class for fire::Processor which does
 * the necessary modifications on the constructors
 * and adds the old configure method.
 */
class EventProcessor : public fire::Processor {
  static fire::config::Parameters minimal_parameter_set(const std::string& name) {
    fire::config::Parameters ps;
    ps.add("name",name);
    return ps;
  }
 public:
  EventProcessor(const std::string& name, framework::Process& p) 
    : fire::Processor(minimal_parameter_set(name)) {
      this->attach(&p);
    }
  virtual void configure(const config::Parameters& ps) {}
};

/// alias for old-style producers
class Producer : public EventProcessor {
 public:
  Producer(const std::string& name, framework::Process& p)
    : EventProcessor(name,p) {}
  virtual void produce(framework::Event &event) = 0;
  virtual void process(fire::Event &event) final override {
    this->produce(framework::Event(event));
  }
};

/// alias for old-style analyzers
class Analyzer : public EventProcessor {
 public:
  Producer(const std::string& name, framework::Process& p)
    : EventProcessor(name,p) {}
  virtual void beforeNewRun(fire::RunHeader&) final override {}
  virtual void analyze(const framework::Event &event) = 0;
  virtual void process(fire::Event &event) final override {
    this->analyze(framework::Event(event));
  }
};

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
