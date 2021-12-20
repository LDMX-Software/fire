# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(fire VERSION 0.0.0 
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

# Search for the Python3 library
find_package(Python3 COMPONENTS Interpreter Development)

# Search for Boost
find_package(
  Boost REQUIRED
  COMPONENTS system
             log
             filesystem
             thread
             chrono
             atomic
             regex)

find_package(HighFive REQUIRED)

# Execute the command to extract the SHA1 hash of the current git tag. The
# variable GIT_SHA1 will be set to contain the hash.
execute_process(COMMAND git rev-parse HEAD OUTPUT_VARIABLE GIT_SHA1)

# Remove the newline character
string(REGEX REPLACE "\n$" "" GIT_SHA1 "${GIT_SHA1}")

# Copies the file 'Version.h.in', substitutes the value of GIT_SHA1 and writes
# it out to Version.h.
configure_file(${PROJECT_SOURCE_DIR}/include/fire/Version.hpp.in
              ${PROJECT_BINARY_DIR}/include/fire/Version.hpp)
install(FILES ${PROJECT_BINARY_DIR}/include/fire/Version.hpp
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/fire/)

add_library(fire_exception SHARED src/fire/exception/Exception.cpp)
target_include_directories(fire_exception PUBLIC include)

add_library(fire_factory SHARED src/fire/factory/Factory.cpp)
target_include_directories(fire_factory PUBLIC include)

add_library(fire_config SHARED src/fire/config/Python.cpp)
target_link_libraries(fire_config PUBLIC Python3::Python)
target_include_directories(fire_config PUBLIC include)

add_library(fire_h5 SHARED
  src/fire/h5/Writer.cpp
  src/fire/h5/Reader.cpp
  src/fire/h5/DataSet.cpp)
target_link_libraries(fire_h5 PUBLIC HighFive fire_config)
target_include_directories(fire_h5 PUBLIC include)

add_library(fire_framework SHARED
  src/fire/Event.cpp
  src/fire/EventHeader.cpp
  src/fire/Processor.cpp
  src/fire/Process.cpp
  src/fire/RunHeader.cpp)
target_include_directories(fire_framework PUBLIC include)
target_link_libraries(fire_framework PUBLIC Boost::log fire_exception fire_config fire_factory fire_h5)

# Compiling the fire library requires features introduced by the cpp 17
# standard.
set_target_properties(
  fire_framework
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO)

install(TARGETS fire_exception fire_framework DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY python/ DESTINATION python)

# Add the fire executable
add_executable(fire app/fire.cpp)
target_link_libraries(fire PRIVATE fire_framework)
install(TARGETS fire DESTINATION bin)

option(TEST_FIRE "Build fire test executable." ON)
if (TEST_FIRE)
  add_subdirectory(test)
endif()
