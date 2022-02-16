/**
 * @file fire.cxx
 * @brief main definition for fire executable
 */

#include <iostream>
#include "fire/config/Python.h"
#include "fire/Process.h"

/**
 * Print how to use this executable to the terminal.
 */
static void usage() {
  std::cout << 
    "\n"
    " USAGE:\n"
    "  fire {configuration_script.py} [arguments to configuration script]\n"
    "\n"
    " ARGUMENTS:\n"
    "  configuration_script.py  (required) "
    "python script to configure the processing\n"
    "  additional arguments     (optional) "
    "passed to configuration script when run in python\n"
    << std::endl;
}

/**
 * definition of fire executable
 *
 * fire is done in two steps
 * 1. Configuration - we run the python script provided
 *    and decode the parameters connected to the root
 *    configuration object, passing these parameters to 
 *    the corresponding C++ classes by creating the Process.
 * 2. Running - we run the Process that has been configured.
 *
 * @param[in] argc command line argument count
 * @param[in] argv array of command line arguments
 */
int main(int argc, char* argv[]) {
  if (argc < 2) {
    usage();
    return 1;
  }

  int ptrpy = 1;
  for (ptrpy = 1; ptrpy < argc; ptrpy++) {
    if (strstr(argv[ptrpy], ".py")) break;
  }

  if (ptrpy == argc) {
    usage();
    std::cout << " ** No python configuration script provided (must end in "
                 "'.py'). ** "
              << std::endl;
    return 1;
  }

  std::cout << "---- FIRE: Loading configuration --------" << std::endl;

  std::unique_ptr<fire::Process> p;
  try {
    fire::config::Parameters config{
        fire::config::run("fire.cfg.Process.lastProcess",
            argv[ptrpy], argv + ptrpy + 1, argc - ptrpy - 1)};
    p = std::make_unique<fire::Process>(config);
  } catch (const fire::Exception& e) {
    std::cerr << "[" << e.category() << "] " << e.message() << std::endl;
    if (not e.trace().empty()) {
      std::cerr << "Stack Trace:\n" << e.trace() << std::endl;
    }
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "UNKNOWN EXCEPTION: " << e.what() << std::endl;
    return 127;
  }

  std::cout << "---- FIRE: Starting event processing --------" << std::endl;

  // successfully creating the Process also means the logger
  // was successfully opened, create one here for printing
  // exceptions
  // the fire_log macro expects this variable to be named 'theLog_'
  auto theLog_{fire::logging::makeLogger("fire")};

  try {
    p->run();
  } catch (const fire::Exception& e) {
    std::cerr << "[" << e.category() << "] " << e.message() << std::endl;
    if (not e.trace().empty()) {
      std::cerr << "Stack Trace:\n" << e.trace() << std::endl;
    }
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "UNKNOWN EXCEPTION: " << e.what() << std::endl;
    return 127;
  }

  std::cout << "---- FIRE: Event processing complete  --------" << std::endl;

  return 0;
}

