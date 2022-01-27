# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(fire 
  VERSION 0.14.1
  DESCRIPTION "fire for sImulation and Reconstruction of Events"
  LANGUAGES C CXX)

# Set the default release type to "Release". If a release type is specified 
# at the command line, it's respected.
set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# necessary dependencies
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(Boost REQUIRED COMPONENTS log)
find_package(HighFive REQUIRED)

# optional dependency
find_package(ROOT 6.20 QUIET)
option(fire_USE_ROOT "Enable ability to read files produced with ROOT-based Framework" ${ROOT_FOUND})
if (NOT ROOT_FOUND AND fire_USE_ROOT)
  message(FATAL_ERROR "Unable to build ROOT-reader without access to ROOT.")
endif()

# Execute the command to extract the SHA1 hash of the current git tag.
# 'git' is removed from within the container to discourage opening a shell
# in the container, so we need to go to some lengths in order to avoid using 'git'
# The variable GIT_SHA1 will be set to contain the hash.
set(git_dir "${PROJECT_SOURCE_DIR}/.git")
if (NOT IS_DIRECTORY "${git_dir}")
  # we are a submodule of another project
  file(READ "${git_dir}" git_dir)
  string(REGEX REPLACE "^gitdir: " "" git_dir ${git_dir})
  string(REGEX REPLACE "\n$" "" git_dir ${git_dir})
endif()
file(READ "${git_dir}/HEAD" current_ref)
string(REGEX REPLACE "\n$" "" current_ref ${current_ref})
if (current_ref MATCHES "^ref:.*$")
  # on a branch
  string(REGEX REPLACE "^ref: " "" current_ref "${current_ref}")
  file(READ "${git_dir}/${current_ref}" GIT_SHA1)
  string(REGEX REPLACE "\n$" "" GIT_SHA1 ${GIT_SHA1})
else()
  # in detached head state (probably on a tag)
  set(GIT_SHA1 ${current_ref})
endif()
message(STATUS "Deduced git SHA: ${GIT_SHA1}")

# Copies the file 'Version.h.in', substitutes the value of GIT_SHA1 and writes
# it out to Version.h.
configure_file(${PROJECT_SOURCE_DIR}/include/fire/version/Version.h.in
              ${PROJECT_BINARY_DIR}/include/fire/version/Version.h)
install(FILES ${PROJECT_BINARY_DIR}/include/fire/version/Version.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/fire/version)

add_library(version INTERFACE)
target_include_directories(version
  INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(exception SHARED src/fire/exception/Exception.cxx)
target_include_directories(exception
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(factory SHARED src/fire/factory/Factory.cxx)
target_link_libraries(factory PUBLIC exception Boost::boost)
target_include_directories(factory
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(logging SHARED src/fire/logging/Logger.cxx)
target_link_libraries(logging PUBLIC Boost::log)
target_include_directories(logging
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(config SHARED src/fire/config/Python.cxx)
target_link_libraries(config PUBLIC exception Python3::Python Boost::boost)

if (fire_USE_ROOT)
  message(STATUS "Building ROOT reader")
  # include ROOT reader in io submodule
  add_library(io SHARED 
    src/fire/io/Writer.cxx
    src/fire/io/Atomic.cxx
    src/fire/io/ParameterStorage.cxx
    src/fire/io/h5/Reader.cxx
    src/fire/io/root/Reader.cxx)
  target_link_libraries(io PUBLIC version config HighFive ROOT::Core ROOT::TreePlayer)
else()
  message(WARNING "Reading ROOT files will not be supported.")
  add_library(io SHARED 
    src/fire/io/Writer.cxx
    src/fire/io/Atomic.cxx
    src/fire/io/ParameterStorage.cxx
    src/fire/io/h5/Reader.cxx)
  target_link_libraries(io PUBLIC version config HighFive)
endif()

add_library(framework SHARED
  src/fire/StorageControl.cxx
  src/fire/Event.cxx
  src/fire/EventHeader.cxx
  src/fire/Processor.cxx
  src/fire/Process.cxx
  src/fire/RunHeader.cxx
  src/fire/ConditionsIntervalOfValidity.cxx
  src/fire/ConditionsProvider.cxx
  src/fire/Conditions.cxx
  src/fire/RandomNumberSeedService.cxx
  )
target_link_libraries(framework PUBLIC logging version exception config factory io Boost::boost)
target_include_directories(framework PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")
if(fire_USE_ROOT)
  # generate dictionary source file from headers and link def
  root_generate_dictionary(fireDict
    ${PROJECT_SOURCE_DIR}/include/fire/RunHeader.h
    ${PROJECT_SOURCE_DIR}/include/fire/EventHeader.h
    LINKDEF ${PROJECT_SOURCE_DIR}/include/fire/fireLinkDef.h
    MODULE framework)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libframework_rdict.pcm DESTINATION lib)
endif()

# Compiling the fire library requires features introduced by the cxx 17 standard.
#   also to avoid conflicts, we expand the library prefix from 'lib' to 'libfire_'
set_target_properties(
  framework exception config factory io logging
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO
             PREFIX "libfire_")

# installing the fire target and generate CMake-compatible package files
#   is done by following the documentation linked below
# https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html
install(TARGETS framework exception config factory io version logging
  EXPORT fireTargets 
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )
# need pattern to exclude version header configured by CMake above
install(DIRECTORY include/ DESTINATION include/ FILES_MATCHING PATTERN "*.h")

# install everything except the pytest modules
install(DIRECTORY python/ DESTINATION python/ FILES_MATCHING 
  PATTERN "*.py" # INCLUDE
  PATTERN "test_*" EXCLUDE
  PATTERN ".pytest_cache" EXCLUDE
  PATTERN "__pycache__" EXCLUDE
  )

# install the export file
install(EXPORT fireTargets
  FILE fireTargets.cmake
  NAMESPACE fire::
  DESTINATION lib/cmake/fire
  )

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/fireConfig.cmake"
      INSTALL_DESTINATION lib/cmake/fire
      )
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/fireConfigVersion.cmake"
  COMPATIBILITY AnyNewerVersion)
install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/fireConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/fireConfigVersion.cmake"
      DESTINATION lib/cmake/fire
      )

# Add the fire executable
add_executable(fire app/fire.cxx)
target_link_libraries(fire PRIVATE framework)
install(TARGETS fire DESTINATION bin)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
  if (BUILD_TESTING)
    # compile C++ tests in test/ directory
    add_subdirectory(test)
    # add Python test executable
    add_test(NAME "pytest"
      COMMAND python3 -m pytest -v
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python)
  endif()
endif()
