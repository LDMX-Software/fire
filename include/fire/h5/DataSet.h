#ifndef FIRE_H5_DATASET_H
#define FIRE_H5_DATASET_H

#include <memory>
#include <type_traits>
#include <vector>
#include <map>

#include "fire/exception/Exception.h"
#include "fire/h5/Reader.h"
#include "fire/h5/Writer.h"

namespace fire::h5 {

/**
 * Allow for a specific h5 Exception
 *
 * These exceptions aren't used directly here,
 * but are used in downstream classes like Event
 * and ParameterStorage.
 */
ENABLE_EXCEPTIONS();

/**
 * @class BaseDataSet
 *
 * Empty dataset base allowing recursion
 *
 * This does not have the type information of the data
 * stored in any of the derived datasets, it simply instructs
 * the derived data sets to define a load and save mechanism
 * for loading/saving the dataset from/to the file.
 *
 * This type should never be seen outside of the inner-workings
 * of fire.
 */
class BaseDataSet {
 public:
  /**
   * Constructor defining whether this dataset is a transient
   * in-memory data set (should_save == false) or a data set
   * that should be saved into the output file (should_save == true).
   */
  explicit BaseDataSet(bool should_save) : should_save_{should_save} {}

  /**
   * virtual destructor so inherited classes can be properly destructed.
   */
  virtual ~BaseDataSet() = default;

  /**
   * pure virtual method for loading the input entry in the data set
   */
  virtual void load(Reader& f, long unsigned int i) = 0;

  /**
   * pure virtual method for saving the input entry in the data set
   */
  virtual void save(Writer& f, long unsigned int i) = 0;

  /**
   * pure virtual method for resetting the current data set handle to a blank state
   */
  virtual void clear() = 0;

  /**
   * The method used in the event class,
   * we check if the data set should be saved before
   * calling the save method overridden by derived classes.
   */
  void checkThenSave(Writer& f, long unsigned int i) {
    if (should_save_) save(f,i);
  }

 protected:
  /// should we save this data set into output file?
  bool should_save_;
};

/**
 * @class AbstractDataSet
 *
 * Type-specific base class to hold common dataset methods.
 *
 * Most (all I can think of?) have a shared initialization, destruction,
 * getting and setting procedure. We can house these procedures in an
 * intermediary class in the inheritence tree.
 *
 * @tparam[in] DataType type of data being held in this set
 */
template <typename DataType>
class AbstractDataSet : public BaseDataSet {
 public:
  /**
   * Only constructor
   *
   * Passes on the handle to the file we are reading from or writing to.
   * Defines the name of the data set and the handle to the current in-memory
   * version of the data.
   *
   * If the handle is a nullptr, then we will own our own dynamically created
   * copy of the data. If the handle is not a nullptr, then we assume a parent
   * data set is holding the full object and we are simply holding a member
   * variable, so we just copy the address into our handle.
   *
   * @param[in] name name of dataset
   * @param[in] handle address of object already created (optional)
   */
  explicit AbstractDataSet(std::string const& name, bool should_save, DataType* handle = nullptr)
      : BaseDataSet(should_save), name_{name}, owner_{handle == nullptr} {
    if (owner_) {
      handle_ = new DataType;
    } else {
      handle_ = handle;
    }
  }

  /**
   * Destructor
   *
   * Delete our object if we own it, otherwise do nothing.
   */
  virtual ~AbstractDataSet() {
    if (owner_) delete handle_;
  }

  /// pass on pure virtual load function
  virtual void load(Reader& f, long unsigned int i) = 0;
  /// pass on pure virtual save function
  virtual void save(Writer& f, long unsigned int i) = 0;

  /**
   * Define the clear function here to handle the most common cases.
   *
   * We re-set the flag checking if the dataset has been updated to 'false'
   * and we call the 'clear' method of the object our handle points to.
   *
   * Downstream datasets can re-implement this method, but **they must**
   * include the 'updated_ = false;' line in order to prevent two processors
   * modifying the same data set in one process.
   */
  virtual void clear() {
    updated_ = false;
    //if (owner_) handle_->clear();
  }

