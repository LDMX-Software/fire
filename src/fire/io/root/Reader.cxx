#include "fire/io/root/Reader.h"

namespace fire::io::root {

Reader::Reader(const std::string& file_name)
  : file_{TFile::Open(file_name.c_str())} {

    tree_ = static_cast<TTree*>(file_->Get("LDMX_Events"));
    if (tree_ == 0) {
      throw Exception("BadFile",
          "TTree named 'LDMX_Events' does not exist in input ROOT file "+
          file_name+".");
    }
}

}
