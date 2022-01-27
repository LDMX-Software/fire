#include "fire/h5/Atomic.h"

namespace fire::h5 {

HighFive::EnumType<Bool> create_enum_bool() {
  return {{"TRUE" , Bool::TRUE },
          {"FALSE", Bool::FALSE}};
}

}  // namespace fire::h5

/// register our enum type with HighFive
HIGHFIVE_REGISTER_TYPE(fire::h5::Bool, fire::h5::create_enum_bool)