  /**
   * Get the current in-memory data object
   *
   * @note virtual so that derived data sets
   * could specialize this, but I can't think of a reason to do so.
   *
   * @return const reference to current data
   */
  virtual DataType const& get() const { return *handle_; }

  /**
   * Update the in-memory data object with the passed value.
   *
   * @note virtual so that derived data sets
   * could specialize this, but I can't think of a reason to do so.
   *
   * @param[in] val new value the in-memory object should be
   */
  virtual void update(DataType const& val) { 
    *handle_ = val; 
    updated_ = true;
  }

  /**
   * Have we been updated?
   */
  inline bool updated() const {
    return updated_;
  }

 protected:
  /// name of data set
  std::string name_;
  /// handle on current object in memory
  DataType* handle_;
  /// we own the object in memory
  bool owner_;
  /// have we been updated?
  bool updated_{false};
};  // AbstractDataSet

/**
 * General data set
 *
 * This is the top-level data set that will be used most often.
 * It is meant to be used by a class which registers its member
 * variables to this set via its 'attach' method.
 *
 * ```cpp
 * class MyData {
 *  public:
 *   MyData() = default; // required by serialization technique
 *   // other public members
 *  private:
 *   friend class fire::h5::DataSet<MyData>;
 *   void attach(fire::h5::DataSet<MyData>& set) {
 *     set.attach("my_double",my_double_);
 *   }
 *   void clear() {
 *     my_double_ = -1; // reset to default value
 *   }
 *  private:
 *   double my_double_;
 *   // this member doesn't appear in 'attach' so it won't end up on disk
 *   int i_wont_be_on_disk_;
 * };
 * ```
 */
template <typename DataType, typename Enable = void>
class DataSet : public AbstractDataSet<DataType> {
 public:
  /**
   * Default constructor
   *
   * After the intermediate class AbstractDataSet does the
   * initialization, we call the 'attach' method of the data
   * pointed to by our handle. This allows us to register
   * its member variables with our own 'attach' method.
   */
  explicit DataSet(std::string const& name, bool should_save, DataType* handle = nullptr)
      : AbstractDataSet<DataType>(name, should_save, handle) {
    this->handle_->attach(*this);
  }

  /**
   * Loading this dataset from the file involves simply loading
   * all of the members of the data type.
   */
  void load(Reader& f, long unsigned int i) {
    for (auto& m : members_) m->load(f, i);
  }

  /*
   * Saving this dataset from the file involves simply saving
   * all of the members of the data type.
   */
  void save(Writer& f, long unsigned int i) {
    for (auto& m : members_) m->save(f, i);
  }

  /**
   * Attach a member object from the our data handle
   *
   * We create a new child DataSet so that we can recursively
   * handle complex member variable types.
   *
   * @tparam[in] MemberType type of member variable we are attaching
   * @param[in] name name of member variable
   * @param[in] m reference of member variable
   */
  template <typename MemberType>
  void attach(std::string const& name, MemberType& m) {
    members_.push_back(
        std::make_unique<DataSet<MemberType>>(this->name_ + "/" + name, this->should_save_, &m));
  }

