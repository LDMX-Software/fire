find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_fire
  main_test.cxx
  dataset.cxx
  config.cxx
  process.cxx
  processor.cxx
  storage_control.cxx
  conditions.cxx
  )
  
target_link_libraries(test_fire
  PRIVATE Boost::unit_test_framework 
          framework)

add_test(NAME "TestFire"
  COMMAND $<TARGET_FILE:test_fire> --report_level=detailed
)
