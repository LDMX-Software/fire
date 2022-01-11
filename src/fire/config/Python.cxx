
#include "fire/config/Python.h"

/*~~~~~~~~~~~~*/
/*   python   */
/*~~~~~~~~~~~~*/
#include "Python.h"

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <any>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

namespace fire {
namespace config {

/**
 * Turn the input python string object into a C++ string.
 *
 * Helpful to condense down the multi-line nature of
 * the python3 code.
 *
 * @param[in] pyObj python object assumed to be a string python object
 * @return the value stored in it
 */
static std::string getPyString(PyObject* pyObj) {
  std::string retval;
  PyObject* pyStr = PyUnicode_AsEncodedString(pyObj, "utf-8", "Error ~");
  retval = PyBytes_AS_STRING(pyStr);
  Py_XDECREF(pyStr);
  return retval;
}

/**
 * Extract members from a python object.
 *
 * Iterates through the object's dictionary and translates the objects inside
 * of it into the type-specified C++ equivalents, then puts these
 * objects into a STL map that can be passed to the Parameters class.
 *
 * This function is recursive. If a non-base type is encountered,
 * we pass it back along to this function to translate it's own dictionary.
 *
 * We rely completely on python being awesome. For all higher level class
 * objects, python keeps track of all of its member variables in the member
 * dictionary `__dict__`.
 *
 * No Py_DECREF calls are made because all of the members of an object
 * are borrowed references, meaning that when we destory that object, it handles
 * the other members. We destroy the one Python object owning all of
 * these references at the end of this function.
 *
 * @note Not sure if this is not leaking memory, kinda just trusting
 * the Python / C API docs on this one.
 *
 * @note Empty lists are NOT read in because there is no way for us
 * to know what type should be inside the list. This means list
 * parameters that can be empty need to put in a default empty list value: 
 * `{}`.
 *
 * This recursive extraction method is able to handle the following cases.
 * - User-defined classes (via the `__dict__` member) are extracted to Parameters
 * - one-dimensional lists whose entries all have the same type are extracted
 *   to `std::vector` of the type of the first entry in the list
 * - `dict` objects are extracted to Parameters
 * - Python `str` are extracted to `std::string`
 * - Python `int` are extracted to C++ `int`
 * - Python `bool` are extracted to C++ `bool`
 * - Python `float` are extracted to C++ `double`
 *
 * Known design flaws include
 * - No support for nested Python lists
 * - Annoying band-aid solution for empty Python lists
 *
 * @param[in] object Python object to get members from
 * @return Mapping between member name and value.
 */
static Parameters getMembers(PyObject* object) {
  PyObject* dictionary{PyObject_GetAttrString(object, "__dict__")};

  if (dictionary == 0) {
    if (PyDict_Check(object))
      dictionary = object;
    else {
      throw python::Exception("Python Object does not have a __dict__ member");
    }
  }

  PyObject *key(0), *value(0);
  Py_ssize_t pos = 0;

  Parameters params;

  while (PyDict_Next(dictionary, &pos, &key, &value)) {
    std::string skey{getPyString(key)};

    if (PyLong_Check(value)) {
      if (PyBool_Check(value)) {
        params.add(skey, bool(PyLong_AsLong(value)));
      } else {
        params.add(skey, int(PyLong_AsLong(value)));
      }
    } else if (PyFloat_Check(value)) {
      params.add(skey, PyFloat_AsDouble(value));
    } else if (PyUnicode_Check(value)) {
      params.add(skey, getPyString(value));
    } else if (PyList_Check(value)) {
      // assume everything is same value as first value
      if (PyList_Size(value) > 0) {
        auto vec0{PyList_GetItem(value, 0)};

        if (PyLong_Check(vec0)) {
          std::vector<int> vals;

          for (auto j{0}; j < PyList_Size(value); j++)
            vals.push_back(PyLong_AsLong(PyList_GetItem(value, j)));

          params.add(skey, vals);

        } else if (PyFloat_Check(vec0)) {
          std::vector<double> vals;

          for (auto j{0}; j < PyList_Size(value); j++)
            vals.push_back(PyFloat_AsDouble(PyList_GetItem(value, j)));

          params.add(skey, vals);

        } else if (PyUnicode_Check(vec0)) {
          std::vector<std::string> vals;
          for (Py_ssize_t j = 0; j < PyList_Size(value); j++) {
            PyObject* elem = PyList_GetItem(value, j);
            vals.push_back(getPyString(elem));
          }

          params.add(skey, vals);

        } else {
          // RECURSION zoinks!
          // If the objects stored in the list doesn't
          // satisfy any of the above conditions, just
          // create a vector of parameters objects
          std::vector<fire::config::Parameters> vals;
          for (auto j{0}; j < PyList_Size(value); ++j) {
            auto elem{PyList_GetItem(value, j)};

            vals.emplace_back(getMembers(elem));
          }
          params.add(skey, vals);

        }  // type of object in python list
      }    // python list has non-zero size
    } else {
      // object got here, so we assume
      // it is a higher level object
      //(same logic as last option for a list)

      // RECURSION zoinks!
      fire::config::Parameters val(getMembers(value));

      params.add(skey, val);

    }  // python object type
  }    // loop through python dictionary

  return std::move(params);
}

Parameters run(const std::string& pythonScript, char* args[], int nargs) {
  // assumes that nargs >= 0
  //  this is true always because we error out if no python script has been
  //  found

  std::string cmd = pythonScript;
  if (pythonScript.rfind("/") != std::string::npos) {
    cmd = pythonScript.substr(pythonScript.rfind("/") + 1);
  }
  cmd = cmd.substr(0, cmd.find(".py"));

  // python needs the argument list as if you are on the command line
  //  targs = [ script , arg0 , arg1 , ... ] ==> len(targs) = nargs+1
  // PySys_SetArgvEx uses wchar_t instead of char in python3
  wchar_t** targs = new wchar_t*[nargs + 1];
  targs[0] = Py_DecodeLocale(pythonScript.c_str(), NULL);
  for (int i = 0; i < nargs; i++) targs[i + 1] = Py_DecodeLocale(args[i], NULL);

  // name our program after the script that is being run
  Py_SetProgramName(targs[0]);

  // start up python interpreter
  Py_Initialize();

  // The third argument to PySys_SetArgvEx tells python to import
  // the args and add the directory of the first argument to
  // the PYTHONPATH
  // This way, the command to import the module just needs to be
  // the name of the python script
  PySys_SetArgvEx(nargs + 1, targs, 1);

  // the following line is what actually runs the script
  std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(pythonScript.c_str(),"r"),&fclose};
  if (fp.get() == NULL) {
    // file does not exist
    throw python::Exception("Configuration script "+pythonScript+
        " either does not exist or can't be read.");
  }
  if (PyRun_SimpleFile(fp.get(), pythonScript.c_str()) != 0) {
    // running the script executed with an error
    PyErr_Print();
    throw python::Exception("Execution of python script failed.");
  } 

