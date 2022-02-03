# 🔥 fire 🔥

Framework for sImulation and Reconstruction of Events.

It's a stretch but it's worth it for the cool name.

<p align="center">
  <a href="http://perso.crans.org/besson/LICENSE.html" alt="GPLv3 license">
    <img src="https://img.shields.io/badge/License-GPLv3-blue.svg" />
  </a>
  <a href="https://github.com/LDMX-Software/fire/actions" alt="Actions">
    <img src="https://img.shields.io/github/workflow/status/LDMX-Software/fire/CI" />
  </a>
  <a href="https://github.com/LDMX-Software/fire/releases" alt="Releases">
    <img src="https://img.shields.io/github/v/release/LDMX-Software/fire" />
  </a>
</p>

An event-by-event processing framework using [HDF5](https://www.hdfgroup.org/) via [HighFive](https://github.com/BlueBrain/HighFive) for serialization, [Boost](https://www.boost.org/) for logging, Python3 for configuration, and C++17.

The core idea of this framework is the assumption that our data (simulated or real) can be grouped into "events" that we can assume are independent (for the software, not necessarily true in real life).
Each event is given to a sequence of "processors" that can look at the current data in the event and potentially produce more data to put into the event.

The behavior of this framework is dynamically configured at run-time by running an input python script and then translating python objects into their C++ counter-parts.
This configuration style is extermely flexible and allows both C++ and Python to do what they do best.

Besides this core functionality of processors looking at data event-by-event, there are optional helpers that allow processors to log through boost logging and access "conditions" through a centrally-controlled system.

## Features
- User defined custom processors to handle data generation and manipulation: fire::Processor
- Dynamic run-time loading of libraries containing processors and 
  creation of registered processors using their full C++ class name: fire::factory::Factory 
- Header for event-wide information: fire::EventHeader
- Header for run-wide information: fire::RunHeader
- Simple serialization of user-defined classes allowing 
  for quick h5py-based analyses: fire::h5
- Run-time configuration via a python script: fire::config and \pythonpackage
- Drop/Keep Rules: Regex-based rules for copying data from input file to output file
  or choosing to _not_ save data created during processing: fire::Event
- Veto: Voting system for processors to decide if an entire event
  should be saved into the output file : fire::StorageControl
- Logging through Boost : fire::logging
- Interface to a conditions system for support data : fire::Conditions
- Modern CMake infrasture enabling simple `find_package(fire)` syntax

## Dependencies

- C++ compiler with C++17 support
- HDF5
- HighFive C++ interface for HDF5
  - Accessing the HDF5 C API directly is feasible; however, HighFive provides support for Exception translation and `std::string` which I don't want to have to figure out myself.
- Boost Core (for various low-level tasks like demangling)
- Boost Logging (for logging through fire)
- Python3

### Developer Dependencies
- Boost Unit Testing Framework
- [pytest](https://docs.pytest.org/en/6.2.x/)
- doxygen
