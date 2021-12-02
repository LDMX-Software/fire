# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(fire VERSION 0.0.0 
  DESCRIPTION "fire for sImulation and Reconstruction of Events"
  LANGUAGES CXX)

setup_library(module fire name Exception)

setup_library(module fire name Configure interface)

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

# Execute the command to extract the SHA1 hash of the current git tag. The
# variable GIT_SHA1 will be set to contain the hash.
execute_process(COMMAND git rev-parse HEAD OUTPUT_VARIABLE GIT_SHA1)

# Remove the newline character
string(REGEX REPLACE "\n$" "" GIT_SHA1 "${GIT_SHA1}")

# Copies the file 'Version.h.in', substitutes the value of GIT_SHA1 and writes
# it out to Version.h.
configure_file(${PROJECT_SOURCE_DIR}/include/fire/Version.h.in
              ${PROJECT_BINARY_DIR}/include/fire/Version.h)
install(FILES ${PROJECT_BINARY_DIR}/include/fire/Version.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/fire/)

find_package(HighFive REQUIRED)

# Setup the library
setup_library(module fire 
  dependencies Python3::Python
               Boost::log
               Boost::atomic
               Boost::regex
               fire::Exception
               fire::Configure
               HighFive)

# Compiling the fire library requires features introduced by the cpp 17
# standard.
set_target_properties(
  fire
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO)

# Add the fire executable
add_executable(fire ${PROJECT_SOURCE_DIR}/app/fire.cxx)
target_link_libraries(fire PRIVATE fire::fire)
install(TARGETS fire DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Setup the test
setup_test(dependencies fire::fire)

setup_python(package_name ${PYTHON_PACKAGE_NAME}/fire)
