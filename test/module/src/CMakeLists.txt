
add_library(Bench_Event SHARED Hit.cxx)
target_link_libraries(Bench_Event PUBLIC fire::h5)
target_include_directories(Bench_Event PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Bench SHARED Produce.cxx Recon.cxx)
target_link_libraries(Bench PUBLIC Bench_Event fire::framework)
target_include_directories(Bench PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS Bench_Event Bench LIBRARY DESTINATION lib)
