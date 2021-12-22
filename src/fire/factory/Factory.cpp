#include "fire/factory/Factory.hpp"

#include <dlfcn.h> // for shared library loading
#include <set>     // for caching loaded libraries

namespace fire::factory {

void loadLibrary(const std::string& libname) {
  static std::set<std::string> libraries_loaded_;
  if (libraries_loaded_.find(libname) != libraries_loaded_.end()) {
    return;  // already loaded
  }

  void* handle = dlopen(libname.c_str(), RTLD_NOW);
  if (handle == nullptr) {
    throw Exception("Error loading library '" + libname + "':" + dlerror());
  }

  libraries_loaded_.insert(libname);
}

}