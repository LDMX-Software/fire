#include "fire/io/Open.h"

#include <map>

namespace fire::io {

std::unique_ptr<io::Reader> open(const std::string& fp) {
  static const std::map<std::string, std::string> ext_to_type = {
    { "root", "fire::io::root::Reader" },
    { "hdf5", "fire::io::h5::Reader" },
    { "h5"  , "fire::io::h5::Reader" }
  };
  auto ext{fp.substr(fp.find_last_of('.')+1)};
  try {
    return io::Reader::Factory::get().make(ext_to_type.at(ext), fp);
  } catch (const std::out_of_range&) {
    throw Exception("BadExt",
        "Unrecognized extension '"+ext+"' for input file "+fp+".");
  }
}

}
