#ifndef FIRE_IO_READER_H
#define FIRE_IO_READER_H

#include "fire/factory/Factory.h"

/**
 * Disk input/output namespace
 */
namespace fire::io {

class Reader {
 public:
  Reader(const std::string& file_name) {}
  virtual std::string name() const = 0;
  virtual std::size_t entries() const = 0;
  virtual std::size_t runs() const = 0;
  virtual std::string getTypeName(const std::string& obj_name) const = 0;
  virtual ~Reader() = default;
  using Factory = ::fire::factory::Factory<Reader, std::unique_ptr<Reader>, const std::string&>;
};

}

#endif

