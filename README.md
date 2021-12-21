# fire :fire:

**F**ramework for s**I**mulation and **R**econstruction of **E**vents.

It's a stretch but it's worth it for the cool name.

A event-by-event processing framework using [HDF5](https://www.hdfgroup.org/) via [HighFive](github.com/BlueBrain/HighFive) for serialization, [Boost](https://www.boost.org/) for logging, and C++17.

The core idea of this framework is the assumption that our data (simulated or real) can be grouped into "events" that we can assume are independent (for the software, not necessarily true in real life).
Each event is given to a sequence of "processors" that can look at the current data in the event and potentially produce more data to put into the event.

The behavior of this framework is dynamically configured at run-time by running an input python script and then translating python objects into their C++ counter-parts.
This configuration style is extermely flexible and allows both C++ and Python to do what they do best.

Besides this core functionality of processors looking at data event-by-event, there are optional helpers that allow processors to log through boost logging.

## Features
- Dynamic loading of libraries containing processors
- Dynamic creation of registered processors using their full C++ class name
- Header for event-wide information
- Header for run-wide information
- Simple serialization of user-defined classes allowing for quick h5py-based analyses
- Configuration through running a python script
- Drop/Keep Rules: Regex-based rules for copying data from input file to output file
  or choosing to _not_ save data created during processing
- Modern CMake infrasture enabling simple `find_package(fire)` syntax

- Basic interface to a conditions system
- Veto: Voting system for processors to decide on if entire event
  should be saved into the output file
- Logging through Boost (optional)

## Dependencies

- C++ compiler with C++17 support
- HDF5
- HighFive C++ interface for HDF5
- Boost.Logging (optional, for logging through fire)
- Boost Unit Testing Framework (optional, for testing)
