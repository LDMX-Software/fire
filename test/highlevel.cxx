#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <highfive/H5Easy.hpp>

#include "fire/Process.h"
#include "fire/config/Parameters.h"

/**
 * isolation of pure testing classes
 */
namespace fire::test {

class TestAdd : public Producer {
 public:
  TestAdd(const config::Parameters& ps)
    : Producer(ps) {}
  ~TestAdd() = default;
  void produce(fire::Event& event) final override {
    int dropme = event.header().number() * 10;
    int keepme = event.header().number() * 100;
    int async  = event.header().number() * 1000;

    if (event.header().number() > 2 and event.header().number() % 2 == 0) {
      event.add("async",async);
    }

    event.add("dropme",dropme);
    event.add("keepme",keepme);
  }
};

class TestGet : public Producer {
  bool same_sequence_;
 public:
  TestGet(const config::Parameters& ps)
    : Producer(ps),
      same_sequence_{ps.get<bool>("same_sequence")} {}
  ~TestGet() = default;
  void produce(fire::Event& event) final override {
    if (same_sequence_) {
      // should still get dropme in same sequence
      BOOST_TEST(event.get<int>("dropme") == event.header().number() * 10);
    } else {
      // make sure dropme was dropped
      BOOST_TEST(not event.exists("dropme"));
    }
    
    // keepme should be here in both cases
    BOOST_TEST(event.get<int>("keepme") == event.header().number() * 100);

    if (event.header().number() > 2 and event.header().number() % 2 == 0) {
      // get valid async value in same sequence or not
      BOOST_TEST(event.get<int>("async") == event.header().number() * 1000);
    } else if (not same_sequence_) {
      // make sure async was cleared when put into output file
      BOOST_TEST(event.get<int>("async") == 0);
    }
  }
};

}

DECLARE_PROCESSOR_NS(fire::test,TestAdd);
DECLARE_PROCESSOR_NS(fire::test,TestGet);

/**
 * High level functionality testing
 *
 * - drop/keep rules with simple regex
 * - async adding
 */
BOOST_AUTO_TEST_SUITE(highlevel)

BOOST_AUTO_TEST_CASE(prod_drop_async, *boost::unit_test::depends_on("process/production_mode")) {
  std::string output{"prod_drop_async.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);
  
  // drop  objects from any pass whose name starts with 'drop' (e.g. 'dropme')
  fire::config::Parameters dk_rule;
  dk_rule.add<std::string>("regex",".*/drop.*");
  dk_rule.add("keep",false);
  configuration.add<std::vector<fire::config::Parameters>>("drop_keep_rules", {dk_rule});
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  fire::config::Parameters test_add;
  test_add.add<std::string>("name","test_add");
  test_add.add<std::string>("class_name","fire::test::TestAdd");

  fire::config::Parameters test_get;
  test_get.add<std::string>("name","test_get");
  test_get.add<std::string>("class_name","fire::test::TestGet");
  test_get.add("same_sequence", true);

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_add, test_get});
  configuration.add<fire::config::Parameters>("conditions",{});

  try {
    fire::Process p(configuration);
    p.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    BOOST_TEST(false);
  }
}

BOOST_AUTO_TEST_CASE(recon_drop_async, *boost::unit_test::depends_on("highlevel/prod_drop_async")) {
  std::string output{"recon_drop_async.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  // same name as before
  std::vector<std::string> input_files = { "prod_drop_async.h5" };
  configuration.add("input_files",input_files );
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  configuration.add("event_limit", -1);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1); // not used for recon mode
  configuration.add("max_tries", 1); // not used in recon mode

  fire::config::Parameters test_get;
  test_get.add<std::string>("name","test_get");
  test_get.add<std::string>("class_name","fire::test::TestGet");
  test_get.add("same_sequence", false);

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_get});
  configuration.add<fire::config::Parameters>("conditions",{});

  try {
    fire::Process p(configuration);
    p.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    BOOST_TEST(false);
  }
}

BOOST_AUTO_TEST_SUITE_END()
