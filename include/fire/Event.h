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
   * @throw Exception if two data sets of the same name are added
   * @throw Exception if input DataType doesn't match the type stored in the
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
      obj.set_ = std::make_unique<h5::DataSet<DataType>>(h5::Reader::EVENT_GROUP+"/"+full_name);
      obj.should_save_ = keep(full_name, true);
      obj.should_load_ = false;
      obj.updated_ = false;
      products_.emplace_back(name, pass_,boost::core::demangle(typeid(DataType).name()));

      // if we are saving this object, we should save the default value for all entries
      // up to this one. This (along with 'clearing' at the end of each event) 
      // allows users to asyncronously add event objects and the events without an 'add'
      // have a 'default' or 'cleared' object value.
      if (obj.should_save_) {
        obj.set_->clear();
        for (std::size_t i{0}; i < i_entry_; i++)
          obj.set_->save(output_file_);
      }
    }

    auto& obj{objects_.at(full_name)};
    if (obj.updated_) {
      // this data set has been updated by another processor
      throw Exception("SetRepeat",
          "DataSet named " + full_name + " already added to the event.");
    }

    try {
      /// maybe throw bad_cast exception
      obj.getDataSetRef<DataType>().update(data);
      obj.updated_ = true;
    } catch (std::bad_cast const&) {
      throw Exception("TypeMismatch",
          "DataSet corresponding to " + full_name + " has different type.");
    }
  }

  /**
   * get a piece of data from the event
   *
   * @throw Exception if requested data doesn't exist
   * @throw Exception if requested DataType doesn't match type in data set
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
        throw Exception("SetMiss",
            "DataSet " + name + " of type " + type + " not found.");
      } else if (options.size() > 1) {
        throw Exception("SetAmbig",
            "DataSet " + name + " of type " + type + " is ambiguous. Provide a pass name.");
      }

      // exactly one option
      full_name = fullName(options.at(0).name(), options.at(0).pass());
      // add into cache
      known_lookups_[name] = full_name;
    }

    if (objects_.find(full_name) == objects_.end()) {
      // final check for input file, never should enter here without one
      assert(input_file_);
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
      obj.set_ = std::make_unique<h5::DataSet<DataType>>(h5::Reader::EVENT_GROUP+"/"+full_name);
      obj.should_save_ = keep(full_name, false);
      obj.should_load_ = true;
      obj.updated_ = false;
      // get this object up to the current entry
      //    loading may throw an H5 error
      for (std::size_t i{0}; i < i_entry_+1; i++)
        obj.set_->load(*input_file_);
    }

    // type casting, 'bad_cast' thrown if unable
    try {
      return objects_[full_name].getDataSetRef<DataType>().get();
    } catch (const std::bad_cast&) {
      throw Exception("BadType",
          "DataSet corresponding to " + full_name + " has different type.");
    }
  }

  /**
   * Get a test event bus.
   *
   * @note This should only be used for testing!.
   *
   * @param[in] test_file Output file to write any testing to.
   */
  static Event test(h5::Writer& test_file) {
    return Event(test_file,"test",{});
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
  Event(h5::Writer& output_file,
        const std::string& pass,
        const std::vector<config::Parameters>& dk_rules);

  /**
   * Go through and save the current in-memory objects into
   * the output file.
   */
  void save();

  /**
   * Go through and load the next entry into the in-memory
   * objects from the input file.
   */
  void load();

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
  void done();

 private:
  /// header that we control
  std::unique_ptr<EventHeader> header_;
  /// pointer to input file (nullptr if no input files)
  h5::Reader* input_file_;
  /// handle to output file (owned by Process)
  h5::Writer& output_file_;
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
    /// have we been updated on the current event?
    bool updated_;
    /**
     * Helper for getting a reference to the dataset
     *
     * @throws bad_cast if DataSet does not hold the same type
     */
    template <typename DataType>
    h5::DataSet<DataType>& getDataSetRef() {
      return dynamic_cast<h5::DataSet<DataType>&>(*set_);
    }
    /**
     * Clear the event object at the end of an event
     *
     * We reset the flag saying if this object has been
     * updated by a processor and call the set's clear.
     */
    void clear() {
      updated_ = false;
      set_->clear();
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
