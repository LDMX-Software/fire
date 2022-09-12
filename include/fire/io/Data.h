#ifndef FIRE_IO_DATA_H
#define FIRE_IO_DATA_H

#include <memory>
#include <type_traits>
#include <vector>
#include <map>

#include "fire/io/AbstractData.h"
#include "fire/io/Writer.h"
#include "fire/io/Constants.h"
#include "fire/io/h5/Reader.h"
#ifdef fire_USE_ROOT
#include "fire/io/root/Reader.h"
#endif

/**
 * serialization to and from HDF5 files
 *
 * Isolation of lower-level interaction with HDF5 files is done here.
 * These classes should never be used directly by the end user except
 * for the example provided below.
 * 
 * The ability of fire to handle
 * the saving and loading of data to and from a file comes from this namespace.
 * fire is able to handle all so-called "atomic" types (types with 
 * [numeric limits](https://en.cppreference.com/w/cpp/types/numeric_limits)
 * defined and std::string, std::vector of, and std::map of these types.
 *
 * This accomodates a lot of workflows, but it doesn't accomodate everything.
 * In order to make fire even more flexible, there is a method of interfacing
 * this serialization procedure with a class that you define.
 *
 * Below is the `MyData` class declaration showing the minium structure 
 * necessary to interface with fire's serialization method.
 *
 * ```cpp
 * #include "fire/io/Data.h"
 * class MyData {
 *   friend class fire::io::Data<MyData>;
 *   MyData() = default;
 *   void clear();
 *   void attach(fire::io::Data<MyData>& d);
 * };
 * ```
 *
 * The user class has four necessary components:
 * 1. Your class declares the the wrapping io::Data class as a `friend`.
 *    - This allows the io::Data class access to the (potentially private)
 *      methods defined below.
 * 2. Your class has a (public or private) default constructor.
 *    - The default constructor may be how we initialize the data,
 *      so it must be defined and available to fire::io.
 *    - If you don't want other parts of the program using the default
 *      constructor, you can declare it `private`.
 * 3. Your class has a `void clear()` method defined which resets the object
 *    to an "empty" or "blank" state.
 *    - This is used by fire to reset the data at the end of each event.
 *    - Similar to the default constructor, this method can be public 
 *      or private.
 * 4. Your class implements a `void attach(fire::io::Data<MyData>& d)` method.
 *    - This method should be private since it should not be called by
 *      other parts of your code.
 *    - More detail below.
 *
 * ## The attach Method
 * This method is where you make the decision on which member variables of
 * your class should be stored to or read from data files and how those
 * variables are named. You do this using the fire::io::Data<DataType>::attach
 * method. This is best illustrated with an example.
 *
 * ```cpp
 * // member_one_ and member_two_ are members of MyData
 * void MyData::attach(fire::io::Data<MyData>& d) {
 *   d.attach("first_member", member_one_);
 *   d.attach("another_member", member_two_);
 * }
 * ```
 *
 * ### Important Comments
 * - The name of a variable on disk (the first argument) and the name
 *   of the variable in the class do not need to relate to each other;
 *   however, it is common to name them similarly so users of your data
 *   files aren't confused.
 * - The name of variables on disk cannot be the same in one `attach`
 *   method, but they can repeat across different classes (similar
 *   to member variables).
 * - Passing io::Data as reference (i.e. with the `&`) is necessary;
 *   otherwise, you would attach to a local copy and the real io::Data
 *   wouldn't be attached to anything.
 * - The members of MyData you pass to io::Data::attach can be any
 *   class that fire::io can handle. This includes the classes listed
 *   above or other classes you have defined following these rules.
 *
 * ## ROOT Reading
 * As a transitory feature, reading from ROOT files fire::io::root previously
 * produced by a ROOT-based serialization framework has been implemented.
 * In order to effectively read these ROOT files, the user must provide the
 * ROOT dictionaries for the classes that they wish to read. The method used
 * in the testing module in this repository is a good example of how to get
 * this done; that method involves three steps.
 *
 * ### Step 1: Add ROOT macros to your class
 * You must include the `TObject.h` header file in order to have access to
 * these macros. Then use `ClassDef(<class-name>,<version>)` in the header
 * within the class definition. Finally, use `ClassDef(<ns>::<class-name>);`
 * in the source file. This lines should be wrapped by preprocessor checks
 * so that users compiling your library _without_ ROOT can still compile it.
 * For example,
 * ```cpp
 * #include <fire/io/Data.h> // get fire_USE_ROOT definition
 * #ifdef fire_USE_ROOT
 * #include <TObject.h>
 * #endif
 * ```
 * **Note**: ROOT associates the data stored in member variables with the
 * name of that member variable. This means that ROOT will print warnings
 * or event error out if new member variables are added or if member variables
 * change names from when the file was written with ROOT.
 *
 * ### Step 2: Write a LinkDef file.
 * This file _must_ end in the string `LinkDef.h`. The prefix to this can
 * be anything that makes sense to you. The template link def is given below
 * with a few examples of how to list classes. This file should be alongside
 * any other headers in your dictionary.
 * ```cpp
 * // MyEventLinkDef.h
 * #ifdef __CINT__
 * 
 * #pragma link off all globals;
 * #pragma link off all classes;
 * #pragma link off all functions;
 * #pragma link C++ nestedclasses;
 * 
 * // always have to list my class
 * #pragma link C++ class myns::MyClass+;
 * // include if you want to read vectors of your class
 * #pragma link C++ class std::vector<myns::MyClass>+;
 * // include if you want maps of your class 
 * // (key could be anything in dictionary, not just int)
 * #pragma link C++ class std::map<int,myns::MyClass>+;
 * 
 * #endif
 * ```
 *
 * ### Step 3: CMake Nonsense
 * ROOT has written a CMake function that can be used to attach a dictionary
 * compilation to an existing CMake target. It is a bit finnicky, so be careful
 * when deviating from the template below.
 * ```cmake
 * find_package(fire REQUIRED 0.13.0)
 * add_library(MyEvent SHARED <list-source-files>)
 * target_link_libraries(MyEvent PUBLIC fire::io)
 * if(fire_USE_ROOT)
 *   target_include_directories(MyEvent PUBLIC
 *     "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
 *     "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")
 *   root_generate_dictionary(MyEventDict
 *     <list-header-files>
 *     LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/MyEvent/MyEventLinkDef.h
 *     MODULE MyEvent)
 *   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libMyEVent_rdict.pcm DESTINATION lib)
 * endif()
 * ```
 * This will include the compilation of a ROOT event dictionary if fire was built
 * with ROOT available.
 *
 * ## Full Example
 * Putting all of these notes together, below is an example class
 * that will read/write the coordinate positions but won't 
 * record the variable that is calculated from them.
 *
 * This isn't an amazing example since `std::sqrt` is pretty fast,
 * but you can perhaps imagine a class that has a time-expensive
 * calculation that should only be done once per event but is also
 * redundant and so it shouldn't waste disk space.
 *
 * ```cpp
 * // Point.h
 * #include "fire/io/Data.h"
 * #ifdef fire_USE_ROOT
 * #include "TObject.h"
 * #endif
 * class Point {
 *   double x_,y_,z_,mag_;
 *   friend class fire::io::Data<Point>;
 *   Point() = default;
 *   void clear();
 *   void attach(fire::io::Data<Point>& d);
 *  public:
 *   Point(double x, double y, double z);
 *   double mag() const;
 * #ifdef fire_USE_ROOT
 *   ClassDef(Point,1);
 * #endif
 * };
 * 
 * // Point.cxx
 * #include "Point.h"
 *
 * void Point::clear() {
 *   x_ = 0.;
 *   y_ = 0.;
 *   z_ = 0.;
 *   mag_ = -1.;
 * }
 * void Point::attach(fire::io::Data<Point>& d);
 *   d.attach("x",x_);
 *   d.attach("y",y_);
 *   d.attach("z",z_);
 * }
 * Point::Point(double x, double y, double z)
 *   : x_{x}, y_{y}, z_{z}, mag_{-1.} {
 *     mag();
 *   }
 * double Point::mag() const {
 *   if (mag_ < 0.) {
 *     mag_ = std::sqrt(x_*x_+y_*y_+z_*z_);
 *   }
 *   return mag_;
 * }
 *
 * #ifdef fire_USE_ROOT
 * ClassImp(Point);
 * #endif
 * ```
 *
 * ## Access Pattern
 *
 * Below is a sketch of how the various fire::io::Data template classes
 * interact with each other and the broader fire ecosystem.
 *
 * @image html fire_io_Data_AccessPattern.svg
 */
