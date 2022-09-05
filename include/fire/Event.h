#ifndef FIRE_EVENT_H
#define FIRE_EVENT_H

#include <regex>
#include <boost/core/demangle.hpp>

#include "fire/io/Data.h"
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
   * Identification for a specific event object.
   *
   * Each event object can be uniquely identified with three pieces of information:
   * 1. It's name (the string input with the object to Event::add)
   * 2. The name of the pass it was generated on
   * 3. The name of its type
   *
   * In fact, this information _overcontrains_ the list of objects because
   * two objects in the same pass are not allowed to have the same name and
   * two objects of the same name are not allowed to have different types.
   */
  class EventObjectTag {
   public:
    /**
     * Wrap the three pieces of information in our class
     * @param name name of event object
     * @param pass pass name
     * @param type name of type
     * @param keep if we should write this object into the output file
     */
    EventObjectTag(const std::string& name, const std::string& pass,
                   const std::string& type, bool keep)
      : name_{name}, pass_{pass}, type_{type}, keep_{keep} {}

    /**
     * Pass the three pieces of information to our class via an array.
     *
     * This is used by io::Reader to pass information from the various
     * types of readers to us in setInputFile.
     *
     * @param[in] obj array of name, pass, typename (that order)
     * @param keep if we should write this object into the output file
     */
    EventObjectTag(std::array<std::string,3> obj, bool keep)
      : EventObjectTag(obj[0],obj[1],obj[2],keep) {}
  
    /**
     * Get the object name
     * @return name of event object
     */
    const std::string& name() const { return name_; }
  
    /**
     * Get the pass name the object was produced on
     * @return pass name
     */
    const std::string& pass() const { return pass_; }
  
    /**
     * Get the name of the type of the object
     * @return demangled type name
     */
    const std::string& type() const { return type_; }

    /**
     * Get if this object will be kept (i.e. written to the output file)
     * @return true if object will be written
     */
    const bool keep() const { return keep_; }

    /**
     * Get if this object is currently loaded in memory
     * @return true if there is a memory representation of this object
     */
    const bool loaded() const { return loaded_; }
  
    /**
     * String method for printing this tag in a helpful manner
     */
    friend std::ostream& operator<<(std::ostream& os, const EventObjectTag& t) {
      return os << "Object(pass=" << t.pass() <<", name=" << t.name() << "type=" << t.type() << ")";
    }
  
    /**
     * Checks if we match the passed regex for name, pass, and type
     */
    inline bool match(const std::regex& name, const std::regex& pass, const std::regex& type) const {
      return std::regex_match(name_, name) and std::regex_match(pass_, pass) and std::regex_match(type_, type);
    }
  
   private:
    /**
     * friends with our parent class Event so that it can modify our members after-the-fact
     *
     * (mainly so that we can change the loaded_ boolean when an object transitions from 
     * being available to being in-memory)
     */
    friend class Event;

    /**
     * Name given to the object
     */
    std::string name_;
  
    /**
     * Pass name given when object was generated
     */
    std::string pass_;
  
    /**
     * Type name of the object
     */
    std::string type_;

    /**
     * If the object represented by this tag should be kept
     *
     * kept means written to output file
     */
    bool keep_;

    /**
     * If the object represented by this tag has been loaded
     * into a memory object.
     *
     * We need to be mutable so that we can be changed during
     * Event::get which is a const member function.
     */
    mutable bool loaded_{false};
  };

 public:
  /**
   * Get the event header
   * @return const reference to event header
   */
  const EventHeader& header() const { return *header_; }

  /**
   * Get non-const event header
   *
   * @note should we name this differently than the const version?
   * Having them have the same name means that a (potentially helpful)
   * compiler error about const-ness would never be thrown and the
   * compiler will automatically deduce which to use (perferring const).
   *
   * @return reference to event header
   */
  EventHeader& header() { return *header_; }

  /**
   * Search through the available objects for a specific match
   *
   * This can be helpful for higher-level processors that require
   * some amount of introsepction. Additionally, the EventObjectTag
   * has the demangled type name for a specific event object, so
   * the processor could use this functionality to more dynamically
   * handle various types of event objects.
   *
   * An empty matching string is interpreted as the "match anything"
   * regex '.*'.
   *
   * @param[in] namematch regex to match name of event object
   * @param[in] passmatch regex to match pass of event object
   * @param[in] typematch regex to match demangled type of event object
   * @return list of event object tags that match all of the regex
   */
  std::vector<EventObjectTag> search(const std::string& namematch,
                                 const std::string& passmatch,
                                 const std::string& typematch) const;

  /**
   * Check if the input name and (optional) pass exist in the bus
   *
   * @note This checks for **unique** existence, i.e. it can be used to make sure
   * that a following 'get' call will only fail if the wrong type is provided.
   * In order to check for any existence, use Event::search.
   *
   * @param[in] name Name of event object
   * @param[in] pass (optional) name of pass that created the event object
   * @return true if there is exactly one available object matching the name
   * and pass
   */
  bool exists(const std::string& name, const std::string& pass = "") const {
    return search("^" + name + "$", pass, "").size() == 1;
  }

  /**
   * add a piece of data to the event
   *
   * If the data isn't already in the list of in-memory objects,
   * we do the initialization procedure defined below. After init,
   * we check if the object has already been updated by another processor,
   * and if it hasn't, we give the input data to the event object to update
   * the in-memory copy.
   *
   * ### Initilialization
   * To initilialize a new in-memory object, we first check that a object
   * with the same name and pass doesn't exist in the available objects.
   * This prevents processors from silently overwriting data that could
   * have been read in from the input file. Then, we create a new event object
   * to hold the in-memory data, wrapping the event object with h5::Data.
   * We also set the event object flags.
   *  should_save : determined by Event::keep with the default set to true
   *  should_load : false, this is a new object and is not being read in
   *  updated : false, we haven't updated it yet
   *
   * Finally, if we end up needing to save this object, we also making
   * sure to call h5::Data::save enough times to align the number of 
   * entries in the serialized data with the current number of entries
   * we are on (Event::i_entry_).
   *
   * @throw Exception if two data sets of the same name and the same pass 
   * are added
   * @throw Exception if input DataType doesn't match the type stored in the
   * data set
   *
   * @tparam DataType type of data being added
   * @param[in] name name of data being added
   * @param[in] data actual value of data being added
   */
  template <typename DataType>
  void add(const std::string& name, const DataType& data) {
    static const bool ADD_KEEP_DEFAULT = true;
    std::string full_name{fullName(name, pass_)};
    if (objects_.find(full_name) == objects_.end()) {
      // check available_objects_ listing so we don't in-advertently replace 
      //   any datasets of the same name read in from the inputfile
      // we know we are worried about data from the input file
      //   because data from previous producers in the sequence
      //   would already exist in the objects_ map
      // we rely on trusting that setInputFile gets the listing
      //   of event objects from the input file and puts them
      //   into availble_objects_
      if (search("^"+name+"$","^"+pass_+"$",".*").size() > 0) {
        throw Exception("Repeat",
            "Data named "+full_name+" already exists in the input file.");
      }
      auto& tag{available_objects_.emplace_back(name, pass_,
                  boost::core::demangle(typeid(DataType).name()),
                  keep(full_name, ADD_KEEP_DEFAULT))};
      tag.loaded_ = true;

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
      obj.data_ = std::make_unique<io::Data<DataType>>(io::constants::EVENT_GROUP+"/"+full_name);
      obj.should_save_ = tag.keep_;
      obj.should_load_ = false;
      obj.updated_ = false;

      // if we are saving this object, we should save the default value for all entries
      // up to this one. This (along with 'clearing' at the end of each event) 
      // allows users to asyncronously add event objects and the events without an 'add'
      // have a 'default' or 'cleared' object value.
      if (obj.should_save_) {
        obj.data_->clear();
        for (std::size_t i{0}; i < i_entry_; i++)
          obj.data_->save(output_file_);
      }
    }

    auto& obj{objects_.at(full_name)};
    if (obj.updated_) {
      // this data set has been updated by another processor in the sequence
      throw Exception("Repeat",
          "Data named " + full_name + " already added to the event"
          " by a previous producer in the sequence.");
    }

    try {
      /// maybe throw bad_cast exception
      obj.getDataRef<DataType>().update(data);
      obj.updated_ = true;
    } catch (std::bad_cast const&) {
      throw Exception("TypeMismatch",
          "Data corresponding to " + full_name + " has different type.");
    }
  }

  /**
   * get a piece of data from the event
   *
   * ### Object Deduction
   * Optionally providing a pass name means we need some automatic
   * deduction of what the pass name should be. If the pass name is
   * provided, all of the deduction procedure is skippped. A cache
   * of known name -> name+pass deductions is kept in Event::known_lookups_
   * to save time. The deduction is pretty simple, we use
   * Event::search with the input name and the demangled input type
   * in order to retrieve a list of options. If the list of options
   * is empty or has more than one element, we throw an exception, 
   * otherwise, we have successfully deduced the pass name.
   *
   * If the requested object is not in the list of in-memory objects,
   * then we **must** have an input file. This assumption is enforced
   * with an exception the pointer to the input file is `nullptr`.
   * If we do have an input file, then we create a new in-memory object
   * to read this data set. After wrapping the input data type with
   * h5::Data, we deduce the other tags:
   *  should_save: use Event::keep with the default of `false`
   *  should_load: true since this is a reading 
   *  updated: false
   * We also use h5::Data::load to get the reading pointer to the entry
   * in the data set corresponding to the current entry we are on
   * (Event::i_entry_).
   *
   * After all of this setup, we attempt to retrieve the a constant
   * reference to the data stored in the in-memory object.
   *
   * @throw Exception if requested data doesn't exist
   * @throw Exception if requested DataType doesn't match type in data set
   *
   * @tparam DataType type of requested data
   * @param[in] name Name of requested data
   * @param[in] pass optional pass name to use for getting the data
   * @return const reference to data in event
   */
  template <typename DataType>
  const DataType& get(const std::string& name,
                      const std::string& pass = "") const {
    std::string full_name, type;
    if (not pass.empty()) {
      // easy case, pass was specified explicitly
      full_name = fullName(name, pass);
    } else if (known_lookups_.find(name) != known_lookups_.end()) {
      full_name = known_lookups_.at(name);
    } else {
      // need to search current (and potential) available_objects using partial name
      auto options{search("^" + name + "$", ".*", ".*")};
      if (options.size() == 0) {
        throw Exception("Miss",
            "Data " + name + " not found.");
      } else if (options.size() > 1) {
        throw Exception("Ambig",
            "Data " + name + " is ambiguous. Provide a pass name.");
      }

      // exactly one option
      full_name = fullName(options.at(0).name(), options.at(0).pass());
      type = options.at(0).type();
      // add into cache
      known_lookups_[name] = full_name;
    }

    if (objects_.find(full_name) == objects_.end()) {
      // final check for input file, never should enter here without one
      if (not input_file_) {
        throw Exception("Miss",
            "Data " + full_name + " was not created by an earlier processor "
            "and there is not input file to attempt to read it from.");
      }
      
      // when setting up the in-memory object, we need to find the tag so we can
      // 1. get whether the object should be copied to the output file and
      // 2. set the loaded_ flag to true
      // we can do a simple linear search since this will only happen once per input file
      auto tag_it = std::find_if(available_objects_.begin(), available_objects_.end(), 
          [&full_name,this](const EventObjectTag& tag) {
            return fullName(tag.name(),tag.pass()) == full_name;
          });
      tag_it->loaded_ = true;

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
      obj.data_ = std::make_unique<io::Data<DataType>>(io::constants::EVENT_GROUP+"/"+full_name);
      obj.should_save_ = tag_it->keep();
      obj.should_load_ = true;
      obj.updated_ = false;

      // get this object up to the current entry
      //    loading may throw an H5 error if the shape of the data on disk
      //    cannot be loaded into the input type
      try {
        if (not obj.should_save_ or not input_file_->canCopy()) {
          // only skip the first i_entry_ entries if the input file cannot copy
          // or the object is not being saved
          //  the objects that are being saved are being mirrored by the input file
          //  if the input file can copy
          for (std::size_t i{0}; i < i_entry_; i++) input_file_->load_into(*obj.data_);
        }
        input_file_->load_into(*obj.data_);
      } catch (const HighFive::DataSetException&) {
        throw Exception("BadType",
            "Data " + full_name + " could not be loaded into "
            + boost::core::demangle(typeid(DataType).name())
            + " from the type it was written as " + type);
      }
    }

    // type casting, 'bad_cast' thrown if unable
    try {
      return objects_[full_name].getDataRef<DataType>().get();
    } catch (const std::bad_cast&) {
      throw Exception("BadType",
          "Data " + full_name + " was initialy loaded with type " + type
          + " which cannot be casted into " 
          + boost::core::demangle(typeid(DataType).name()));
    }
  }

  /**
   * Get a test event bus.
   *
   * @note This should only be used for testing!.
   *
   * @param[in] test_file Output file to write any testing to.
   */
  static Event test(io::Writer& test_file) {
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
   *
   * @param[in] name object name
   * @param[in] pass pass name, if empty use current pass
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
   *
   * @param[in] full_name object name including the pass prefix
   * @param[in] def default drop/keep decision value
   * @return true if object should be saved into the output file
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
  Event(io::Writer& output_file,
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
   * Attach a file to this event as the input file
   *
   * @note We store the input file as a pointer, but
   * don't clean-up later.
   *
   * @param[in] r reference to input file reader
   */
  void setInputFile(io::Reader* r);

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
  io::Reader* input_file_;
  /// handle to output file (owned by Process)
  io::Writer& output_file_;
  /// name of current processing pass
  std::string pass_;
  /**
   * structure to hold event data in memory
   */
  struct EventObject {
    /**
     * the data for save/load
     */
    std::unique_ptr<io::BaseData> data_;
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
    io::Data<DataType>& getDataRef() {
      return dynamic_cast<io::Data<DataType>&>(*data_);
    }
    /**
     * Clear the event object at the end of an event
     *
     * We reset the flag saying if this object has been
     * updated by a processor and call the set's clear.
     */
    void clear() {
      updated_ = false;
      data_->clear();
    }
  };
  /// list of event objects being processed
  mutable std::unordered_map<std::string, EventObject> objects_;
  /// current index in the datasets
  long unsigned int i_entry_;
  /// regular expressions determining if a dataset should be written to output
  /// file
  std::vector<std::pair<std::regex, bool>> drop_keep_rules_;
  /// list of objects available to us either on disk or newly created
  std::vector<EventObjectTag> available_objects_;
  /// cache of known lookups when requesting an object without a pass name
  mutable std::unordered_map<std::string, std::string> known_lookups_;
};  // Event

}  // namespace fire

#endif  // FIRE_EVENT_H
