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
 *
 * These "legacy" processors are written with some small behavioral differences.
 * We decoreate them with deprecation warnings to try to encourage users towards
 * using the newer style of processors.
 */
namespace framework {

/**
 * config namespace alias within framework
 */
namespace config {

/**
 * alias for Parameters within this namespace
 */
using Parameters = fire::config::Parameters;
}

/**
 * Wrapper Event in this namespace reintroducing legacy functionality
 *
 * This class is a light wrapper class merely providing function handles
 * for the legacy interface with the event bus.
 *
 * @deprecated This class is meant to be used only in the transitory phase
 * where legacy processors are still supported. This class will be dropped
 * when legacy processors are dropped.
 */
class Event {
  /// reference to current event bus
  fire::Event& event_;
 public:
  /**
   * wrap the current event with a legacy interface
   */
  Event(fire::Event& e) : event_{e} {}

  /**
   * Get the event number
   * @return event number
   */
  int getEventNumber() const {
    return event_.header().number();
  }

  /**
   * Get the event weight
   * @return event weight
   */
  double getEventWeight() const {
    return event_.header().weight();
  }

  /**
   * Add an object to the event bus
   * @see fire::Event::add
   */
  template<typename T>
  void add(const std::string& n, const T& o) {
    event_.add(n,o);
  }

  /**
   * Retrieve an object from the event bus
   * @see fire::Event::get
   */
  template<typename T>
  const T& getObject(const std::string& n, const std::string& p = "") const {
    return event_.get<T>(n,p);
  }

  /**
   * Retrieve an object from the event bus
   * @see fire::Event::get
   */
  template<typename T>
  const std::vector<T>& getCollection(const std::string& n, const std::string& p = "") const {
    return getObject<std::vector<T>>(n,p);
  }

  /**
   * Retrieve an object from the event bus
   * @see fire::Event::get
   */
  template<typename K, typename V>
  const std::map<K,V>& getCollection(const std::string& n, const std::string& p = "") const {
    return getObject<std::map<K,V>>(n,p);
  }
};

/// alias Process into this namespace
using Process = fire::Process;

/**
 * Wrapper class for fire::Processor which does
 * the necessary modifications on the constructors
 * and adds the old configure method.
 *
 * @deprecated This class is meant to be used only in the transitory phase
 * where legacy processors are still supported. This class will be dropped
 * when legacy processors are dropped.
 */
class EventProcessor : public fire::Processor {
  /**
   * Construct the minimal parameter set for a fire::Processor.
   * 
   * This just involves putting the name parameter into the set of Parameters
   * under the key `name`.
   *
   * @param[in] name Name of this processor
   * @return minimal parameter set `{ 'name' : name }`
   */
  static fire::config::Parameters minimal_parameter_set(const std::string& name) {
    fire::config::Parameters ps;
    ps.add("name",name);
    return ps;
  }
 public:
  /**
   * Construct a legacy event processor
   *
   * @see minimal_parameter_set for creating the parameter set to be passed
   * to fire::Processor
   * @see fire::Processor::attach for providing the current handle to the Process
   *
   * @param[in] name Name of this processor
   * @param[in] p handle to current process
   */
  EventProcessor(const std::string& name, framework::Process& p) 
    : fire::Processor(minimal_parameter_set(name)) {
      this->attach(&p);
    }

  /// pass on pure virtual process function
  virtual void process(fire::Event &event) = 0;

  /**
   * Legacy configure method
   *
   * This method is here solely to support legacy processors
   *
   * @param[in] ps Parameter set to configure the processor with
   */
  virtual void configure(const config::Parameters& ps) {}
};

/**
 * Legacy producer class
 *
 * @deprecated This class is meant to be used only in the transitory phase
 * where legacy processors are still supported. This class will be dropped
 * when legacy processors are dropped.
 */
class Producer : public EventProcessor {
 public:
  /**
   * Pass construction to base legacy processor
   * @param[in] name Name of this processor
   * @param[in] p handle to current process
   */
  Producer(const std::string& name, framework::Process& p)
    : EventProcessor(name,p) {}

  /**
   * Pure virtual produce function to align with legacy implementation
   * @param[in] event legacy event bus for legacy processor
   */
  virtual void produce(framework::Event &event) = 0;

  /**
   * Final implementation of pure virtual process method, wrapping
   * event in framework::Event and giving it to produce method.
   *
   * @param[in] event current event bus for legacy processor
   */
  virtual void process(fire::Event &event) final override {
    framework::Event e(event);
    this->produce(e);
  }
};

/**
 * Legacy analyzer class
 *
 * @note Since histogram files have been removed from fire,
 * using legacy analyzers have very minimal capabilities.
 * If you are attempting to create an Ntuple, it is suggested
 * to simply use a framework::Producer (or a fire::Processor) and
 * add the branches of your Ntuple to the event bus instead of using
 * the (non-existent) NtupleManager.
 *
 * @deprecated This class is meant to be used only in the transitory phase
 * where legacy processors are still supported. This class will be dropped
 * when legacy processors are dropped.
 */
class Analyzer : public EventProcessor {
 public:
  /**
   * Pass construction to base legacy processor
   * @param[in] name Name of this processor
   * @param[in] p handle to current process
   */
  Analyzer(const std::string& name, framework::Process& p)
    : EventProcessor(name,p) {}
  /**
   * Don't allow legacy analyzers to use this method
   * by defining the `final` version to be empty.
   */
  virtual void beforeNewRun(fire::RunHeader&) final override {}

  /**
   * Pure virtual analyze function to align with legacy implementation
   * @param[in] event legacy event bus for legacy processor
   */
  virtual void analyze(const framework::Event &event) = 0;

  /**
   * Final implementation of pure virtual process method, wrapping
   * event in framework::Event and giving it to analyze method.
   *
   * @param[in] event current event bus for legacy processor
   */
  virtual void process(fire::Event &event) final override {
    framework::Event e(event);
    this->analyze(e);
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
