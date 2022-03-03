
project(Module)

find_package(ROOT CONFIG REQUIRED)

option(BUILD_EVENT_ONLY "Build event dictionary" ON)
if (BUILD_EVENT_ONLY)
  register_event_object(module_path "Module/Event" namespace "bench" class "Hit" type "collection")
  setup_library(module Module
                name Event
                dependencies ROOT::Core
                register_target)
  return()
endif()

setup_library(module Module dependencies Framework::Framework)
