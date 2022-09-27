#ifndef FIRE_USERREADER_H
#define FIRE_USERREADER_H

#include "fire/io/Reader.h"
#include "fire/Event.h"

namespace fire {
class UserReader {
 public:
  /**
   * Prepare the reader for reading
   *
   * @note opening the Event bus with a NULL writer is dangerous.
   * Any development of this class needs to be done extremely carefully
   * and tested thoroughly.
   */
  UserReader();
  /// open the file, skipping n events
  void open(const std::string& fn, unsigned long int n = 0);
  /// go to the next event
  void next();
  /// number of events in this file
  unsigned long int entries() const;
  /// check if reader is open or not
  bool is_open() const;
  /// get the object for the current entry under the passed name
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
  unsigned long int i_entry_;
  /// do we wrap from end to beginning or throw exception?
  bool wrap_around_;
};
}

#endif