namespace fire::io {

/**
 * General data set
 *
 * This is the top-level data set that will be used most often.
 * It is meant to be used by a class which registers its member
 * variables to this set via the io::DataSet<DataType>::attach
 * method.
 *
 * More complete documentation is kept in the documentation
 * of the fire::io namespace; nevertheless, a short example
 * is kep here.
 *
 * ```cpp
 * class MyData {
 *  public:
 *   MyData() = default; // required by serialization technique
 *   // other public members
 *  private:
 *   friend class fire::io::Data<MyData>;
 *   void attach(fire::io::Data<MyData>& set) {
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
class Data : public AbstractData<DataType> {
 public:
  /**
   * Attach ourselves to the input type after construction.
   *
   * After the intermediate class AbstractData does the
   * initialization, we call the `void attach(io::Data<DataType>& d)`
   * method of the data pointed to by our handle. 
   * This allows us to register its member variables with our own 
   * io::Data<DataType>::attach method.
   *
   * @param[in] path full in-file path to the data set for this data
   * @param[in] handle address of object already created (optional)
   */
  explicit Data(const std::string& path, DataType* handle = nullptr)
      : AbstractData<DataType>(path, handle) {
    this->handle_->attach(*this);
  }

  /**
   * Loading this dataset from the file involves simply loading
   * all of the members of the data type.
   *
   * @param[in] f file to load from
   */
  void load(h5::Reader& f) final override {
    for (auto& m : members_) m->load(f);
  }

#ifdef fire_USE_ROOT
  /**
   * Loading this dataset from a ROOT file involves giving
   * it directly to the file immediately.
   */
  void load(root::Reader& f) final override {
    f.load(this->path_, *(this->handle_));
  }
#endif

