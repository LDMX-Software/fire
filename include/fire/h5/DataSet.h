#ifndef FIRE_H5_DATASET_H
#define FIRE_H5_DATASET_H

#include <memory>
#include <type_traits>
#include <vector>
#include <map>

#include "fire/h5/Reader.h"
#include "fire/h5/Writer.h"

/**
 * serialization to and from HDF5 files
 *
 * Isolation of lower-level interaction with HDF5 files is done here.
 * These classes should never be used directly by the end user.
 */
namespace fire::h5 {

/**
 * Empty dataset base allowing recursion
 *
 * This does not have the type information of the data
 * stored in any of the derived datasets, it simply instructs
 * the derived data sets to define a load and save mechanism
 * for loading/saving the dataset from/to the file.
 */
class BaseDataSet {
 public:
  /**
   * Basic constructor
   */
  explicit BaseDataSet() = default;

  /**
   * virtual destructor so inherited classes can be properly destructed.
   */
  virtual ~BaseDataSet() = default;

  /**
   * pure virtual method for loading the next entry in the data set
   *
   * @param[in] f h5::Reader to load the next entry from
   */
  virtual void load(Reader& f) = 0;

  /**
   * pure virtual method for saving the current entry in the data set
   *
   * @param[in] f h5::Writer to write the current entry to
   */
  virtual void save(Writer& f) = 0;

  /**
   * pure virtual method for resetting the current data set handle to a blank state
   */
  virtual void clear() = 0;
};

/**
 * Type-specific base class to hold common dataset methods.
 *
 * Most (all I can think of?) have a shared initialization, destruction,
 * getting and setting procedure. We can house these procedures in an
 * intermediary class in the inheritence tree.
 *
 * @tparam DataType type of data being held in this set
 */
template <typename DataType>
class AbstractDataSet : public BaseDataSet {
 public:
  /**
   * Define the dataset path and provide an optional handle
   *
   * Defines the path to the data set in the file
   * and the handle to the current in-memory version of the data.
   *
   * If the handle is a nullptr, then we will own our own dynamically created
   * copy of the data. If the handle is not a nullptr, then we assume a parent
   * data set is holding the full object and we are simply holding a member
   * variable, so we just copy the address into our handle.
   *
   * @param[in] path full in-file path to the dataset
   * @param[in] handle address of object already created (optional)
   */
  explicit AbstractDataSet(const std::string& path, DataType* handle = nullptr)
      : BaseDataSet(), path_{path}, owner_{handle == nullptr} {
    if (owner_) {
      handle_ = new DataType;
    } else {
      handle_ = handle;
    }
  }

  /**
   * Delete our object if we own it, otherwise do nothing.
   *
   * @note This is virtual, but I can't think of a good reason to re-implement
   * this function in downstream DataSets!
   */
  virtual ~AbstractDataSet() {
    if (owner_) delete handle_;
  }

  /**
   * pure virtual method for loading the next entry in the data set
   *
   * @param[in] f h5::Reader to load the next entry from
   */
  virtual void load(Reader& f) = 0;

  /**
   * pure virtual method for saving the current entry in the data set
   *
   * @param[in] f h5::Writer to save the current entry to
   */
  virtual void save(Writer& f) = 0;

  /**
   * Define the clear function here to handle the most common cases.
   *
   * We 'clear' the object our handle points to.
   * 'clear' means two different things depending on the object.
   * 1. If the object is apart of 'numeric_limits', then we set it to the minimum.
   * 2. Otherwise, we assume the object has the 'clear' method defined.
   *
   * Case (1) handles the common fundamental types listed in the reference
   * [Numeric Limits](https://en.cppreference.com/w/cpp/types/numeric_limits)
   *
   * Case (2) handles common STL containers as well as std::string and is
   * a simple requirement on user classes.
   */
  virtual void clear() {
    if (owner_) {
      if constexpr (std::numeric_limits<DataType>::is_specialized) {
        *(this->handle_) = std::numeric_limits<DataType>::min();
      } else {
        handle_->clear();
      }
    }
  }

