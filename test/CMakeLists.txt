find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_fire
  main_test.cpp
  dataset.cpp
  config.cpp
  process.cpp
  processor.cpp
  storage_control.cpp
  )
  
target_link_libraries(test_fire
  PRIVATE Boost::unit_test_framework 
          framework)

# insert running of test_executable as the 'make test' target
add_custom_target(test
  COMMAND $<TARGET_FILE:test_fire> --report_level=detailed
  COMMENT "Run the unit test executable"
)
