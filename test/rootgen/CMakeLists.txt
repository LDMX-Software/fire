# This is a sample project to test integration via add_subdirectory and CMakeConfig
cmake_minimum_required(VERSION 3.12)

project(fire-test-rootread)

find_package(ROOT)
if (NOT ROOT_FOUND)
  message(STATUS "Not testing ROOT reader.")
  return()
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()


# Load additional macros used by this project. 
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)

# Load the BuildMacros module. If loaded correctly, the variable 
# 'build_macros_found' will be set to the path of the module.  Otherwise, it
# is set to NOTFOUND.
include(BuildMacros RESULT_VARIABLE build_macros_found)

# Set the python installation path
set(PYTHON_PACKAGE_NAME LDMX)

# Set the default release type to "Release". If a release type is specified 
# at the command line, it's respected.
set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
      message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
      set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

# Clear any variables cached during previous configuration cycles. 
clear_cache_variables()

# Start by building all of the classes needed for building the event bus and 
# ROOT dictionary. This is on by forcing the global "BUILD_EVENT_ONLY" option
# here.
set(BUILD_EVENT_ONLY ON CACHE BOOL "Build event dictionary." FORCE)

add_subdirectory(Module ${CMAKE_BINARY_DIR}/ModuleEvent)

# Once the event libraries have been built, turn off the global option. 
set(BUILD_EVENT_ONLY OFF CACHE BOOL "Build event dictionary." FORCE)

add_subdirectory(Framework)
add_subdirectory(Module)
