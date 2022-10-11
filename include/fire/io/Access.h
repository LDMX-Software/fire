#ifndef FIRE_IO_ACCESS_H
#define FIRE_IO_ACCESS_H

namespace fire::io {

struct access {
  template <typename T, typename D>
  static void connect(T& t, D& d) {
    t.attach(d);
  }
};

}

#endif
