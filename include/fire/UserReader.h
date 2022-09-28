#ifndef FIRE_USERREADER_H
#define FIRE_USERREADER_H

#include "fire/io/Reader.h"
#include "fire/Event.h"

namespace fire {

/**
 * class for easier reading of files written by fire
 *
 * The UserReader uses a specially-configured Event instance to
 * interface with the file it is reading. This allows for it to 
 * read everything in the same manner that a processor would see
 * while giving a user control over when to go to the next event
 * in the file.
 *
 * @note opening the Event bus with a NULL writer is dangerous.
 * Any development of this class needs to be done extremely carefully
 * and tested thoroughly.
 *
 * ### Basic Usage
 * ```cpp
 * fire::UserReader r("filename.h5");
 * while (r.next()) {
 *   const auto& obj = r.get<T>(name,pass);
 * }
 * ```
 *
 * ### Wrap Around Usage
 * If the reader is configured to wrap around from the end of the file
 * to the beginning, then the reader cannot be in control of the event
 * loop (since it would be an infinite loop). The dummy example below
 * shows how a user can go through a file of arbitrary number of non-zero
 * events repeating if necessary.
 * ```cpp
 * fire::UserReader r("filename.h5", n_events_to_skip, true);
 * for (std::size_t i{0}; i < n_events_wanted; ++i) {
 *   r.next();
 *   const auto& obj = r.get<T>(name, pass);
 * }
 * ```
 */
class UserReader {
 public:
  /**
   * Configure the reading mode
   *
   * @param[in] wrap_around (optional) will we loop from end of file to beginning?
   */
  UserReader(bool wrap_around = false);
  
  /**
   * Configure the reading mode and open a file
   *
   * @see open for details on opening the file
   *
   * @param[in] fn file path to be opened
   * @param[in] n (optional) number of entries at beginning of file to skip
   * @param[in] wrap_around (optional) will we loop from end of file to beginning?
   */
  UserReader(const std::string& fn, unsigned long int n = 0, bool wrap_around = false);

  /**
   * Open the provided file, skipping an initial number of entries
   *
   * @see io::open for how we open ROOT and/or H5 files
   * @see io::root::Reader for reading of ROOT files
   * @see io::h5::Reader for reading of H5 files
   * @see Event::setInputFile for connecting the file reader and event bus
   *
   * Skipping the initial number of entries is done by just calling next
   * the requested number of times. Without any objects loaded onto the
   * event bus, this is simply moving the entry index within Event.
   *
   * @param[in] fn file path to be opened
   * @param[in] n (optional) number of entries at beginning of file to skip
   */
  void open(const std::string& fn, unsigned long int n = 0);

  /**
   * go to the next event entry in the file
   *
   * @throw Exception if a file hasn't been opened yet
   *
   * We simply mimic the interaction between Process and Event
   * such that Event::next is called "after" the last event
   * and Event::load is called in preparation for the next
   * event. If we have been configured to wrap around, then
   * we simply re-call open and next instead of returning false
   * at the end of the file.
   *
   * @return true if another entry has been loaded
   */
  bool next();

  /**
   * number of events in this file
   * @return number of event entries
   */
  unsigned long int entries() const;

  /**
   * check if reader is open or not
   * @return true if reader is open and false otherwise
   */
  bool is_open() const;

  /**
   * get the object for the current entry under the passed name
   *
   * This is a direct call to Event::get so look there for more detailed
   * documentation.
   *
   * @tparam T type of data
   * @param[in] name name of event object
   * @param[in] pass (optional) name of pass that produced the event object
   */
  template<typename T>
  const T& get(const std::string& name, const std::string& pass = "") const {
    return event_.get<T>(name,pass);
  }
 private:
  /// event bus for this reader
  Event event_;
  /// handle of underlying reader
  std::unique_ptr<io::Reader> reader_;
  /// index of entry within file that is open
  unsigned long int i_entry_{0};
  /// do we wrap from end to beginning or throw exception?
  bool wrap_around_{false};
  /// have we started yet?
  bool in_file_{false};
};
}

#endif
