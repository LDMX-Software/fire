find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_fire
  main_test.cpp
  dataset.cpp
  config.cpp
  process.cpp
  processor.cpp
  storage_control.cpp
  conditions.cpp
  )
  
target_link_libraries(test_fire
  PRIVATE Boost::unit_test_framework 
          framework)

add_test(NAME "TestFire"
  COMMAND $<TARGET_FILE:test_fire> --report_level=detailed
)

add_test(NAME "CMakeFind"
  COMMAND
    "${CMAKE_CTEST_COMMAND}"
      --build-and-test "${CMAKE_CURRENT_SOURCE_DIR}/module"
      "${CMAKE_CURRENT_BINARY_DIR}/module"
      --build-generator "${CMAKE_GENERATOR}"
      --test-command "${CMAKE_CTEST_COMMAND}"
)

