#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <highfive/H5Easy.hpp>

#include "fire/Process.hpp"

/**
 * Test running of Process class with no processors
 *
 * Checks:
 * - construction of event header
 * - construction of process object
 * - serialization of Event and Run Headers
 * - event and run numbers in output files are done correctly
 */
BOOST_AUTO_TEST_SUITE(process)

BOOST_AUTO_TEST_CASE(production_mode) {
  std::string output{"production_mode_output.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  configuration.add("output_file",output_file);

  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  //configuration.add("input_files",vec<str> );
  
  //fire::config::Parameters& dk_rule;
  //dk_rule.add("regex","^.*/ObjToDrop$");
  //dk_rule.add("keep",false);
  //configuration.add("keep", vec<Param> );
  
  configuration.add("event_limit", 10);
  configuration.add("log_frequency", -1);
  configuration.add("run", 1);
  configuration.add("max_tries", 1);

  //configuration.add("libraries",vec<str>);
  //configuration.add("sequence",vec<Param>);
  configuration.add("testing",true); // ok for no sequence

  std::unique_ptr<fire::Process> p;
  try {
    p = std::make_unique<fire::Process>(configuration);
  } catch (fire::config::Parameters::Exception& e) {
    std::cerr << "[Config Error] " << e.what() << std::endl;
    BOOST_REQUIRE(false);
  }
  try {
    p->run();
  } catch (const HighFive::Exception& e) {
    std::cerr << "[H5 Error]" << e.what() << std::endl;
    BOOST_REQUIRE(false);
  } catch (fire::exception::Exception& e) {
    std::cerr << e.what() << std::endl;
    BOOST_REQUIRE(false);
  }

  // check that the event and run numbers in the output file are correct
  H5Easy::File f(output);
  auto event_numbers = H5Easy::load<std::vector<int>>(f, fire::EventHeader::NAME+"/number");
  std::vector<int> correct = {1,2,3,4,5,6,7,8,9,10};
  BOOST_CHECK(event_numbers == correct);
  auto run_numbers = H5Easy::load<std::vector<int>>(f, fire::RunHeader::NAME+"/number");
  correct = {1};
  BOOST_CHECK(run_numbers == correct);
}
BOOST_AUTO_TEST_CASE(recon_mode_single_file, *boost::unit_test::depends_on("process/production_mode")) {
  std::string output{"recon_mode_output.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  configuration.add("output_file",output_file);

  std::vector<std::string> input_files = { "production_mode_output.h5" };
  configuration.add("input_files",input_files );
  
  //fire::config::Parameters& dk_rule;
  //dk_rule.add("regex","^.*/ObjToDrop$");
  //dk_rule.add("keep",false);
  //configuration.add("keep", vec<Param> );
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  configuration.add("event_limit", -1);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1); // not used for recon mode
  configuration.add("max_tries", 1); // not used in recon mode

  //configuration.add("libraries",vec<str>);
  //configuration.add("sequence",vec<Param>);
  configuration.add("testing",true); // ok for no sequence

  try {
    fire::Process p(configuration);
    p.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    BOOST_CHECK(false);
  }

  // check that the event and run numbers in the output file are correct
  H5Easy::File f(output);
  auto event_numbers = H5Easy::load<std::vector<int>>(f, fire::EventHeader::NAME+"/number");
  std::vector<int> correct = {1,2,3,4,5,6,7,8,9,10};
  BOOST_CHECK(event_numbers == correct);
  auto run_numbers = H5Easy::load<std::vector<int>>(f, fire::RunHeader::NAME+"/number");
  correct = {1};
  BOOST_CHECK(run_numbers == correct);
}
BOOST_AUTO_TEST_CASE(recon_mode_multi_file, *boost::unit_test::depends_on("process/production_mode")) {

  { // generate another input file
    fire::config::Parameters configuration;
    configuration.add("pass",std::string("test"));

    fire::config::Parameters output_file;
    output_file.add("name", std::string("recon_mode_multi_input.h5"));
    output_file.add("event_limit", 10);
    output_file.add("rows_per_chunk", 1000);
    configuration.add("output_file",output_file);

    fire::config::Parameters storage;
    storage.add("default_keep",true);
    configuration.add("storage",storage);

    configuration.add("event_limit", 8);
    configuration.add("log_frequency", -1);
    configuration.add("run", 2);
    configuration.add("max_tries", 1);

    configuration.add("testing",true); // ok for no sequence

    fire::Process p(configuration);
    p.run();
  }

  std::string output{"recon_mode_multi_output.h5"};
  fire::config::Parameters configuration;
  configuration.add("pass",std::string("test"));

  fire::config::Parameters output_file;
  output_file.add("name", output);
  output_file.add("event_limit", 10);
  output_file.add("rows_per_chunk", 1000);
  configuration.add("output_file",output_file);

  std::vector<std::string> input_files = { "production_mode_output.h5", "recon_mode_multi_input.h5" };
  configuration.add("input_files",input_files );
  
  //fire::config::Parameters& dk_rule;
  //dk_rule.add("regex","^.*/ObjToDrop$");
  //dk_rule.add("keep",false);
  //configuration.add("keep", vec<Param> );
  
  fire::config::Parameters storage;
  storage.add("default_keep",true);
  configuration.add("storage",storage);

  configuration.add("event_limit", -1);
  configuration.add("log_frequency", -1);

  configuration.add("run", 1); // not used for recon mode
  configuration.add("max_tries", 1); // not used in recon mode

  //configuration.add("libraries",vec<str>);
  //configuration.add("sequence",vec<Param>);
  configuration.add("testing",true); // ok for no sequence

  try {
    fire::Process p(configuration);
    p.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    BOOST_CHECK(false);
  }

  // check that the event and run numbers in the output file are correct
  H5Easy::File f(output);
  auto event_numbers = H5Easy::load<std::vector<int>>(f, fire::EventHeader::NAME+"/number");
  std::vector<int> correct = {1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8};
  BOOST_CHECK(event_numbers == correct);
  auto run_numbers = H5Easy::load<std::vector<int>>(f, fire::RunHeader::NAME+"/number");
  correct = {2,1};
  BOOST_CHECK(run_numbers == correct);
}

BOOST_AUTO_TEST_SUITE_END()
