#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <highfive/H5Easy.hpp>

#include "fire/Processor.h"

namespace test {

class TestProcessor : public fire::Processor {
 public:
  TestProcessor(const fire::config::Parameters& ps)
    : fire::Processor(ps) {
      run_header_ = ps.get<int>("run");
    }
  void process(fire::Event& event) final override {
    auto ievent{event.header().getEventNumber()};
    event.add("TestProcessor", ievent);
  }
 private:
  int run_header_;
};

class TestProcessor2 : public fire::Processor {
 public:
  TestProcessor2(const fire::config::Parameters& ps)
    : fire::Processor(ps) {}
  void process(fire::Event& event) final override {
    BOOST_TEST(event.header().getEventNumber() == event.get<int>("TestProcessor"));
  }
};

class TestThrow : public fire::Processor {
 public:
  TestThrow(const fire::config::Parameters& ps)
    : fire::Processor(ps) {}
  void onProcessStart() final override {
    fatalError("test throw");
  }
  void onProcessEnd() final override {
    fatalError("test throw");
  }
  void process(fire::Event& event) final override {
    fatalError("test throw");
  }
};

}

/**
 * We can't use the macro here because then the anonymous
 * variables would conflict since they are in the same 
 * compilation unit.
 */
namespace {
  auto v0 = ::fire::Processor::Factory::get().declare<test::TestProcessor>();
  auto v1 = ::fire::Processor::Factory::get().declare<test::TestProcessor2>();
  auto v2 = ::fire::Processor::Factory::get().declare<test::TestThrow>();
}

/**
 * Test basic functionality of processors
 */
BOOST_AUTO_TEST_SUITE(processor)

BOOST_AUTO_TEST_CASE(misspell) {
  fire::config::Parameters ps;
  
  // misspell class name
  BOOST_CHECK_THROW(fire::Processor::Factory::get().make("DNE",ps), fire::Exception);

  // misspell a different parameter
  ps.add("run_number", 420);
  BOOST_CHECK_THROW(fire::Processor::Factory::get().make("test::TestProcessor",ps), fire::Exception);
}

BOOST_AUTO_TEST_CASE(mimic_process) {
  fire::config::Parameters producer, analyzer;
  producer.add<std::string>("name","producer");
  producer.add("run", 420);

  analyzer.add<std::string>("name","analyzer");

  fire::config::Parameters output_file;
  output_file.add<std::string>("name", "processor.h5");
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  fire::io::Writer of{10,output_file};

  fire::Event event{fire::Event::test(of)};

  std::vector<std::unique_ptr<fire::Processor>> sequence;
  sequence.emplace_back(fire::Processor::Factory::get().make("test::TestProcessor",producer));
  sequence.emplace_back(fire::Processor::Factory::get().make("test::TestProcessor2",analyzer));

  for (auto& proc : sequence) proc->process(event);
}

BOOST_AUTO_TEST_CASE(throw_exceptions) {
  fire::config::Parameters analyzer;
  analyzer.add<std::string>("name","analyzer");

  auto p{fire::Processor::Factory::get().make("test::TestThrow",analyzer)};
  BOOST_CHECK_THROW(p->onProcessStart(), fire::Exception);
  BOOST_CHECK_THROW(p->onProcessEnd(), fire::Exception);

  fire::config::Parameters output_file;
  output_file.add<std::string>("name", "processor.h5");
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  fire::io::Writer of{10,output_file};

  fire::Event event{fire::Event::test(of)};
  BOOST_REQUIRE_THROW(p->process(event), fire::Exception);

  // check that exception has correct name
  try {
    p->process(event);
  } catch (const fire::Exception& e) {
    BOOST_TEST(e.category() == analyzer.get<std::string>("name"));
  }
}

BOOST_AUTO_TEST_SUITE_END()
