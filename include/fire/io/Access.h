#ifndef FIRE_IO_ACCESS_H
#define FIRE_IO_ACCESS_H

#include "fire/version/Version.h"

namespace fire::io {

struct access {
  template <typename T, typename D>
  static void connect(T& t, D& d) {
    t.attach(d);
  }
};

}

#endif
