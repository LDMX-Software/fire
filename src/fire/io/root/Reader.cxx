#include "fire/io/root/Reader.h"

#include <sstream>

#include "TSystem.h"
#include "TBranchElement.h"
#include "TLeaf.h"

#include "fire/io/Constants.h"

namespace fire::io::root {

Reader::Reader(const std::string& file_name)
  : file_{TFile::Open(file_name.c_str())},
    ::fire::io::Reader(file_name) {

  if (file_ == 0) {
    throw Exception("NoFile",
        "File named "+file_name+" could not be opened by ROOT.");
  }

  event_tree_ = static_cast<TTree*>(file_->Get("LDMX_Events"));
  if (event_tree_ == 0) {
    throw Exception("BadFile",
        "TTree named 'LDMX_Events' does not exist in input ROOT file "+
        file_name+".");
  }

  run_tree_ = static_cast<TTree*>(file_->Get("LDMX_Run"));
  if (run_tree_ == 0) {
    throw Exception("BadFile",
        "TTree named 'LDMX_Run' does not exist in input ROOT file "+
        file_name+".");
  }

  // don't allow ROOT to handle unix failure signals
  for (int s{0}; s < kMAXSIGNALS; s++) gSystem->ResetSignal((ESignals)s);
}

void Reader::load_into(BaseData& d) {
  d.load(*this);
}

std::vector<std::pair<std::string,std::string>> Reader::availableObjects() {
  std::vector<std::pair<std::string,std::string>> objs;
  // find the names of all the existing branches
  TObjArray* branches = event_tree_->GetListOfBranches();
  for (int i = 0; i < branches->GetEntriesFast(); i++) {
    std::string brname = branches->At(i)->GetName();
    if (brname != "EventHeader") {
      size_t j = brname.find("_");
      objs.emplace_back(brname.substr(0,j),brname.substr(j+1));
    }
  }
  return objs;
}

std::pair<std::string,int> Reader::type(const std::string& path) {
  std::string branch_name;
  try {
    branch_name = transform(path);
  } catch (const Exception& e) {
    // silence exception but return undefined pair
    return std::make_pair("",-1);
  }
  TTree* tree{event_tree_};
  if (branch_name == "RunHeader") tree = run_tree_;
  auto br = tree->GetBranch(branch_name.c_str());
  if (not br) return std::make_pair("",-1);
  std::string type;
  int vers{0};
  if (auto bre = dynamic_cast<TBranchElement*>(br)) {
    type = bre->GetClassName();
    vers = bre->GetClass()->GetClassVersion();
  } else {
    if (br->GetListOfLeaves()->GetEntries() > 0) {
      auto leaf{(TLeaf*)br->GetListOfLeaves()->At(0)};
      type = leaf->GetTypeName();
    }
    vers = -1;
  }
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
  // static cache initialized with objects we already know the locations of
  static std::map<std::string,std::string> cache = {
    { constants::RUN_HEADER_NAME , "RunHeader" },
    { constants::EVENT_GROUP+"/"+constants::EVENT_HEADER_NAME , constants::EVENT_HEADER_NAME }
  };
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
