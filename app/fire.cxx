
//----------------//
//   C++ StdLib   //
//----------------//
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

//-------------//
//   ldmx-sw   //
//-------------//
#include "fire/Config/Python.h"
#include "fire/Process.h"

/**
 * @namespace fire
 * @brief All classes in the ldmx-sw project use this namespace.
 */
// using namespace fire;

// This code allows ldmx-app to exit gracefully when Ctrl-c is used. It is
// currently causing segfaults when certain processors are used.  The code
// will remain commented out until these issues are investigated further.
/*
static Process* p { 0 };

static void softFinish (int sig, siginfo_t *siginfo, void *context) {
if (p) p->requestFinish();
}
*/

/**
 * @func printUsage
 *
 * Print how to use this executable to the terminal.
 */
void printUsage();

/**
 * @mainpage
 *
 * <a
 * href="https://confluence.slac.stanford.edu/display/MME/Light+Dark+Matter+Experiment">LDMX</a>
 * C++ Software fire providing a full <a
 * href="http://geant4.cern.ch">Geant4</a> simulation and flexible event
 * processing and analysis chain.  The IO and analysis tools are based on <a
 * href="http://root.cern.ch">ROOT</a>, and detectors are described in the <a
 * href="https://gdml.web.cern.ch/GDML/">GDML</a> XML language.
 *
 * Refer to the <a href="https://github.com/LDMXAnalysis/ldmx-sw/">ldmx-sw
 * github</a> for more information, including build and usage instructions.
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
    fire::config::Parameters config{fire::config::run(argv[ptrpy], argv + ptrpy + 1, argc - ptrpy - 1)};
    p = std::make_unique<fire::Process>(config);
  } catch (fire::exception::Exception& e) {
    // TODO split into Python errors and C++ configuration errors
    std::cerr << "Configuration Error [" << e.name() << "] : " << e.message()
              << std::endl;
    std::cerr << "  at " << e.module() << ":" << e.line() << " in "
              << e.function() << std::endl;
    std::cerr << "Stack trace: " << std::endl << e.stackTrace();
    return 1;
  }

  std::cout << "---- FIRE: Starting event processing --------" << std::endl;

  try {
    p->run();
  } catch (fire::exception::Exception& e) {
    auto theLog_{fire::logging::makeLogger(
        "fire")};  // ldmx_log macro needs this variable to be named 'theLog_'
    ldmx_log(fatal) << "[" << e.name() << "] : " << e.message() << "\n"
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