  /*
   * Saving this dataset from the file involves simply saving
   * all of the members of the data type.
   *
   * @param[in] f file to save to
   */
  void save(Writer& f) final override {
    for (auto& m : members_) m->save(f);
  }

  void done(Writer& f) final override {
    f.setTypeName(this->path_, this->type_, this->version_);
    for (auto& m : members_) m->done(f);
  }

  /**
   * Attach a member object from the our data handle
   *
   * We create a new child Data so that we can recursively
   * handle complex member variable types.
   *
   * @tparam MemberType type of member variable we are attaching
   * @param[in] name name of member variable
   * @param[in] m reference of member variable
   */
  template <typename MemberType>
  void attach(const std::string& name, MemberType& m) {
    if (name == constants::SIZE_NAME) {
      throw Exception("BadName",
          "The member name '"+constants::SIZE_NAME+"' is not allowed due to "
          "its use in the serialization of variable length types.\n"
          "    Please give your member a more detailed name corresponding to "
          "your class", false);
    }
    members_.push_back(
        std::make_unique<Data<MemberType>>(this->path_ + "/" + name, &m));
  }

 private:
  /// list of members in this dataset
  std::vector<std::unique_ptr<BaseData>> members_;
};  // Data

/**
 * Data wrapper for atomic types
 *
 * @see io::is_atomic for how we deduce if a type is atomic
 *
 * Once we finally recurse down to actual fundamental ("atomic") types,
 * we can start actually calling the file load and save methods.
 */
template <typename AtomicType>
class Data<AtomicType, std::enable_if_t<is_atomic_v<AtomicType>>>
    : public AbstractData<AtomicType> {
 public:
  /**
   * We don't do any more initialization except which is handled by the
   * AbstractData
   *
   * @param[in] path full in-file path to set holding this data
   * @param[in] handle pointer to already constructed data object (optional)
   */
  explicit Data(const std::string& path, AtomicType* handle = nullptr)
      : AbstractData<AtomicType>(path, handle) {}
  /**
   * Down to a type that h5::Reader can handle.
   *
   * @see h5::Reader::load for how we read data from
   * the file at the input path to our handle.
   *
   * @param[in] f h5::Reader to load from
   */
  void load(h5::Reader& f) final override {
    f.load(this->path_, *(this->handle_));
  }

#ifdef fire_USE_ROOT
  /**
   * Loading this dataset from a ROOT file involves giving
   * it directly to the file immediately.
   */
  void load(root::Reader& f) final override {
    f.load(this->path_, *(this->handle_));
  }
#endif

  /**
   * Down to a type that io::Writer can handle
   *
   * @see io::Writer::save for how we write data to
   * the file at the input path from our handle.
   *
   * @param[in] f io::Writer to save to
   */
  void save(Writer& f) final override {
    f.save(this->path_, *(this->handle_));
  }

  void done(Writer& f) final override {
    f.setTypeName(this->path_, this->type_);
  }
};  // Data<AtomicType>

/**
 * Our wrapper around std::vector
 *
 * @note We assume that the load/save is done sequentially.
 * This assumption is made because
 *  (1) it is how fire is designed and
 *  (2) it allows us to not have to store
 *      as much metadata about the vectors.
 *
 * @tparam ContentType type of object stored within the std::vector
 */
