#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <fstream>  // writing test files
#include <string_view> // test file literals

#include "fire/config/Python.hpp"

/// python class defs for testing python running of script
std::string_view class_defs = " \n\
class SubClass() : \n\
    def __init__(self) : \n\
        self.substr = 'subtest' \n\
        self.subint = 100 \n\
        self.vec_dict = [{'sub':3,'bla':55},{'foo':'bar','baz':'buz'}] \n\
 \n\
class TestRoot() : \n\
    def __init__(self) : \n\
        self.string = 'test' \n\
        self.integer = 10 \n\
        self.double = 6.9 \n\
        self.int_vec = [1,2,3] \n\
        self.dict = {'one':1, 'two':2} \n\
        self.double_vec = [0.1,0.2] \n\
        self.string_vec = ['first','second','third'] \n\
        self.sub_class = SubClass() \n\
        self.vec_class = [SubClass(),SubClass()] \n\
";

/// declaration of test root config object with correct name
std::string_view root_obj = "test_root = TestRoot()";

/// python exception throw
std::string_view throw_exception = "raise Exception('test python exceptions')";

/// change integer variable given first argument to script
std::string_view arg_parse = "\n\
import sys \n\
test_root.integer = int(sys.argv[1]) \n\
";

/// the config file we will run and load
const std::string config_file_name{"/tmp/fire_config_python_test.py"};

/// change the root config object to dummy class defined in test script
std::string fire::config::root_object = "test_root";

/**
 * Test for reading configuration from python
 *
 * Checks:
 * - recursively load classes and basic types form a python script
 * - make sure mis-spelled or mis-typed parameters throw errors
 * - make sure catch python errors
 */
BOOST_AUTO_TEST_CASE(config_no_arg) {
  {
    std::ofstream config_py(config_file_name);
    config_py << class_defs << '\n' << root_obj << std::endl;
  }
  char *args[1];
  fire::config::Parameters config{fire::config::run(config_file_name, args,0)};
  BOOST_CHECK(config.get<std::string>("string") == "test");
  BOOST_CHECK(config.get<int>("integer") == 10);
  BOOST_CHECK(config.get<double>("double") == 6.9);

  auto int_vec{config.get<std::vector<int>>("int_vec")};
  for (std::size_t i{0}; i < int_vec.size(); i++) BOOST_CHECK(int_vec.at(i) == i+1);

  auto double_vec{config.get<std::vector<double>>("double_vec")};
  for (std::size_t i{0}; i < double_vec.size(); i++) BOOST_CHECK(double_vec.at(i) == (i+1)*0.1);

  auto string_vec{config.get<std::vector<std::string>>("string_vec")};
  BOOST_CHECK(string_vec.at(0) == "first");
  BOOST_CHECK(string_vec.at(1) == "second");
  BOOST_CHECK(string_vec.at(2) == "third");

  auto dict{config.get<fire::config::Parameters>("dict")};
  BOOST_CHECK(dict.get<int>("one") == 1);
  BOOST_CHECK(dict.get<int>("two") == 2);

  auto sub_class{config.get<fire::config::Parameters>("sub_class")};
  BOOST_CHECK(sub_class.get<std::string>("substr") == "subtest");
  BOOST_CHECK(sub_class.get<int>("subint") == 100);

  auto vec_dict{sub_class.get<std::vector<fire::config::Parameters>>("vec_dict")};
  BOOST_CHECK(vec_dict.at(0).get<int>("sub") == 3);
  BOOST_CHECK(vec_dict.at(0).get<int>("bla") == 55);
  BOOST_CHECK(vec_dict.at(1).get<std::string>("foo") == "bar");
  BOOST_CHECK(vec_dict.at(1).get<std::string>("baz") == "buz");

  // just check that we can 'get' vector of subclasses
  auto vec_class{config.get<std::vector<fire::config::Parameters>>("vec_class")};
}
BOOST_AUTO_TEST_CASE(config_one_arg) {
  {
    std::ofstream config_py(config_file_name);
    config_py << class_defs << "\n" << root_obj << "\n" << arg_parse << std::endl;
  }
  char *args[1];
  int correct{9000};
  char arg[16];
  snprintf(arg,sizeof(arg),"%d",correct);
  args[0] = arg;
  fire::config::Parameters config{fire::config::run(config_file_name, args, 1)};
  BOOST_CHECK(config.get<int>("integer") == correct);
}

BOOST_AUTO_TEST_CASE(config_no_root) {
  {
    std::ofstream config_py(config_file_name);
    config_py << class_defs << std::endl;
  }
  char *args[1];
  BOOST_CHECK_THROW(fire::config::run(config_file_name,args,0), fire::config::PyException);
}
BOOST_AUTO_TEST_CASE(config_py_except) {
  {
    std::ofstream config_py(config_file_name);
    config_py << class_defs << "\n" << throw_exception << "\n" << root_obj << std::endl;
  }
  char *args[1];
  BOOST_CHECK_THROW(fire::config::run(config_file_name,args,0), fire::config::PyException);
}
