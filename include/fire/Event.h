#ifndef FIRE_EVENT_H
#define FIRE_EVENT_H

#include <regex>
#include <boost/core/demangle.hpp>

#include "fire/h5/DataSet.h"
#include "fire/ProductTag.h"
#include "fire/EventHeader.h"

namespace fire {

/// forward declaration for friendship
class Process;

/**
 * Event class for interfacing with processors
 *
 * This class is what is given to the processors to 'get' data from
 * and/or 'add' data to.
 *
 * The 'Process' class is a friend class so that it can do all
 * the orginizational work behind the scenes using private methods
 * while the public methods are the only ones available to processors
 * written by users.
 */
class Event {
 public:
  /**
   * Get the event header
   */
  const EventHeader& header() const { return *header_; }

  /**
   * Get non-const event header
   * @TODO should we name this differently than the const version?
   */
  EventHeader& header() { return *header_; }

  /**
   * Search through the products listing for a specific match
   */
  std::vector<ProductTag> search(const std::string& namematch,
                                 const std::string& passmatch,
                                 const std::string& typematch) const;

  /**
   * Check if the input name and (optional) pass exist in the bus
   * This checks for **unique** existence, i.e. it can be used to make sure
   * that a following 'get' call will only fail if the wrong type is provided.
   */
  bool exists(const std::string& name, const std::string& pass = "") const {
    return search("^" + name + "$", pass, "").size() == 1;
  }

  /**
   * add a piece of data to the event
   *
   * @throw h5::Exception if two data sets of the same name are added
   * @throw h5::Exception if input DataType doesn't match the type stored in the
   * data set
   *
   * @tparam[in] DataType type of data being added
   * @param[in] name name of data being added
   * @param[in] data actual value of data being added
   */
  template <typename DataType>
  void add(const std::string& name, const DataType& data) {
    std::string full_name{fullName(name, pass_)};
    if (objects_.find(full_name) == objects_.end()) {
      // a data set hasn't been created for this data yet
      // we good, lets create the new data set
      //
      // NOTES:
      // - check if new data is going to be written to output file 
      //   or just used during this run 
      // - without any applicable drop/keep rules, 
      //   we do save these datasets
      // - we mark these objects as should_load == false because
      //   they are new and not from an input file
      auto& obj{objects_[full_name]};
      obj.set_ = std::make_unique<h5::DataSet<DataType>>(full_name);
      obj.should_save_ = keep(full_name, true);
      obj.should_load_ = false;
      products_.emplace_back(name, pass_,boost::core::demangle(typeid(DataType).name()));
    }

    try {
      /// maybe throw bad_cast exception
      auto& s{objects_.at(full_name).getDataSetRef<DataType>()};
      if (s.updated()) {
        // this data set has been updated by another processor
        throw h5::Exception("DataSet named " + full_name +
                        " already added to the event.");
      }
      s.update(data);
    } catch (std::bad_cast const&) {
      throw h5::Exception("DataSet corresponding to " + full_name +
                      " has different type.");
    }
  }

  /**
   * get a piece of data from the event
   *
   * @throw h5::Exception if requested data doesn't exist
   * @throw h5::Exception if requested DataType doesn't match type in data set
   *
   * @tparam[in] DataType type of requested data
   * @param[in] name Name of requested data
   * @param[in] pass optional pass name to use for getting the data
   * @return const reference to data in event
   */
  template <typename DataType>
  const DataType& get(const std::string& name,
                      const std::string& pass = "") const {
    std::string full_name;
    if (not pass.empty()) {
      // easy case, pass was specified explicitly
      full_name = fullName(name, pass);
    } else if (known_lookups_.find(name) != known_lookups_.end()) {
      full_name = known_lookups_.at(name);
    } else {
      // need to search current (and potential) products using partial name
      auto type = boost::core::demangle(typeid(DataType).name());
      auto options{search("^" + name + "$", "", "^" + type + "$")};
      if (options.size() == 0) {
        throw h5::Exception("DataSet " + name + " of type " + type +
                        " not found.");
      } else if (options.size() > 1) {
        throw h5::Exception("DataSet " + name + " of type " + type +
                        " is ambiguous. Provide a pass name.");
      }

      // exactly one option
      full_name = fullName(options.at(0).name(), options.at(0).pass());
      // add into cache
      known_lookups_[name] = full_name;
    }

    if (objects_.find(full_name) == objects_.end()) {
      // check if file on disk by trying to create and load it
      //  this line won't throw an error because we haven't tried accessing the
      //  data yet
      if (!input_file_) {
        // no input file
        throw h5::Exception("DataSet named " + full_name +
                        " does not exist.");
      }
      // a data set hasn't been created for this data yet
      // we good, lets create the new data set
      //
      // NOTES:
      // - check if new data is going to be written to output file 
      //   or just used during this run 
      // - without any applicable drop/keep rules, 
      //   we do save these datasets
      // - we mark these objects as should_load == false because
      //   they are new and not from an input file
      auto& obj{objects_[full_name]};
      obj.set_ = std::make_unique<h5::DataSet<DataType>>(full_name);
      obj.should_save_ = keep(full_name, false);
      obj.should_load_ = true;
      //  this line may throw an error
      obj.set_->load(*input_file_, i_entry_);
    }

    // type casting, 'bad_cast' thrown if unable
    try {
      return objects_[full_name].getDataSetRef<DataType>().get();
    } catch (const std::bad_cast&) {
      throw h5::Exception("DataSet corresponding to " + full_name +
                      " has different type.");
    }
  }

