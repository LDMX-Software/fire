#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <highfive/H5Easy.hpp>

#include "fire/Processor.h"
#include "fire/Process.h"

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

BOOST_AUTO_TEST_SUITE(misspell)

BOOST_AUTO_TEST_CASE(class_name) {
  std::string output{"shouldnt_exist.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);
  
  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);
  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  configuration.add<fire::config::Parameters>("conditions",{});
  
  // misspell class name
  fire::config::Parameters test_proc;
  test_proc.add<std::string>("name","test_proc");
  test_proc.add<std::string>("class_name","DNE");
  configuration.add<std::vector<fire::config::Parameters>>("sequence",{test_proc});
  BOOST_CHECK_THROW(std::make_unique<fire::Process>(configuration), fire::Exception);
}

BOOST_AUTO_TEST_CASE(other_parameter) {
  std::string output{"shouldnt_exist.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);
  
  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);
  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  configuration.add<fire::config::Parameters>("conditions",{});
  
  // misspell a different parameter
  fire::config::Parameters test_proc;
  test_proc.add<std::string>("name","test_proc");
  test_proc.add<std::string>("class_name","test::TestProcessor");
  test_proc.add("run_number", 420);
  configuration.add<std::vector<fire::config::Parameters>>("sequence",{test_proc});
  BOOST_CHECK_THROW(std::make_unique<fire::Process>(configuration), fire::Exception);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(mimic_process) {
  std::string output{"processors_run.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);
  
  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);
  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  configuration.add<fire::config::Parameters>("conditions",{});

  fire::config::Parameters producer, analyzer;
  producer.add<std::string>("name","producer");
  producer.add<std::string>("class_name","test::TestProcessor");
  producer.add("run", 420);

  analyzer.add<std::string>("name","analyzer");
  analyzer.add<std::string>("class_name","test::TestProcessor2");

  configuration.add<std::vector<fire::config::Parameters>>("sequence",{producer, analyzer});

  std::unique_ptr<fire::Process> p;
  try {
    p = std::make_unique<fire::Process>(configuration);
    p->run();
  } catch (const fire::Exception& e) {
    std::cerr << "ERROR [" << e.category() << "] : " << e.message() << std::endl;
    BOOST_TEST(false);
  }
}

BOOST_AUTO_TEST_CASE(throw_exceptions) {
  std::string output{"shouldnt_exist.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);
  
  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);
  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  configuration.add<fire::config::Parameters>("conditions",{});

  fire::config::Parameters th;
  th.add<std::string>("name","test_throw");
  th.add<std::string>("class_name","test::TestThrow");
  configuration.add<std::vector<fire::config::Parameters>>("sequence",{th});

  try {
    auto p = std::make_unique<fire::Process>(configuration);
    p->run();
  } catch (const fire::Exception& e) {
    BOOST_TEST(e.category() == th.get<std::string>("name"));
  }
}

BOOST_AUTO_TEST_SUITE_END()
