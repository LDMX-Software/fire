#ifndef FIRE_H5_READER_H
#define FIRE_H5_READER_H

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/h5/Atomic.h"

namespace fire::h5 {

/**
 * Reading a file generated by fire
 *
 * @see h5::Writer for our files are written with fire
 *
 * This reader handles the buffering of data read in from an HDF5 file
 * in a seamless manner so that individual entries can be requested at
 * a time without making disk read operation each time Reader::load is
 * called.
 */
class Reader {
 public:
  /// the name of the event header data set
  static const std::string EVENT_HEADER_NAME;
  /// the name of the variable in event and run  header corresponding to event number
  static const std::string NUMBER_NAME;
  /// the name of the group holding all of the event data sets
  static const std::string EVENT_GROUP;
  /// the name of the run header data set
  static const std::string RUN_HEADER_NAME;
  /// the name of the attribute that stores the type of event object
  static const std::string TYPE_ATTR_NAME;

 public:
  /**
   * Open the file in read mode
   *
   * Our read mode is the same as HDF5 Read Only mode
   *
   * We also read the number of entries in this file by 
   * retrieving the size of the data set at
   *  EVENT_GROUP/EVENT_HEADER_NAME/NUMBER_NAME
   * This is reliable as long as 
   * 1. EventHeader attaches the event number (or a similar
   *    once-per-event atomic type) under the name EVENT_HEADER_NUMBER
   * 2. The EventHeader is created under a h5::Data named
   *    EVENT_GROUP/EVENT_HEADER_NAME
   *
   * We inspect the size of the dataset located at
   *  RUN_HEADER_NAME/NUMBER_NAME
   * in the file to get the number of runs. This will work as long as
   * 1. The RunHeader is written to the file as RUN_HEADER_NAME.
   * 2. The RunHeader attaches an atomic type member under the name `number`
   *
   * @throws HighFive::Exception if file is not accessible.
   * @param[in] name file name to open and read
   */
  Reader(const std::string& name);

  /**
   * Get the name of this file
   * @return name of the file we are reading
   */
  const std::string& name() const;

  /**
   * List the entries inside of a group.
   *
   * @note This is low-level and is only being used in 
   * h5::ParameterStorage for more flexibility in the parameter maps
   * and Event::setInputFile to obtain the event products within a file.
   *
   * @param[in] group_path full in-file path to H5 group to list elements of
   * @return list of elements in the group, empty if group does not exist
   */
  std::vector<std::string> list(const std::string& group_path) const;

  /**
   * Deduce the type of the dataset requested.
   *
   * @note This is low-level and is only being used in h5::ParameterStorage
   * for more flexibility in the parameter maps.
   *
   * @param[in] dataset full in-file path to H5 dataset
   * @return HighFive::DataTypeClass specifying the atomic type of the set
   */
  HighFive::DataTypeClass getDataSetType(const std::string& dataset) const;

  /**
   * Get the 'type' attribute from the input group name
   *
   * We retrieve the attribute named TYPE_ATTR_NAME from the group
   * located at EVENT_GROUP/obj_name. This enables us to construct
   * the list of products within an inputfile in Event::setInputFile.
   *
   * @see h5::Writer::setTypeName for where the type attr is set
   *
   * @param[in] obj_name Name of event object to retrieve type of
   * @return demangled type name in string format
   */
  std::string getTypeName(const std::string& obj_name) const;

  /**
   * Get the number of entries in the file
   *
   * This value was determined upon construction.
   *
   * @return number of events within the file
   */
  inline std::size_t entries() const { return entries_; }

  /**
   * Get the number of runs in the file
   *
   * This value was determined upon construction
   *
   * @return number of runs within the file
   */
  inline std::size_t runs() const { return runs_; }

  /**
   * Try to load a single value of an atomic type into the input handle
   *
   * If the input path does not exist in our list of buffers,
   * then we create a new buffer for the requested type and
   * load the first chunk of data into memory.
   *
   * Besides that, we access the Buffer and call its Buffer::read
   * method, allowing the Buffer to handle the necessary disk reading
   * operations.
   *
   * @throws std::bad_cast if mismatched type is passed
   * @throws HighFive::DataSetException if requested dataset doesn't exist
   *
   * @param[in] path Full in-file path to dataset to load
   * @param[out] val Set to the next entry in the dataset
   */
  template <typename AtomicType>
  void load(const std::string& path, AtomicType& val) {
    static_assert(
        is_atomic_v<AtomicType>,
        "Type not supported by HighFive atomic made its way to Reader::load");
    if (buffers_.find(path) == buffers_.end()) {
      // first load attempt, we will find out if dataset exists in file here
      buffers_.emplace(path, std::make_unique<Buffer<AtomicType>>(
                                 rows_per_chunk_, file_.getDataSet(path)));
    }

    dynamic_cast<Buffer<AtomicType>&>(*buffers_[path]).read(val);
  }

  /**
   * Print the input reader to the stream
   *
   * We print the name Reader as well as the name of the file
   * we are reading.
   *
   * @param[in] s std::ostream to write to
   * @param[in] r Reader to stream
   * @return modified std::ostream
   */
  friend std::ostream& operator<<(std::ostream& s, const Reader& r) {
    return s << "Reader(" << r.name() << ")";
  }

