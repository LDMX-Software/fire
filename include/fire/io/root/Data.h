#ifndef FIRE_IO_ROOT_DATA_H
#define FIRE_IO_ROOT_DATA_H

#include "fire/io/AbstractData.h"

namespace fire::io::root {

template<typename DataType>
class Data : public io::AbstractData<DataType> {
 public:
  virtual void load(io::Reader& r) {
    static_assert(static_cast<root::Reader*>(&r),
        "Loading into ROOT data only allowed for ROOT reader.");
    r.load(this->name_, this->handle_);
  }
  virtual void save(io::Writer& w) final override {
    static_assert(false,
        "Writing ROOT data not implemented.");
  }
};

}

#endif
