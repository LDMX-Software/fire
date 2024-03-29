#ifndef FIRE_IO_READER_H
#define FIRE_IO_READER_H

#include <iostream>
#include <vector>

#include "fire/factory/Factory.h"
#include "fire/io/AbstractData.h"

/**
 * Disk input/output namespace
 */
namespace fire::io {

/**
 * Prototype for reading files within fire
 *
 * This is a prototype class in the sense of fire::factory::Factory,
 * derived classes can register themselves and then the fire::Process
 * can use io::Reader::Factory to create readers from their types.
 *
 * A reader has a lot of responsiblities in fire, so developing a new
 * one should not be taken lightly.
 *
 * Besides deriving this class, additional io::Data::load methods need
 * to be defined so that the Reader and successfully interact with the 
 * in-memory data objects.
 */
class Reader {
 public:
  /**
   * open the file at the passed location
   *
   * @param[in] file_name full path to file to open
   */
  Reader(const std::string& file_name) {}

  /**
   * virtual destructor so derived classes can be closed
   */
  virtual ~Reader() = default;

  /**
   * Load the current event into the passed data object
   *
   * This _must_ be defined in derived classes to be
   * ```cpp
   * virtual void load_into(BaseData& d) final override {
   *   d.load(*this);
   * }
   * ```
   *
   * We require this function so that the different implementations
   * of io::Data::load are correctly called based on the type of
   * the derived reader.
   *
   * @param[in] d data object to load into
   */
  virtual void load_into(BaseData& d) = 0;

  /**
   * Return the name of the file
   * @return name of file
   */
  virtual std::string name() const = 0;

  /**
   * Return the number of events in the file
   * @return number of events in the file
   */
  virtual std::size_t entries() const = 0;

  /**
   * Return the number of runs in the file
   * @return number of runs in the file
   */
  virtual std::size_t runs() const = 0;

  /**
   * Get the event objects available in the file
   * @return vector of string pairs `{ obj_name, pass }`
   */
  virtual std::vector<std::pair<std::string,std::string>> availableObjects() = 0;

  /**
   * Get the type of the input object
   * @return pair of string type of input full object name and its version
   */
  virtual std::pair<std::string,int> type(const std::string& path) = 0;

  /**
   * Event::get needs to know if the reader implements a copy that advances
   * the entry index of the data sets being read
   *
   * @return true if the reader implements a copy
   */
  virtual bool canCopy() const {
    return false;
  }

  /**
   * Copy the input object into the output file
   * @param[in] i_entry the entry index that is being copied from this reader to output
   * @param[in] path the full object path that should be copied
   * @param[out] output the writer we should write the copy to
   */
  virtual void copy(long unsigned int i_entry, const std::string& path, Writer& output) {
    std::cerr << "[ WARN ] : " << path << " is supposed to be kept but has not been accessed"
      " with Event::get so it is not being written to the output file." << std::endl;
  }

  /**
   * Type of factory used to create readers
   */
  using Factory = ::fire::factory::Factory<Reader, std::unique_ptr<Reader>, const std::string&>;
};

}

#endif

