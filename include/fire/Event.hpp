#ifndef FIRE_EVENT_HPP
#define FIRE_EVENT_HPP

#include <exception>

#include "fire/h5/DataSet.hpp"

namespace fire {

/// forward declaration for friendship
class Process;

/**
 * Event class for interfacing with processors
 *
 * This class is what is given to the processors to 'get' data from
 * and/or 'add' data to.
 *
 * The 'File' class is a friend class so that it can register
 * itself with the Event if it is an input file and it can iterate
 * the current entry index by calling 'Event::next'.
 */
class Event {
 public:
  /**
   * Default constructor
   *
   * Besides the name of the pass, the input_file handle is
   * initialized to a nullptr to signify that no input file has
   * been "registered" yet and i_entry_ is started at zero.
   *
   * @param[in] pass name of current processing pass
   */
  Event(const std::string& pass) : pass_{pass}, input_file_{nullptr}, i_entry_{0} {}

  /**
   * add a piece of data to the event
   *
   * @TODO check if two add's of the same name are done per event
   * @TODO add in handling of the pass name
   * @throw Exception if input DataType doesn't match the type stored in the data set
   *
   * @tparam[in] DataType type of data being added
   * @param[in] name name of data being added
   * @param[in] data actual value of data being added
   */
  template <typename DataType>
  void add(std::string const& name, DataType& data) {
    auto set_it{sets_.find(name)};
    if (set_it == sets_.end()) {
      // TODO prevent two add's of same 'name' per event
      sets_[name] = std::make_unique<h5::DataSet<DataType>>(name);
    }
    try {
      /// maybe throw bad_cast exception
      dynamic_cast<h5::DataSet<DataType>&>(*sets_[name]).update(data);
    } catch (std::bad_cast const&) {
      throw std::runtime_error("DataSet corresponding to " + name +
                               " has different type.");
    }
  }

  /**
   * get a piece of data from the event
   *
   * @TODO add in handling of the pass name
   *
   * @throw Exception if requested data doesn't exist
   * @throw Exception if requested DataType doesn't match type in data set
   *
   * @tparam[in] DataType type of requested data
   * @param[in] name Name of requested data
   * @return const reference to data in event
   */
  template <typename DataType>
  DataType const& get(std::string const& name) {
    auto set_it{sets_.find(name)};
    if (set_it == sets_.end()) {
      // check if file on disk by trying to create and load it
      //  this line won't throw an error because we haven't tried accessing the
      //  data yet
      if (!input_file_) {
        // no input file
        throw std::runtime_error("DataSet named "+name+" does not exist.");
      }
      sets_[name] = std::make_unique<h5::DataSet<DataType>>(name);
      //  this line may throw an error
      sets_[name]->load(*input_file_, i_entry_);
    }

    // type casting, 'bad_cast' thrown if unable
    try {
      return dynamic_cast<h5::DataSet<DataType>&>(*sets_[name]).get();
    } catch (std::bad_cast const&) {
      throw std::runtime_error("DataSet corresponding to " + name +
                               " has different type.");
    }
  }
 private:
  /**
   * The File is the Event's friend,
   * this allows the File to handle the core iteration procedure
   * while preventing other classes from accessing these methods.
   */
  friend class Process;

  /**
   * Go through and save the current in-memory objects into 
   * the output file at the input index.
   *
   * @param[in] f output HDF5 file to write to
   * @param[in] i index of dataset to write to
   */
  void save(h5::File& f, unsigned long int i) {
    for (auto& [_, set] : sets_) set->save(f, i);
  }

  /**
   * Go through and load the input index into the in-memory
   * objects from the input file.
   *
   * @param[in] f input HDF5 file to read from
   * @param[in] i index of dataset to read from
   */
  void load(h5::File& f, unsigned long int i) {
    for (auto& [_, set] : sets_) set->load(f, i);
  }

  /**
   * Attach a HDF5 file to this event as the input file
   *
   * @note We store the input file as a pointer, but
   * don't clean-up later.
   *
   * @param[in] f reference to HDF5 input file
   */
  void setInputFile(h5::File& f) {
    input_file_ = &f;
  }

  /**
   * Move to the next event
   */
  void next() {
    i_entry_++;
  }

 private:
  /// pointer to input file (maybe nullptr)
  h5::File* input_file_;
  /// name of current processing pass
  std::string pass_;
  /// list of datasets being processed
  std::unordered_map<std::string, std::unique_ptr<h5::BaseDataSet>> sets_;
  /// current index in the datasets
  long unsigned int i_entry_;
};  // Event

}

#endif  // FIRE_EVENT_HPP
