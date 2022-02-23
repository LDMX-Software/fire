
add_library(Bench_Event SHARED Hit.cxx)
target_link_libraries(Bench_Event PUBLIC fire::h5)

find_package(ROOT 6.20)
if (ROOT_FOUND)
  target_link_libraries(Bench_Event PUBLIC ROOT::Core)
  target_compile_definitions(Bench_Event PUBLIC USE_ROOT=1)
endif()

add_library(Bench SHARED Produce.cxx Recon.cxx)
target_link_libraries(Bench PUBLIC Bench_Event fire::framework)

install(TARGETS Bench_Event Bench LIBRARY DESTINATION lib)
