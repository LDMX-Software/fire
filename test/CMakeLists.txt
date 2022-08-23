find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_fire
  main_test.cxx
  data.cxx
  config.cxx
  process.cxx
  processor.cxx
  legacy_processor.cxx
  storage_control.cxx
  conditions.cxx
  highlevel.cxx
  helper_exceptions.cxx
  )
  
target_link_libraries(test_fire PRIVATE Boost::unit_test_framework framework) 

add_test(NAME "TestFire"
  COMMAND $<TARGET_FILE:test_fire> --report_level=detailed
)

add_test(NAME "h5pyReadBools"
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/read-bools.py ${CMAKE_CURRENT_BINARY_DIR}/production_mode_output.h5
)