  // script has been run so we can
  // free up arguments to python script
  for (int i = 0; i < nargs + 1; i++) PyMem_RawFree(targs[i]);
  delete[] targs;

  // running a python script effectively imports the script into the top-level
  // code environment called '__main__'
  //  we "import" this module which is already imported to get a handle
  //  on the necessary objects
  PyObject* py_root_obj = PyImport_ImportModule("__main__");
  if (!py_root_obj) {
    PyErr_Print();
    throw python::Exception("I don't know what happened. This should never happen.");
  }

  // descend the hierarchy of modules that hold the root_object
  // manually expanding the '.' allows us to handle all of the different
  // cases of how the configuration Python class could have been imported
  // and constructed
  std::string attr;
  std::stringstream root_obj_ss{root_object};
  while (std::getline(root_obj_ss, attr, '.')) {
    PyObject* one_level_down = PyObject_GetAttrString(py_root_obj, attr.c_str());
    if (one_level_down == 0) {
      throw python::Exception("Unable to find python object '"+attr+"'.");
    }
    Py_DECREF(py_root_obj); // don't need previous python object anymore
    py_root_obj = one_level_down;
  }

  // now py_root_obj should hold the root configuration object
  if (py_root_obj == Py_None) {
    // root config object left undefined
    throw python::Exception("Root configuration object "+root_object+" not defined. This object is required to run.");
  }

  // okay, now we have fully imported the script and gotten the handle
  // to the root configuration object defined in the script.
  // We can now look at this object and recursively get all of our parameters out of it.

  Parameters configuration(getMembers(py_root_obj));

  // all done with python nonsense
  // delete one parent python object
  // MEMORY still not sure if this is enough, but not super worried about it
  //  because this only happens once per run
  Py_DECREF(py_root_obj);
  // close up python interpreter
  if (Py_FinalizeEx() < 0) {
    PyErr_Print();
    throw python::Exception("I wasn't able to close up the python interpreter!");
  }

  return configuration;
}

}  // namespace config
}  // namespace fire
