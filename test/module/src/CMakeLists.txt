
project(fire-Bench)

add_library(Bench_Event SHARED Hit.cxx)
target_link_libraries(Bench_Event PUBLIC fire::io)
if (fire_USE_ROOT)
  # tell build event library with ROOT dictionary
  target_include_directories(Bench_Event PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")
  root_generate_dictionary(Bench_EventDict
    ${CMAKE_CURRENT_SOURCE_DIR}/Hit.h
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/Bench_EventLinkDef.h
    MODULE Bench_Event)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libBench_Event_rdict.pcm DESTINATION lib)
endif()

add_library(Bench SHARED Produce.cxx Recon.cxx)
target_link_libraries(Bench PUBLIC Bench_Event fire::framework)

install(TARGETS Bench_Event Bench 
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include)