  /**
   * Get a test event bus.
   *
   * @note This should only be used for testing!.
   */
  static Event test() {
    return Event("test",{});
  }

  /// Delete the copy constructor to prevent any in-advertent copies.
  Event(const Event&) = delete;

  /// Delete the assignment operator to prevent any in-advertent copies
  void operator=(const Event&) = delete;

 private:
  /**
   * Deduce full data set name given a pass or using our pass
   *
   * 'inline' because we call this at least once per event for each dataset,
   * having it 'inline' means that it will be in the same compilation unit
   * as where it is used and therefore will hopefully improve performance.
   */
  inline std::string fullName(const std::string& name, const std::string& pass) const {
    return (pass.empty() ? pass_ : pass) + "/" + name;
  }

  /**
   * Determine if the passed data set should be saved into output file
   *
   * We apply all of the rules **in order** i.e. the last matching
   * regex rule is the one that makes the actual decision for a data set.
   * This behavior is helpful for our use case because we can have general
   * rules and then various exceptions.
   */
  bool keep(const std::string& full_name, bool def) const;

 private:
  /**
   * The Process is the Event's friend,
   * this allows the Process to handle the core iteration procedure
   * while preventing other classes from accessing these methods.
   *
   * While the Process class has access to all of Event's private
   * members, the methods below are the ones used by it.
   */
  friend class Process;

  /**
   * Default constructor
   * Only our good friend Process can construct us.
   *
   * Besides the name of the pass, the input_file handle is
   * initialized to a nullptr to signify that no input file has
   * been "registered" yet and i_entry_ is started at zero.
   *
   * Pass drop/keep rules for determining if a data set is going
   * to be written to the output file or not.
   *
   * The regex grammar is set to "extended", case is ignored, and
   * the 'nosubs' parameter is passed.
   *
   * @param[in] pass name of current processing pass
   * @param[in] dk_rules configuration for the drop/keep rules
   */
  Event(const std::string& pass,
        const std::vector<config::Parameters>& dk_rules);

  /**
   * Go through and save the current in-memory objects into
   * the output file at the input index.
   *
   * We call the 'checkThenSave' method of the base data set class.
   * This class checks if the data set was marked as should be saved
   * when it was created.
   *
   * @param[in] f output HDF5 file to write to
   * @param[in] i index of dataset to write to
   */
  void save(h5::Writer& w, unsigned long int i);

  /**
   * Go through and load the input index into the in-memory
   * objects from the input file.
   *
   * @param[in] f input HDF5 file to read from
   * @param[in] i index of dataset to read from
   */
  void load(h5::Reader& r, unsigned long int i);

  /**
   * Attach a HDF5 file to this event as the input file
   *
   * @note We store the input file as a pointer, but
   * don't clean-up later.
   *
   * @param[in] r reference to HDF5 input file reader
   */
  void setInputFile(h5::Reader& r);

  /**
   * Move to the next event
   *  we just need to keep our entry index up-to-date
   *  for loading a newly requested object
   *
   *  and reset the other event objects to their empty state.
   */
  void next();

  /**
   * Let the event bus know we are done
   *
   * This gives us an opportunity to go through the products in the output file
   * and attach the 'type' attributes to them
   */
  void done(h5::Writer& w);

 private:
  /// header that we control
  std::unique_ptr<EventHeader> header_;
  /// pointer to input file (maybe nullptr)
  h5::Reader* input_file_;
  /// name of current processing pass
  std::string pass_;
  /**
   * structure to hold event data in memory
   */
  struct EventObject {
    /// the dataset for save/load the data
    std::unique_ptr<h5::BaseDataSet> set_;
    /// should we save the data into output file?
    bool should_save_;
    /// should we load the data from the input file?
    bool should_load_;
    /**
     * Helper for getting a reference to the dataset
     *
     * @throws bad_cast if DataSet does not hold the same type
     */
    template <typename DataType>
    h5::DataSet<DataType>& getDataSetRef() {
      return dynamic_cast<h5::DataSet<DataType>&>(*set_);
    }
  };
  /// list of event objects being processed
  mutable std::unordered_map<std::string, EventObject> objects_;
  /// current index in the datasets
  long unsigned int i_entry_;
  /// regular expressions determining if a dataset should be written to output
  /// file
  std::vector<std::pair<std::regex, bool>> drop_keep_rules_;
  /// list of products available to us either on disk or newly created
  std::vector<ProductTag> products_;
  /// cache of known lookups when requesting an object without a pass name
  mutable std::unordered_map<std::string, std::string> known_lookups_;
};  // Event

}  // namespace fire

#endif  // FIRE_EVENT_H
