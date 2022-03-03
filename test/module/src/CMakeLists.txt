
project(fire-Bench)

if (fire_USE_ROOT)
  # tell build event library with ROOT dictionary
  add_library(Bench_Event SHARED Hit.cxx)
  target_link_libraries(Bench_Event PUBLIC fire::io)
else()
  add_library(Bench_Event SHARED Hit.cxx)
  target_link_libraries(Bench_Event PUBLIC fire::io)
endif()

add_library(Bench SHARED Produce.cxx Recon.cxx)
target_link_libraries(Bench PUBLIC Bench_Event fire::framework)

install(TARGETS Bench_Event Bench LIBRARY DESTINATION lib)
