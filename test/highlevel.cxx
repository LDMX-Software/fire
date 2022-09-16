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

/**
 * need test class to make sure schema evolution
 * interacts well with copy
 */
class DummyInt {
 public:
  fire_class_version(1);
  int i;
  friend class fire::io::Data<DummyInt>;
  void clear() {
    i = 0.;
  }
  void attach(fire::io::Data<DummyInt>& d) {
    d.attach("i",i);
  }
  DummyInt(int _i) : i{_i} {}
  DummyInt() = default;
};  // DummyInt

class TestAdd : public Processor {
 public:
  TestAdd(const config::Parameters& ps)
    : Processor(ps) {}
  ~TestAdd() = default;
  void process(fire::Event& event) final override {
    int along  = event.header().number();
    int dropme = event.header().number() * 10;
    int keepme = event.header().number() * 100;
    int async  = event.header().number() * 1000;

    if (event.header().number() > 2 and event.header().number() % 2 == 0) {
      event.add("async",async);
    }

    // along is not access with get so we can check that it is being
    // copied to the output file in recon mode
    event.add("keepalong", along); 
    // keeplateget will only be accessed by TestGet during the last half
    // of processing, but since it should be kept, the whole dataset should
    // exist in the final output file
    event.add("keeplateget", along);
    event.add("keepanotherlateget", DummyInt(along));
    // these objects should not be around during the recon stage
    event.add("dropalong", along); 
    event.add("dropme",dropme);
    // this should be around
    event.add("keepme",keepme);
  }
};

class TestGet : public Processor {
  bool same_sequence_;
 public:
  TestGet(const config::Parameters& ps)
    : Processor(ps),
      same_sequence_{ps.get<bool>("same_sequence")} {}
  ~TestGet() = default;
  void process(fire::Event& event) final override {
    if (same_sequence_) {
      // should still get dropme in same sequence
      BOOST_TEST(event.get<int>("dropme") == event.header().number() * 10);
    } else {
      // make sure dropme and dropalong were dropped
      BOOST_TEST(not event.exists("dropme"));
      BOOST_TEST(not event.exists("dropalong"));
    }
    
    // keepme should be here in both cases
    BOOST_TEST(event.get<int>("keepme") == event.header().number() * 100);

    if (event.header().number() > 2 and event.header().number() % 2 == 0) {
      // get valid async value in same sequence or not
      BOOST_TEST(event.get<int>("async") == event.header().number() * 1000);
    } else if (not same_sequence_) {
      // make sure async was cleared when put into output file
      BOOST_TEST(event.get<int>("async") == std::numeric_limits<int>::min());
    }

    if (event.header().number() > 4) {
      BOOST_TEST(event.get<int>("keeplateget") == event.header().number());
      BOOST_TEST(event.get<DummyInt>("keepanotherlateget").i == event.header().number());
    }
  }
};

}

/**
 * Cannot use macro because two processors are
 * in the same compilation unit.
 */
namespace {
  auto v0 = ::fire::Processor::Factory::get().declare<fire::test::TestAdd>();
  auto v1 = ::fire::Processor::Factory::get().declare<fire::test::TestGet>();
}

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
  std::string output{"recon_drop_async.h5"},
              pass{"test"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",pass);

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

  // keep all objects beginning with 'keep'
  //  this should be keepme, keepalong
  fire::config::Parameters dk_rule;
  dk_rule.add<std::string>("regex",".*/keep.*");
  dk_rule.add("keep",true);
  configuration.add<std::vector<fire::config::Parameters>>("drop_keep_rules", {dk_rule});

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

  // check that output has keepme and keepalong while it does not
  //  have dropme or dropalong
  std::vector<int> correct = {
    1,2,3,4,5,6,7,8,9,10
  };
  std::vector<int> keepme_correct = {
    100,200,300,400,500,600,700,800,900,1000
  };

  H5Easy::File f(output);
  std::string pass_grp{fire::io::constants::EVENT_GROUP+"/"+pass};
  BOOST_TEST(f.exist(pass_grp+"/keepme"));
  BOOST_TEST(H5Easy::load<std::vector<int>>(f, pass_grp+"/keepme") == keepme_correct);
  BOOST_TEST(f.exist(pass_grp+"/keepalong"));
  BOOST_TEST(H5Easy::load<std::vector<int>>(f, pass_grp+"/keepalong") == correct);
  BOOST_TEST(f.exist(pass_grp+"/keeplateget"));
  BOOST_TEST(H5Easy::load<std::vector<int>>(f, pass_grp+"/keeplateget") == correct);
  BOOST_TEST(not f.exist(pass_grp+"/dropme"));
  BOOST_TEST(not f.exist(pass_grp+"/dropalong"));
}

BOOST_AUTO_TEST_SUITE_END()
