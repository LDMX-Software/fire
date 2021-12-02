#ifndef FIRE_H5_DATASET_HPP
#define FIRE_H5_DATASET_HPP

#include <memory>
#include <type_traits>
#include <vector>
#include <map>

#include "fire/h5/File.hpp"

namespace fire {
namespace h5 {

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
   * virtual destructor so inherited classes can be
   * properly destructed.
   */
  virtual ~BaseDataSet() = default;
  /**
   * pure virtual method for loading the input entry in the data set
   */
  virtual void load(File& f, long unsigned int i) = 0;
  /**
   * pure virtual method for saving the input entry in the data set
   */
  virtual void save(File& f, long unsigned int i) = 0;
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
  AbstractDataSet(std::string const& name, DataType* handle = nullptr)
      : name_{name}, owner_{handle == nullptr} {
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
  virtual void load(File& f, long unsigned int i) = 0;
  /// pass on pure virtual load function
  virtual void save(File& f, long unsigned int i) = 0;

  /**
   * Get the current in-memory data object
   *
   * @note virtual so that derived data sets
   * could specialize this, but I can't think of a reason to do so.
   *
   * @return const reference to current data
   */
  virtual DataType const& get() { return *handle_; }

  /**
   * Update the in-memory data object with the passed value.
   *
   * @note virtual so that derived data sets
   * could specialize this, but I can't think of a reason to do so.
   *
   * @param[in] val new value the in-memory object should be
   */
  virtual void update(DataType const& val) { *handle_ = val; }

 protected:
  /// name of data set
  std::string name_;
  /// handle on current object in memory
  DataType* handle_;
  /// we own the object in memory
  bool owner_;
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
  DataSet(std::string const& name, DataType* handle = nullptr)
      : AbstractDataSet<DataType>(name, handle) {
    this->handle_->attach(*this);
  }

  /**
   * Loading this dataset from the file involves simply loading
   * all of the members of the data type.
   */
  void load(File& f, long unsigned int i) {
    for (auto& m : members_) m->load(f, i);
  }

  /*
   * Saving this dataset from the file involves simply saving
   * all of the members of the data type.
   */
  void save(File& f, long unsigned int i) {
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
        std::make_unique<DataSet<MemberType>>(this->name_ + "/" + name, &m));
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
 *
 * @TODO port H5Easy::File into our framework. We can specialize it
 * to our purposes to improve performance.
 */
template <typename AtomicType>
class DataSet<AtomicType, std::enable_if_t<std::is_arithmetic_v<AtomicType>>>
    : public AbstractDataSet<AtomicType> {
 public:
  /**
   * We don't do any more initialization except which is handled by the
   * AbstractDataSet
   */
  DataSet(std::string const& name, AtomicType* handle = nullptr)
      : AbstractDataSet<AtomicType>(name, handle) {}
  /**
   * Call the H5Easy::load method with our atomic type and our name
   */
  void load(File& f, long unsigned int i) {
    f.load(this->name_, i, *(this->handle_));
  }
  /**
   * Call the H5Easy::save method with our atomic type and our name
   */
  void save(File& f, long unsigned int i) {
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
  DataSet(std::string const& name, std::vector<ContentType>* handle = nullptr)
      : AbstractDataSet<std::vector<ContentType>>(name, handle),
        size_{name + "/size"},
        data_{name + "/data"},
        i_data_entry_{0} {}

  /**
   * Load a vector from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   */
  void load(File& f, long unsigned int i_entry) {
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
  void save(File& f, long unsigned int i_entry) {
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
  DataSet(std::string const& name, std::map<KeyType,ValType>* handle = nullptr)
      : AbstractDataSet<std::map<KeyType,ValType>>(name, handle),
        size_{name + "/size"},
        keys_{name + "/keys"},
        vals_{name + "/vals"},
        i_data_entry_{0} {}

  /**
   * Load a map from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   */
  void load(File& f, long unsigned int i_entry) {
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
  void save(File& f, long unsigned int i_entry) {
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

}  // namespace h5
}  // namespace fire

#endif  // FIRE_H5_DATASET_HPP

