#include "fire/io/root/Reader.h"

#include <sstream>

#include "fire/io/Constants.h"

namespace fire::io::root {

Reader::Reader(const std::string& file_name)
  : file_{TFile::Open(file_name.c_str())},
    ::fire::io::Reader(file_name) {
    event_tree_ = static_cast<TTree*>(file_->Get("LDMX_Events"));
    if (event_tree_ == 0) {
      throw Exception("BadFile",
          "TTree named 'LDMX_Events' does not exist in input ROOT file "+
          file_name+".");
    }

    run_tree_ = static_cast<TTree*>(file_->Get("LDMX_Runs"));
    if (run_tree_ == 0) {
      throw Exception("BadFile",
          "TTree named 'LDMX_Runs' does not exist in input ROOT file "+
          file_name+".");
    }
}

void Reader::load_into(BaseData& d) {
  d.load(*this);
}

std::vector<std::array<std::string,3>> Reader::availableObjects() {
  std::vector<std::array<std::string,3>> objs;
  // find the names of all the existing branches
  TObjArray* branches = event_tree_->GetListOfBranches();
  for (int i = 0; i < branches->GetEntriesFast(); i++) {
    std::string brname = branches->At(i)->GetName();
    if (brname != "EventHeader") { //ldmx::EventHeader::BRANCH) {
      size_t j = brname.find("_");
      auto br = dynamic_cast<TBranchElement*>(branches->At(i));
      // can't determine type if branch isn't
      //  the higher-level TBranchElement type
      // Only occurs if the type on the bus is one of:
      //  bool, short, int, long, float, double (BSILFD)
      std::array<std::string,3> obj = {
          brname.substr(0, j),   // object name is before '_'
          brname.substr(j + 1),  // pass name is after
          br ? br->GetClassName() : "BSILFD"
          };
      objs.push_back(obj);
    }
  }
  return objs;
}

std::string Reader::name() const {
  return file_->GetName();
}

std::size_t Reader::entries() const {
  return event_tree_->GetEntriesFast();
}

std::size_t Reader::runs() const {
  return run_tree_->GetEntriesFast();
}

std::string Reader::transform(const std::string& dir_name) {
  static std::map<std::string,std::string> cache;
  if (cache.find(dir_name) == cache.end()) {
    // split dir_name into event_group, pass, obj
    std::string dir;
    std::stringstream dir_name_ss{dir_name};
    std::vector<std::string> dirs;
    while (std::getline(dir_name_ss, dir, '/')) {
      // skip event group directory
      if (dir == constants::EVENT_GROUP)
        continue;
      dirs.push_back(dir);
    }
    if (dirs.size() != 2) {
      throw Exception("MalformDir",
          "ROOT Reader received malformed directory "+dir_name);
    }
    // inside event group, the structure looks like pass/obj
    //  and we want it to be obj_pass
    cache[dir_name] = dirs.at(1)+"_"+dirs.at(0);
  }
  return cache.at(dir_name);
}

}

namespace {
auto v = ::fire::io::Reader::Factory::get().declare<fire::io::root::Reader>();
}