 private:
  /// list of members in this dataset
  std::vector<std::unique_ptr<BaseDataSet>> members_;
};  // DataSet

/**
 * Atomic types
 *
 * Once we finally recurse down to actual fundamental ("atomic") types,
 * we can start actually calling the file load and save methods.
 */
template <typename AtomicType>
class DataSet<AtomicType, std::enable_if_t<is_atomic_v<AtomicType>>>
    : public AbstractDataSet<AtomicType> {
 public:
  /**
   * We don't do any more initialization except which is handled by the
   * AbstractDataSet
   */
  explicit DataSet(std::string const& name, bool should_save, AtomicType* handle = nullptr)
      : AbstractDataSet<AtomicType>(name, should_save, handle) {}
  /**
   * Call the H5Easy::load method with our atomic type and our name
   */
  void load(Reader& f, long unsigned int i) {
    f.load(this->name_, i, *(this->handle_));
  }
  /**
   * Call the H5Easy::save method with our atomic type and our name
   */
  void save(Writer& f, long unsigned int i) {
    f.save(this->name_, i, *(this->handle_));
  }
};  // DataSet<AtomicType>

/**
 * Vectors
 *
 * @note We assume that the load/save is done sequentially.
 * This assumption is made because
 *  (1) it is common and
 *  (2) it allows us to not have to store
 *      as much metadata about the vectors.
 */
template <typename ContentType>
class DataSet<std::vector<ContentType>>
    : public AbstractDataSet<std::vector<ContentType>> {
 public:
  /**
   * We create two child data sets, one to hold the successive sizes of the
   * vectors and one to hold all of the data in all of the vectors serially.
   */
  explicit DataSet(std::string const& name, bool should_save, std::vector<ContentType>* handle = nullptr)
      : AbstractDataSet<std::vector<ContentType>>(name, should_save, handle),
        size_{name + "/size",should_save},
        data_{name + "/data",should_save},
        i_data_entry_{0} {}

  /**
   * Load a vector from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   */
  void load(Reader& f, long unsigned int i_entry) {
    size_.load(f, i_entry);
    this->handle_->resize(size_.get());
    for (std::size_t i_vec{0}; i_vec < size_.get(); i_vec++) {
      data_.load(f, i_data_entry_);
      (*(this->handle_))[i_vec] = data_.get();
      i_data_entry_++;
    }
  }

  /**
   * Save a vector to the output file
   *
   * @note We assume that the saves are done sequentially.
   *
   * We write the size and the content onto the end of their data sets.
   */
  void save(Writer& f, long unsigned int i_entry) {
    size_.update(this->handle_->size());
    size_.save(f, i_entry);
    for (std::size_t i_vec{0}; i_vec < this->handle_->size(); i_vec++) {
      data_.update(this->handle_->at(i_vec));
      data_.save(f, i_data_entry_);
      i_data_entry_++;
    }
  }

 private:
  /// the data set of sizes of the vectors
  DataSet<std::size_t> size_;
  /// the data set holding the content of all the vectors
  DataSet<ContentType> data_;
  /// the entry in the content data set we are currently on
  unsigned long int i_data_entry_;
};  // DataSet<std::vector>

/**
 * Maps
 *
 * Very similar implementation as vectors, just having
 * two columns rather than only one.
 *
 * @note We assume the load/save is done sequentially.
 * Similar rational as Vectors
 */
template <typename KeyType, typename ValType>
class DataSet<std::map<KeyType,ValType>>
    : public AbstractDataSet<std::map<KeyType,ValType>> {
 public:
  /**
   * We create three child data sets, one for the successive sizes
   * of the maps and two to hold all the keys and values serially.
   */
  explicit DataSet(std::string const& name, bool should_save, std::map<KeyType,ValType>* handle = nullptr)
      : AbstractDataSet<std::map<KeyType,ValType>>(name, should_save, handle),
        size_{name + "/size",should_save},
        keys_{name + "/keys",should_save},
        vals_{name + "/vals",should_save},
        i_data_entry_{0} {}

  /**
   * Load a map from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   */
  void load(Reader& f, long unsigned int i_entry) {
    size_.load(f, i_entry);
    for (std::size_t i_map{0}; i_map < size_.get(); i_map++) {
      keys_.load(f, i_data_entry_);
      vals_.load(f, i_data_entry_);
      this->handle_->emplace(keys_.get(), vals_.get());
      i_data_entry_++;
    }
  }

  /**
   * Save a vector to the output file
   *
   * @note We assume that the saves are done sequentially.
   *
   * We write the size and the content onto the end of their data sets.
   */
  void save(Writer& f, long unsigned int i_entry) {
    size_.update(this->handle_->size());
    size_.save(f, i_entry);
    for (auto const& [key,val] : *(this->handle_)) {
      keys_.update(key);
      keys_.save(f, i_data_entry_);
      vals_.update(val);
      vals_.save(f, i_data_entry_);
      i_data_entry_++;
    }
  }

 private:
  /// the data set of sizes of the vectors
  DataSet<std::size_t> size_;
  /// the data set holding the content of all the keys
  DataSet<KeyType> keys_;
  /// the data set holding the content of all the vals
  DataSet<ValType> vals_;
  /// the entry in the content data set we are currently on
  unsigned long int i_data_entry_;
};  // DataSet<std::map>

}  // namespace fire::h5

#endif  // FIRE_H5_DATASET_H

