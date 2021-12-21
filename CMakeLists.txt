# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(fire 
  VERSION 0.0.0 
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

# Execute the command to extract the SHA1 hash of the current git tag. The
# variable GIT_SHA1 will be set to contain the hash.
execute_process(COMMAND git rev-parse HEAD OUTPUT_VARIABLE GIT_SHA1)

# Remove the newline character
string(REGEX REPLACE "\n$" "" GIT_SHA1 "${GIT_SHA1}")

# Copies the file 'Version.h.in', substitutes the value of GIT_SHA1 and writes
# it out to Version.h.
configure_file(${PROJECT_SOURCE_DIR}/include/fire/Version.hpp.in
              ${PROJECT_BINARY_DIR}/include/Version.hpp)
install(FILES ${PROJECT_BINARY_DIR}/include/Version.hpp
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/fire)

add_library(exception SHARED src/fire/exception/Exception.cpp)
target_include_directories(exception
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(factory SHARED src/fire/factory/Factory.cpp)
target_include_directories(factory
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(config SHARED src/fire/config/Python.cpp)
target_link_libraries(config PUBLIC Python3::Python)
target_include_directories(config
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(h5 SHARED
  src/fire/h5/Writer.cpp
  src/fire/h5/Reader.cpp
  src/fire/h5/DataSet.cpp
  src/fire/h5/ParameterStorage.cpp)
target_link_libraries(h5 PUBLIC HighFive config)
target_include_directories(h5
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )

add_library(framework SHARED
  src/fire/Event.cpp
  src/fire/EventHeader.cpp
  src/fire/Processor.cpp
  src/fire/Process.cpp
  src/fire/RunHeader.cpp)
target_include_directories(framework 
  PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
  )
target_link_libraries(framework PUBLIC Boost::log exception config factory h5)

# Compiling the fire library requires features introduced by the cpp 17 standard.
set_target_properties(
  framework
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO)

# installing the fire target and generate CMake-compatible package files
#   is done by following the documentation linked below
# https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html
install(TARGETS framework exception config factory h5
  EXPORT fireTargets 
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )
# need pattern to exclude version header configured by CMake above
install(DIRECTORY include/ DESTINATION include/ FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY python/ DESTINATION python/fire)

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
add_executable(fire app/fire.cpp)
target_link_libraries(fire PRIVATE framework)
install(TARGETS fire DESTINATION bin)

option(TEST_FIRE "Build fire test executable." ON)
if (TEST_FIRE)
  add_subdirectory(test)
endif()
