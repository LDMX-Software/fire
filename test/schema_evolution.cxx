#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <highfive/H5Easy.hpp>

#include "fire/io/Data.h"
#include "fire/Process.h"
#include "fire/config/Parameters.h"

namespace fire::test::schema_evolution {
namespace v1 {
class Double {
 public:
  fire_class_version(1);
  double d_;
  friend class fire::io::Data<Double>;
  void clear() {
    d_ = 0.;
  }
  void attach(fire::io::Data<Double>& d) {
    d.attach("dv1",d_);
  }
  Double(double d) : d_{d} {}
  Double() = default;
};  // Double
} // namespace v1
namespace v2 {
class Double {
 public:
  fire_class_version(2);
  double d_;
  friend class fire::io::Data<Double>;
  void clear() {
    d_ = 0.;
  }
  void attach(fire::io::Data<Double>& d) {
    if (d.version() < 2) d.rename("dv1","dv2",d_);
    else {
      d.attach("dv2", d_);
      // reset version number?
    }
  }
  Double(double d) : d_{d} {}
  Double() = default;
};  // Double
}  // namespace v2

class Add : public Processor {
  int version;
 public:
  Add(const config::Parameters& ps)
    : Processor(ps), version{ps.get<int>("version")} {}
  ~Add() = default;
  void process(fire::Event& event) final override {
    if (version == 1) {
      event.add("foo",v1::Double(event.header().number()));
    } else {
      event.add("foo",v2::Double(event.header().number()));
    }
  }
};

class Get : public Processor {
 public:
  Get(const config::Parameters& ps)
    : Processor(ps) {}
  ~Get() = default;
  void process(fire::Event& event) final override {
    auto d{event.get<v2::Double>("foo")};
    BOOST_TEST(d.d_ == double(event.header().number()));
  }
};

}  // namespace fire::test::schema_evolution

namespace {
  auto a = ::fire::Processor::Factory::get().declare<fire::test::schema_evolution::Add>();
  auto g = ::fire::Processor::Factory::get().declare<fire::test::schema_evolution::Get>();
}

int readVersion(H5Easy::File& f, const std::string& path) {
  int v;
  f.getGroup(path).getAttribute(fire::io::constants::VERS_ATTR_NAME).read(v);
  return v;
}

/**
 * Make sure schema structure is working as intended
 */
BOOST_AUTO_TEST_SUITE(schema_evolution)

BOOST_AUTO_TEST_CASE(write_v1) {
  std::string output{"schema_evolution_prodv1.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  configuration.add("event_limit", 3);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  fire::config::Parameters test_add;
  test_add.add<std::string>("name","test_add");
  test_add.add<std::string>("class_name","fire::test::schema_evolution::Add");
  test_add.add<int>("version",1);

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_add});
  configuration.add<fire::config::Parameters>("conditions", {});

  fire::Process p(configuration);
  p.run();

  H5Easy::File f{output};
  BOOST_TEST(f.exist("/events/test/foo/dv1"));
  BOOST_TEST(readVersion(f, "/events/test/foo") == 1);
}

BOOST_AUTO_TEST_CASE(read_v1) {
  std::string output{"schema_evolution_recov1.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("recotest"));
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  fire::config::Parameters dk_rule;
  dk_rule.add<std::string>("regex",".*");
  dk_rule.add("keep",true);
  configuration.add<std::vector<fire::config::Parameters>>("drop_keep_rules", {dk_rule});

  configuration.add("event_limit", -1);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  fire::config::Parameters test_get;
  test_get.add<std::string>("name","test_get");
  test_get.add<std::string>("class_name","fire::test::schema_evolution::Get");

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_get});
  configuration.add<fire::config::Parameters>("conditions", {});
  configuration.add<std::vector<std::string>>("input_files", {"schema_evolution_prodv1.h5"});

  fire::Process p(configuration);
  p.run();

  // v1 gets copied over into v2
  H5Easy::File f{output};
  BOOST_TEST(readVersion(f, "events/test/foo") == 2);
  BOOST_TEST(not f.exist("/events/test/foo/dv1"));
  BOOST_TEST(f.exist("/events/test/foo/dv2"));
}

BOOST_AUTO_TEST_CASE(write_v2) {
  std::string output{"schema_evolution_prodv2.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  configuration.add("event_limit", 3);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  fire::config::Parameters test_add;
  test_add.add<std::string>("name","test_add");
  test_add.add<std::string>("class_name","fire::test::schema_evolution::Add");
  test_add.add<int>("version",2);

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_add});
  configuration.add<fire::config::Parameters>("conditions", {});

  fire::Process p(configuration);
  p.run();

  H5Easy::File f{output};
  BOOST_TEST(readVersion(f, "events/test/foo") == 2);
}

BOOST_AUTO_TEST_CASE(read_v2) {
  std::string output{"schema_evolution_recov2.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("recotest"));
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  output_file.add("compression_level", 6);
  output_file.add("shuffle",false);
  configuration.add("output_file",output_file);

  fire::config::Parameters dk_rule;
  dk_rule.add<std::string>("regex",".*");
  dk_rule.add("keep",true);
  configuration.add<std::vector<fire::config::Parameters>>("drop_keep_rules", {dk_rule});

  configuration.add("event_limit", -1);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  fire::config::Parameters test_get;
  test_get.add<std::string>("name","test_get");
  test_get.add<std::string>("class_name","fire::test::schema_evolution::Get");

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_get});
  configuration.add<fire::config::Parameters>("conditions", {});
  configuration.add<std::vector<std::string>>("input_files", {"schema_evolution_prodv2.h5"});

  fire::Process p(configuration);
  p.run();

  // v2 gets copied over as v2
  H5Easy::File f{output};
  BOOST_TEST(readVersion(f, "events/test/foo") == 2);
}


BOOST_AUTO_TEST_SUITE_END()
