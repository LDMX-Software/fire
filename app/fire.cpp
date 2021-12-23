
//----------------//
//   C++ StdLib   //
//----------------//
#include <iostream>

//-------------//
//   ldmx-sw   //
//-------------//
#include "fire/config/Python.hpp"
/// define the object to pull parameters from
std::string fire::config::root_object = "fire.cfg.Process.lastProcess";

#include "fire/Process.hpp"

/**
 * @func printUsage
 *
 * Print how to use this executable to the terminal.
 */
void printUsage();

/**
 * @mainpage
 * TODO rewrite
 */
int main(int argc, char* argv[]) {
  if (argc < 2) {
    printUsage();
    return 1;
  }

  int ptrpy = 1;
  for (ptrpy = 1; ptrpy < argc; ptrpy++) {
    if (strstr(argv[ptrpy], ".py")) break;
  }

  if (ptrpy == argc) {
    printUsage();
    std::cout << " ** No python configuration script provided (must end in "
                 "'.py'). ** "
              << std::endl;
    return 1;
  }

  std::cout << "---- FIRE: Loading configuration --------" << std::endl;

  std::unique_ptr<fire::Process> p;
  try {
    fire::config::Parameters config{
        fire::config::run(argv[ptrpy], argv + ptrpy + 1, argc - ptrpy - 1)};
    p = std::make_unique<fire::Process>(config);
  } catch (fire::config::PyException& e) {
    std::cerr << "[Python Error] " << e.what() << std::endl;
    return 1;
  } catch (fire::config::Parameters::Exception& e) {
    std::cerr << "[Config Error] " << e.what() << std::endl;
    return 2;
  } catch (fire::factory::Exception& e) {
    std::cerr << "[Creation Error] " << e.what() << std::endl;
    return 3;
  } catch (std::exception& e) {
    std::cerr << "Unknown Exception: " << e.what() << std::endl;
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
  } catch (const HighFive::Exception& e) {
    fire_log(fatal) << "[H5 Error] " << e.what();
  } catch (const fire::Conditions::Exception& e) {
    fire_log(fatal) << "[Conditions Error] " << e.what();
  } catch (fire::exception::Exception& e) {
    fire_log(fatal) << "[" << e.name() << "] : " << e.message() << "\n"
                    << "  at " << e.module() << ":" << e.line() << " in "
                    << e.function() << "\nStack trace: " << std::endl
                    << e.stackTrace();
    return 127;  // return non-zero error-status
  }

  std::cout << "---- FIRE: Event processing complete  --------" << std::endl;

  return 0;
}

void printUsage() {
  std::cout << "Usage: fire {configuration_script.py} [arguments to "
               "configuration script]"
            << std::endl;
  std::cout << "     configuration_script.py  (required) python script to "
               "configure the processing"
            << std::endl;
  std::cout << "     arguments                (optional) passed to "
               "configuration script when run in python"
            << std::endl;
}
