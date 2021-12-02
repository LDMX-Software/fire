#include "fire/ProductTag.h"

std::ostream& operator<<(std::ostream& s, const fire::ProductTag& pt) {
  return s << pt.name() << "_" << pt.passname() << "_" << pt.type();
}
