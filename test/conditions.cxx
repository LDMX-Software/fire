#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "fire/Conditions.h"
#include "fire/Process.h"

namespace test {

class TestCO : public fire::ConditionsObject {
 public:
  static int num_constructed;
  static int num_alive;
  TestCO() : fire::ConditionsObject("TestCO") { 
    num_constructed++; 
    num_alive++;
  }
  ~TestCO() { num_alive--; }
  int num() const { return num_constructed; }
};

int TestCO::num_constructed = 0;
int TestCO::num_alive = 0;

class TestCP : public fire::ConditionsProvider {
  static int run;
 public:
  TestCP(const fire::config::Parameters& ps) : fire::ConditionsProvider(ps) {}
  ~TestCP() = default;
  std::pair<const fire::ConditionsObject*, fire::ConditionsIntervalOfValidity>
  getCondition(const fire::EventHeader& eh) override {
    run++;
    auto co = new TestCO;
    fire::ConditionsIntervalOfValidity iov(run,run);
    return {co,iov};
  }
};

int TestCP::run = 0;

}  // namespace test

DECLARE_CONDITIONS_PROVIDER_NS(test, TestCP);

/**
 * Test basic functionality of conditions system
 */
BOOST_AUTO_TEST_SUITE(conditions)

BOOST_AUTO_TEST_SUITE(providers)
BOOST_AUTO_TEST_CASE(misspell) {
  fire::config::Parameters ps;

  // misspell class name
  BOOST_CHECK_THROW(fire::ConditionsProvider::Factory::get().make("DNE", ps),
                    fire::factory::Exception);

  // misspell a different parameter
  ps.add<std::string>("names", "test");
  BOOST_CHECK_THROW(
      fire::ConditionsProvider::Factory::get().make("test::TestCP", ps),
      fire::config::Parameters::Exception);
}

BOOST_AUTO_TEST_CASE(simple) {
  fire::config::Parameters ps;
  ps.add<std::string>("obj_name", "TestCO");
  ps.add<std::string>("tag_name", "Test");

  std::shared_ptr<fire::ConditionsProvider> cp;
  BOOST_CHECK_NO_THROW(
      cp = fire::ConditionsProvider::Factory::get().make("test::TestCP", ps));

  fire::EventHeader eh;
  {
    const auto& [co, iov] = cp->getCondition(eh);
    BOOST_TEST(co->getName() == "TestCO");
    BOOST_TEST(dynamic_cast<const test::TestCO*>(co) != nullptr);
    BOOST_TEST(test::TestCO::num_constructed == 1);
    BOOST_TEST(test::TestCO::num_alive == 1);
    cp->release(co);
    BOOST_TEST(test::TestCO::num_alive == 0);
  }

  {
    const auto& [co, iov] = cp->getCondition(eh);
    BOOST_TEST(co->getName() == "TestCO");
    BOOST_TEST(dynamic_cast<const test::TestCO*>(co) != nullptr);
    BOOST_TEST(test::TestCO::num_constructed == 2);
    BOOST_TEST(test::TestCO::num_alive == 1);
    cp->release(co);
    BOOST_TEST(test::TestCO::num_alive == 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(system) {
  std::string output{"conditions_system_output.h5"};
  fire::config::Parameters configuration;
  configuration.add<std::string>("pass_name", "test");

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  configuration.add("output_file", output_file);

  fire::config::Parameters storage;
  storage.add("default_keep", true);
  configuration.add("storage", storage);

  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);
  configuration.add("run", 3);
  configuration.add("max_tries", 1);

  // configuration.add("libraries",vec<str>);
  // configuration.add("sequence",vec<Param>);
  configuration.add("testing", true);  // ok for no sequence

  // test loading of a conditions provider parameters
  fire::config::Parameters provider;
  provider.add<std::string>("class_name", "test::TestCP");
  provider.add<std::string>("obj_name", "TestCO");
  provider.add<std::string>("tag_name", "Test");

  fire::config::Parameters conditions;
  conditions.add<std::vector<fire::config::Parameters>>("providers",
                                                        {provider});

  configuration.add("conditions", conditions);

  fire::Process p{configuration};
  fire::Conditions& c{p.conditions()};

  const auto& co = c.get<test::TestCO>("TestCO");
  BOOST_TEST(co.getName() == "TestCO");
  BOOST_TEST(test::TestCO::num_constructed == 3);
  BOOST_TEST(test::TestCO::num_alive == 1);
}

BOOST_AUTO_TEST_SUITE_END()
