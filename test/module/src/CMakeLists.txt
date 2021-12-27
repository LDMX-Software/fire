
add_library(MyModule_Event SHARED MyObject.cxx)
target_link_libraries(MyModule_Event PUBLIC fire::h5)
target_include_directories(MyModule_Event PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(MyModule SHARED MyProducer.cxx)
target_link_libraries(MyModule PUBLIC MyModule_Event fire::framework)
target_include_directories(MyModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)
add_test(NAME ProductionMode COMMAND fire production.py)
add_test(NAME ReconMode COMMAND fire recon.py)
