/**
 * @file Constants.h
 * Definitions for names of structures required by
 * serialization methods.
 */
#ifndef FIRE_H5_STRUCTURE_H
#define FIRE_H5_STRUCTURE_H

#include <string>

namespace fire::h5 {

/**
 * Structure constants vital to serialization method
 */
struct constants {
  /// the name of the event header data set
  inline static const std::string EVENT_HEADER_NAME = "EventHeader";
  /// the name of the variable in event and run headers corresponding to their ID
  inline static const std::string NUMBER_NAME = "number";
  /// the name of the group in the file holding all event objects
  inline static const std::string EVENT_GROUP = "events";
  /// the name of the group holding the run headers
  inline static const std::string RUN_HEADER_NAME = "runs";
  /// the name of the HDF5 object attribute that holds the event object type
  inline static const std::string TYPE_ATTR_NAME = "type";
  /// the name of the fire version attribute
  inline static const std::string VERS_ATTR_NAME = "version";
};

}

#endif  // FIRE_H5_STRUCTURE_H