  /**
   * Get the current in-memory data object
   *
   * @note virtual so that derived data sets
   * could specialize this, but I can't think of a reason to do so.
   *
   * @return const reference to current data
   */
  virtual const DataType& get() const { return *handle_; }

  /**
   * Update the in-memory data object with the passed value.
   *
   * @note virtual so that derived data sets could specialize this, 
   * but I can't think of a reason to do so.
   *
   * @param[in] val new value the in-memory object should be
   */
  virtual void update(const DataType& val) { 
    *handle_ = val; 
  }

 protected:
  /// path of data set
  std::string path_;
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
  explicit DataSet(std::string const& path, DataType* handle = nullptr)
      : AbstractDataSet<DataType>(path, handle) {
    this->handle_->attach(*this);
  }

  /**
   * Loading this dataset from the file involves simply loading
   * all of the members of the data type.
   */
  void load(Reader& f) final override {
    for (auto& m : members_) m->load(f);
  }

  /*
   * Saving this dataset from the file involves simply saving
   * all of the members of the data type.
   */
  void save(Writer& f) final override {
    for (auto& m : members_) m->save(f);
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
        std::make_unique<DataSet<MemberType>>(this->path_ + "/" + name, &m));
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
  explicit DataSet(std::string const& path, AtomicType* handle = nullptr)
      : AbstractDataSet<AtomicType>(path, handle) {}
  /**
   * Call the H5Easy::load method with our atomic type and our path
   */
  void load(Reader& f) final override {
    f.load(this->path_, *(this->handle_));
  }
  /**
   * Call the H5Easy::save method with our atomic type and our path
   */
  void save(Writer& f) final override {
    f.save(this->path_, *(this->handle_));
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
  explicit DataSet(std::string const& path, std::vector<ContentType>* handle = nullptr)
      : AbstractDataSet<std::vector<ContentType>>(path, handle),
        size_{path + "/size"},
        data_{path + "/data"} {}

  /**
   * Load a vector from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   */
  void load(Reader& f) final override {
    size_.load(f);
    this->handle_->resize(size_.get());
    for (std::size_t i_vec{0}; i_vec < size_.get(); i_vec++) {
      data_.load(f);
      (*(this->handle_))[i_vec] = data_.get();
    }
  }

  /**
   * Save a vector to the output file
   *
   * @note We assume that the saves are done sequentially.
   *
   * We write the size and the content onto the end of their data sets.
   */
  void save(Writer& f) final override {
    size_.update(this->handle_->size());
    size_.save(f);
    for (std::size_t i_vec{0}; i_vec < this->handle_->size(); i_vec++) {
      data_.update(this->handle_->at(i_vec));
      data_.save(f);
    }
  }

 private:
  /// the data set of sizes of the vectors
  DataSet<std::size_t> size_;
  /// the data set holding the content of all the vectors
  DataSet<ContentType> data_;
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
  explicit DataSet(std::string const& path, std::map<KeyType,ValType>* handle = nullptr)
      : AbstractDataSet<std::map<KeyType,ValType>>(path, handle),
        size_{path + "/size"},
        keys_{path + "/keys"},
        vals_{path + "/vals"} {}

  /**
   * Load a map from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   */
  void load(Reader& f) final override {
    size_.load(f);
    for (std::size_t i_map{0}; i_map < size_.get(); i_map++) {
      keys_.load(f);
      vals_.load(f);
      this->handle_->emplace(keys_.get(), vals_.get());
    }
  }

  /**
   * Save a vector to the output file
   *
   * @note We assume that the saves are done sequentially.
   *
   * We write the size and the content onto the end of their data sets.
   */
  void save(Writer& f) final override {
    size_.update(this->handle_->size());
    size_.save(f);
    for (auto const& [key,val] : *(this->handle_)) {
      keys_.update(key);
      keys_.save(f);
      vals_.update(val);
      vals_.save(f);
    }
  }

 private:
  /// the data set of sizes of the vectors
  DataSet<std::size_t> size_;
  /// the data set holding the content of all the keys
  DataSet<KeyType> keys_;
  /// the data set holding the content of all the vals
  DataSet<ValType> vals_;
};  // DataSet<std::map>

}  // namespace fire::h5

#endif  // FIRE_H5_DATASET_H