template <typename ContentType>
class Data<std::vector<ContentType>>
    : public AbstractData<std::vector<ContentType>> {
 public:
  /**
   * We create two child data sets, one to hold the successive sizes of the
   * vectors and one to hold all of the data in all of the vectors serially.
   *
   * @param[in] path full in-file path to set holding this data
   * @param[in] handle pointer to object already constructed (optional)
   */
  explicit Data(const std::string& path, std::vector<ContentType>* handle = nullptr)
      : AbstractData<std::vector<ContentType>>(path, handle),
        size_{path + "/" + constants::SIZE_NAME},
        data_{path + "/data"} {}

  /**
   * Load a vector from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the content data set into the vector handle.
   *
   * @param[in] f h5::Reader to load from
   */
  void load(h5::Reader& f) final override {
    size_.load(f);
    this->handle_->resize(size_.get());
    for (std::size_t i_vec{0}; i_vec < size_.get(); i_vec++) {
      data_.load(f);
      (*(this->handle_))[i_vec] = data_.get();
    }
  }

#ifdef fire_USE_ROOT
  /**
   * Loading this dataset from a ROOT file involves giving
   * it directly to the file immediately.
   */
  void load(root::Reader& f) final override {
    f.load(this->path_, *(this->handle_));
  }
#endif

  /**
   * Save a vector to the output file
   *
   * @note We assume that the saves are done sequentially.
   *
   * We write the size and the content onto the end of their data sets.
   *
   * @param[in] f io::Writer to save to
   */
  void save(Writer& f) final override {
    size_.update(this->handle_->size());
    size_.save(f);
    for (std::size_t i_vec{0}; i_vec < this->handle_->size(); i_vec++) {
      data_.update(this->handle_->at(i_vec));
      data_.save(f);
    }
  }

  void done(Writer& f) final override {
    f.setTypeName(this->path_, this->type_);
    size_.done(f);
    data_.done(f);
  }

 private:
  /// the data set of sizes of the vectors
  Data<std::size_t> size_;
  /// the data set holding the content of all the vectors
  Data<ContentType> data_;
};  // Data<std::vector>

/**
 * Our wrapper around std::map
 *
 * Very similar implementation as vectors, just having
 * two columns rather than only one.
 *
 * @note We assume the load/save is done sequentially.
 * Similar rational as io::Data<std::vector<ContentType>>
 *
 * @tparam KeyType type that the keys in the map are
 * @tparam ValType type that the vals in the map are
 */
template <typename KeyType, typename ValType>
class Data<std::map<KeyType,ValType>>
    : public AbstractData<std::map<KeyType,ValType>> {
 public:
  /**
   * We create three child data sets, one for the successive sizes
   * of the maps and two to hold all the keys and values serially.
   *
   * @param[in] path full in-file path to set holding this data
   * @param[in] handle pointer to object already constructed (optional)
   */
  explicit Data(const std::string& path, std::map<KeyType,ValType>* handle = nullptr)
      : AbstractData<std::map<KeyType,ValType>>(path, handle),
        size_{path + "/" + constants::SIZE_NAME},
        keys_{path + "/keys"},
        vals_{path + "/vals"} {}

  /**
   * Load a map from the input file
   *
   * @note We assume that the loads are done sequentially.
   *
   * We read the next size and then read that many items from
   * the keys/vals data sets into the map handle.
   *
   * @param[in] f h5::Reader to load from
   */
  void load(h5::Reader& f) final override {
    size_.load(f);
    for (std::size_t i_map{0}; i_map < size_.get(); i_map++) {
      keys_.load(f);
      vals_.load(f);
      this->handle_->emplace(keys_.get(), vals_.get());
    }
  }

#ifdef fire_USE_ROOT
  /**
   * Loading this dataset from a ROOT file involves giving
   * it directly to the file immediately.
   */
  void load(root::Reader& f) final override {
    f.load(this->path_, *(this->handle_));
  }
#endif

  /**
   * Save a vector to the output file
   *
   * @note We assume that the saves are done sequentially.
   *
   * We write the size and the keys/vals onto the end of their data sets.
   *
   * @param[in] f io::Writer to save to
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

  void done(Writer& f) final override {
    f.setTypeName(this->path_, this->type_);
    size_.done(f);
    keys_.done(f);
    vals_.done(f);
  }

 private:
  /// the data set of sizes of the vectors
  Data<std::size_t> size_;
  /// the data set holding the content of all the keys
  Data<KeyType> keys_;
  /// the data set holding the content of all the vals
  Data<ValType> vals_;
};  // Data<std::map>

}  // namespace fire::io

#endif  // FIRE_H5_DATASET_H