  /// never want to copy a reader
  Reader(const Reader&) = delete;
  /// never want to copy a reader
  void operator=(const Reader&) = delete;

 private:
  /**
   * Type-less handle to the Buffer
   *
   * This base class allows us to hold the DataSet being
   * read from and hold all of the Buffers of atomic types
   * in a single map.
   */
  class BufferHandle {
   protected:
    /// the maximum length of the buffer
    std::size_t max_len_;
    /// the HDF5 dataset we are reading from
    HighFive::DataSet set_;
   public:
    /**
     * Define the size of the in-memory buffer and the set we are reading from
     *
     * @param[in] max maximum number of elements allowed in-memory
     * @param[in] s DataSet we are reading from
     */
    explicit BufferHandle(std::size_t max, HighFive::DataSet s)
        : max_len_{max}, set_{s} {}
    /// virtual destructor to pass on to derived types
    virtual ~BufferHandle() = default;
    /**
     * pure virtual load function to be defined when we know the type
     *
     * This isn't used outside of the Buffer class but it is helpful
     * in making sure future developments don't inadvertently abuse
     * the BufferHandle class which is meant to be abstract.
     */
    virtual void load() = 0;
  };

  /**
   * Read buffer allowing individual element access from a dataset
   *
   * @tparam AtomicType type of elements in dataset
   */
  template <typename AtomicType>
  class Buffer : public BufferHandle {
    /// the actual buffer of in-memory elements
    std::vector<AtomicType> buffer_;
    /// the current index of data-set elements in the file
    std::size_t i_file_;
    /// the current index of data-set elements in-memory
    std::size_t i_memory_;
    /// the number of entries in the entire dataset
    std::size_t entries_;
   public:
    /**
     * Define the size of the buffer and provide the dataset to read from
     *
     * We initialize ourselves with the indices set to 0,
     * the entries read from the size of the dataset passed,
     * and the buffer empty. Then we call our own Buffer::load method
     * to get the first chunk of data into memory.
     *
     * @param[in] max size of the buffer
     * @param[in] s dataset to read from
     */
    explicit Buffer(std::size_t max, HighFive::DataSet s)
        : BufferHandle(max, s), buffer_{}, i_file_{0}, i_memory_{0} {
      // get the number of entries for later checking
      entries_ = this->set_.getDimensions().at(0);
      // do first load upon creation
      this->load();
    }
    /// nothing fancy, just clearing in-memory objects
    virtual ~Buffer() = default;
    
    /**
     * Read the next entry from the dataset into the input variable
     *
     * If we have reached the end of the in-memory buffer,
     * then we call Buffer::load to retrieve the next chunk
     * of data into memory.
     *
     * @param[out] out variable to read entry into
     */
    void read(AtomicType& out) {
      if (i_memory_ == buffer_.size()) this->load();
      out = buffer_[i_memory_];
      i_memory_++;
    }
    
    /**
     * Load the next chunk of data into memory
     *
     * We determine the size of the next chunk from our own
     * maximum and the number of entries in the data set.
     * We shrink the size of the next chunk depending on how
     * many entries are left if we can't grab a whole maximum
     * sized chunk.
     *
     * We have a compile-time split in order to patch 
     * [a bug](https://github.com/BlueBrain/HighFive/issues/490)
     * in HighFive that doesn't allow writing of std::vector<bool>
     * due to the specialization of it.
     *
     * After reading the next chunk into memory, we update our
     * indicies by resetting the in-memory index to 0 and moving
     * the file index by the size of the buffer.
     *
     * @note We assume that the downstream objects using this buffer
     * know to stop processing before attempting to read passed the
     * end of the data set. We enforce this with an assertion.
     */
    virtual void load() final override {
      // determine the length we want to request depending
      // on the number of entries left in the file
      std::size_t request_len = this->max_len_;
      if (request_len + i_file_ > entries_) {
        request_len = entries_ - i_file_;
        assert(request_len >= 0);
      }
      // load the next chunk into memory
      if constexpr (std::is_same_v<AtomicType,bool>) {
        // get around std::vector<bool> specialization
        std::vector<short> buff;
        this->set_.select({i_file_}, {request_len}).read(buff);
        buffer_.reserve(buff.size());
        for (const auto& v : buff) buffer_.push_back(v);
      } else {
        this->set_.select({i_file_}, {request_len}).read(buffer_);
      }
      // update indices
      i_file_ += buffer_.size();
      i_memory_ = 0;
    }
  };

 private:
  /// our highfive file
  HighFive::File file_;
  /// the number of entries in this file, set in constructor
  const std::size_t entries_;
  /// the number of runs in this file, set in constructor
  const std::size_t runs_;
  /// the number of rows to keep in each chunk, read from DataSet?
  std::size_t rows_per_chunk_{10000};
  /// our in-memory buffers for the data to be read in from disk
  std::unordered_map<std::string, std::unique_ptr<BufferHandle>> buffers_;
};  // namespace fire::h5

}  // namespace fire::h5

#endif  // FIRE_H5_READER_H
