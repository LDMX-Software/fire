#ifndef FIRE_IO_ROOT_READER_H_
#define FIRE_IO_ROOT_READER_H_

#include <set>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "fire/io/Reader.h"
#include "fire/exception/Exception.h"

/**
 * Reading of ROOT TTree files generated by ROOT-based Framework.
 *
 * The [ROOT-based Framework](https://github.com/LDMX-Software/Framework)
 * is a framework with similar design goals as fire, but built
 * using [CERN's ROOT](root.cern.ch) as its core serialization library.
 */
namespace fire::io::root {

/**
 * Reading ROOT files into our data structures
 *
 * If you are familiar with the ROOT-based Framework,
 * this Reader effectively does tasks done by the 
 * framework::Event and framework::EventFile classes.
 */
class Reader : public ::fire::io::Reader {
 public:
  /**
   * open input file for reading
   *
   * @throws Exception if file cannot be opened, a TTree named 'LDMX_Events'
   * is not found, or if a TTree named 'LDMX_Run' is not found.
   *
   * After opening the file and retrieve the TTrees we will read,
   * we turn off ROOT's "feature" of handling standard Unix signals.
   *
   * @param[in] file_name file to open with ROOT
   */
  Reader(const std::string& file_name);

  /**
   * Following the instructions in ::fire::io::Reader, we simply call the BaseData's
   * load function.
   *
   * @param[in] d Data to load data into
   */
  virtual void load_into(BaseData& d) final override;

  /**
   * Get the event objects available in the file
   *
   * We loop through the branches of the event TTree and parse their
   * type names, object name, and pass name. We skip the branch named
   * "EventHeader" to achieve the same behavior as h5::Reader.
   *
   * @return vector of string pairs `{ obj_name, pass }`
   */
  virtual std::vector<std::pair<std::string,std::string>> availableObjects() final override;

  /**
   * Get the type of the input object
   */
  virtual std::pair<std::string, int> type(const std::string& path) final override;

  /**
   * Get the name of the file
   *
   * We use ROOT's TFile rather than storing the name of the file ourselves.
   *
   * @return full name of file currently reading
   */
  virtual std::string name() const final override;

  /**
   * Number of entries in the file
   *
   * We get this from the number of entries in the event ttree.
   *
   * @return number of entries in the file
   */
  virtual std::size_t entries() const final override;

  /**
   * Number of runs in the file
   *
   * We get this from the number of runs in the run ttree.
   *
   * @return number of runs in the file
   */
  virtual std::size_t runs() const final override;

  /**
   * Load the data at the input name into the input object
   *
   * @see transform for how we get the ROOT branch name
   * from the input HDF5-style nested group names.
   *
   * We choose the tree from the deduced branch name. 
   * If the branch name is "RunHeader", the we use the run tree ; 
   * otherwise, the event tree.
   *
   * If the branch name isn't found in our 'attached_' map,
   * then we use attach to attempt to attach it.
   *
   * At the end, we load the data into memory using TBranch
   * GetReadEntry to retrieve the current entry and increment it
   * before providing it back to GetEntry.
   *
   * @tparam DataType type of data to load into
   * @param[in] name name of data to load
   * @param[in] obj reference to load into
   */
  template <typename DataType>
  void load(const std::string& name, DataType& obj) {
    // transform h5 directory-style name into branchname
    // of ROOT-based framework
    std::string branch_name{transform(name)};
    if (attached_.find(branch_name) == attached_.end()) {
      TTree* tree{event_tree_};
      if (branch_name == "RunHeader") tree = run_tree_;
      attached_[branch_name] = attach(tree,branch_name,obj);
    }
    if (attached_[branch_name]->GetEntry(attached_[branch_name]->GetReadEntry()+1) < 0) {
      throw Exception("BadType",
          "ROOT was unable to deserialize the branch "+branch_name
          +" into the passed type "
          +boost::core::demangle(typeid(DataType).name())
          +".\n  This error is usually caused by not providing the correct"
          " ROOT dictionary for the type you are attempting to read.",false);
    }
  }

  /// no copy constructor of readers
  Reader(const Reader&) = delete;
  /// no copy assignment of readers
  void operator=(const Reader&) = delete;
 private:
  /**
   * pull out pass and object name, construct branch name
   *
   * static caching map since this function is called so
   * frequently
   *
   * @param[in] dir_name HDF5-style (<pass>/<object>) group name of event object
   * @return branch name in ROOT-framework-style (<object>_<pass>)
   */
  static std::string transform(const std::string& dir_name);

  /**
   * Attach the input object to the TTree on the given branch
   *
   * @throws Exception if branch cannot be found.
   *
   * If the type is arithmetic (numeric_limits are specialized for it),
   * then we use the SetAddress function for attaching to the branch;
   * otherwise, we can use the SetObject method.
   *
   * @tparam T type of object to attach
   * @param[in] tree TTree to attach to
   * @param[in] branch_name name of branch to attach to
   * @param[in] obj reference to object to attach
   * @return pointer to branch that we attached to
   */
  template<typename T>
  TBranch* attach(TTree* tree, const std::string& branch_name, T& obj) {
    TBranch* br = tree->GetBranch(branch_name.c_str());
    if (br == 0) {
      throw Exception("NotFound", 
          "Branch "+branch_name+" not found in input ROOT file.");
    }
    if constexpr (std::numeric_limits<T>::is_specialized) {
      br->SetAddress(&obj);
    } else {
      br->SetObject(&obj);
    }
    br->SetStatus(1);
    return br;
  }
 private:
  /// file being read
  TFile* file_;
  /// tree of events in the file
  TTree* event_tree_;
  /// tree of runs in the file
  TTree* run_tree_;
  // branches that have been attached
  std::map<std::string, TBranch*> attached_;
};  // Reader

}  // namespace fire::io::root

#endif
