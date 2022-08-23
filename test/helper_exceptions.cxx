#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "fire/Process.h"
#include "fire/config/Parameters.h"

namespace fire::test {

bool is_no_output_file_exception(const fire::Exception& e) {
  bool pass{e.category() == "NoOutputFile"};
  if (not pass) {
    std::cerr << e.category() << std::endl;
  }
  return pass;
}

}

/**
 * Make sure helpful, high-level exceptions are being thrown
 *
 * - no output file -> specific NoFile exception
 */
BOOST_AUTO_TEST_SUITE(helper_exceptions)

BOOST_AUTO_TEST_CASE(no_output_file) {
  fire::config::Parameters configuration;
  configuration.add("pass_name",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add<std::string>("name", "");
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

  fire::config::Parameters test_add;
  test_add.add<std::string>("name","test_add");
  test_add.add<std::string>("class_name","fire::test::TestAdd");

  fire::config::Parameters test_get;
  test_get.add<std::string>("name","test_get");
  test_get.add<std::string>("class_name","fire::test::TestGet");
  test_get.add("same_sequence", true);

  configuration.add<std::vector<fire::config::Parameters>>("sequence", {test_add, test_get});
  configuration.add<fire::config::Parameters>("conditions",{});

  BOOST_CHECK_EXCEPTION(std::make_unique<fire::Process>(configuration), 
      fire::Exception, fire::test::is_no_output_file_exception);
}

BOOST_AUTO_TEST_SUITE_END()
