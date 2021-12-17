#ifndef FRAMEWORK_CONFIGUREPYTHON_H
#define FRAMEWORK_CONFIGUREPYTHON_H

/*~~~~~~~~~~~~~~~*/
/*   fire   */
/*~~~~~~~~~~~~~~~*/
#include "fire/config/Parameters.hpp"

namespace fire {

namespace config {

/**
 * namespace variables defining where to look for "root"
 * object to kickoff the parameter extraction from python.
 */
std::string root_module = "firecfg";
std::string root_class = "Process";
std::string root_object = "lastProcess";

class PyException : public std::runtime_error {
 public:
  PyException(const std::string& what) noexcept : std::runtime_error(what) {}
};

/**
 * run
 *
 * This method contains all the parsing and execution of the python script.
 *
 * @throw Exception if any necessary components of the python configuration
 * are missing. e.g. The Process class or the different members of
 * the lastProcess object.
 *
 * The basic premise of this function is to execute the python
 * configuration script by loading it into python as a module.
 * Then, **after the script has been executed**, all of the parameters
 * for the Process are gathered from python.
 * The fact that the script has been executed means that the user
 * can get up to a whole lot of shenanigans that can help them
 * make their work more efficient.
 *
 * @param pythonScript Filename location of the python script.
 * @param args Commandline arguments to be passed to the python script.
 * @param nargs Number of commandline arguments.
 */
Parameters run(const std::string& pythonScript, char* args[], int nargs);

}  // namespace config

}  // namespace fire

#endif  // FRAMEWORK_CONFIGURE_PYTHON_H
