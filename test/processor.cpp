#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <highfive/H5Easy.hpp>

#include "fire/Processor.hpp"

namespace test {

class TestProducer : public fire::Producer {
 public:
  TestProducer(const fire::config::Parameters& ps)
    : fire::Producer(ps) {
      run_header_ = ps.get<int>("run");
    }
  void produce(fire::Event& event) {
    auto ievent{event.header().getEventNumber()};
    event.add("TestProducer", ievent);
  }
 private:
  int run_header_;
};

class TestAnalyzer : public fire::Analyzer {
 public:
  TestAnalyzer(const fire::config::Parameters& ps)
    : fire::Analyzer(ps) {}
  void analyze(const fire::Event& event) {
    BOOST_TEST(event.header().getEventNumber() == event.get<int>("TestProducer"));
  }
};

class TestThrow : public fire::Analyzer {
 public:
  TestThrow(const fire::config::Parameters& ps)
    : fire::Analyzer(ps) {}
  void onProcessStart() final override {
    fatalError("test throw");
  }
  void onProcessEnd() final override {
    fatalError("test throw");
  }
  void analyze(const fire::Event& event) final override {
    fatalError("test throw");
  }
};

}

DECLARE_PROCESSOR_NS(test,TestProducer);
DECLARE_PROCESSOR_NS(test,TestAnalyzer);
DECLARE_PROCESSOR_NS(test,TestThrow);

/**
 * Test basic functionality of processors
 */
BOOST_AUTO_TEST_SUITE(processor)

BOOST_AUTO_TEST_CASE(misspell) {
  fire::config::Parameters ps;
  
  // misspell class name
  BOOST_CHECK_THROW(fire::Processor::Factory::get().make("DNE",ps), fire::factory::Exception);

  // misspell a different parameter
  ps.add("run_number", 420);
  BOOST_CHECK_THROW(fire::Processor::Factory::get().make("test::TestProducer",ps), fire::config::Parameters::Exception);
}

BOOST_AUTO_TEST_CASE(mimic_process) {
  fire::config::Parameters producer, analyzer;
  producer.add<std::string>("name","producer");
  producer.add("run", 420);

  analyzer.add<std::string>("name","analyzer");

  fire::Event event{fire::Event::test()};

  std::vector<std::unique_ptr<fire::Processor>> sequence;
  sequence.emplace_back(fire::Processor::Factory::get().make("test::TestProducer",producer));
  sequence.emplace_back(fire::Processor::Factory::get().make("test::TestAnalyzer",analyzer));

  for (auto& proc : sequence) proc->process(event);
}

BOOST_AUTO_TEST_CASE(throw_exceptions) {
  fire::config::Parameters analyzer;
  analyzer.add<std::string>("name","analyzer");

  auto p{fire::Processor::Factory::get().make("test::TestThrow",analyzer)};
  BOOST_CHECK_THROW(p->onProcessStart(), fire::Processor::Exception);
  BOOST_CHECK_THROW(p->onProcessEnd(), fire::Processor::Exception);

  fire::Event event{fire::Event::test()};
  BOOST_REQUIRE_THROW(p->process(event), fire::Processor::Exception);

  // check that exception has correct name
  try {
    p->process(event);
  } catch (const fire::Processor::Exception& e) {
    BOOST_TEST(e.name() == analyzer.get<std::string>("name"));
  }
}

BOOST_AUTO_TEST_SUITE_END()
