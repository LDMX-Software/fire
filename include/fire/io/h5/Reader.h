#ifndef FIRE_IO_H5_READER_H
#define FIRE_IO_H5_READER_H

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/io/Reader.h"
#include "fire/io/Atomic.h"

namespace fire::io::h5 {

/**
 * Reading a file generated by fire
 *
 * @see Writer for our files are written with fire
 *
 * This reader handles the buffering of data read in from an HDF5 file
 * in a seamless manner so that individual entries can be requested at
 * a time without making disk read operation each time Reader::load is
 * called.
 */
class Reader : public ::fire::io::Reader {
 public:
  /**
   * Open the file in read mode
   *
   * Our read mode is the same as HDF5 Read Only mode
   *
   * We also read the number of entries in this file by 
   * retrieving the size of the data set at
   *  constants::EVENT_GROUP
   *    / constants::EVENT_HEADER_NAME
   *    / constants::NUMBER_NAME
   * This is reliable as long as 
   * 1. EventHeader attaches the event number (or a similar
   *    once-per-event atomic type) under the name constants::EVENT_HEADER_NUMBER
   * 2. The EventHeader is created under a io::Data named
   *    constants::EVENT_GROUP/constants::EVENT_HEADER_NAME
   *
   * We inspect the size of the dataset located at
   *  constants::RUN_HEADER_NAME/constants::NUMBER_NAME
   * in the file to get the number of runs. This will work as long as
   * 1. The RunHeader is written to the file as RUN_HEADER_NAME.
   * 2. The RunHeader attaches an atomic type member under the name `number`
   *
   * @throws HighFive::Exception if file is not accessible.
   * @param[in] name file name to open and read
   */
  Reader(const std::string& name);

  /**
   * Load the next event into the passed data
   *
   * As instructed by ::fire::io::Reader, we simply
   * call the data's load function with a reference to
   * ourselves.
   *
   * @param[in] d Data to load data into
   */
  virtual void load_into(BaseData& d) final override;

  /**
   * Get the event objects available in the file
   *
   * We crawl the internal directory structure of the file we have open.
   *
   * @see type for how we get the name of the class that was used
   * to write the data
   *
   * @return vector of string pairs `{ obj_name, pass }`
   */
  virtual std::vector<std::pair<std::string,std::string>> availableObjects() final override;

  /**
   * Get the type of the input object
   *
   * We retrieve the attributes named TYPE_ATTR_NAME and VERS_ATTR_NAME 
   * from the group located at EVENT_GROUP/obj_name. This enables us to 
   * construct the list of products within an inputfile in Event::setInputFile.
   *
   * @see io::Writer::setTypeName for where the type attr is set
   *
   * @param[in] obj_name Name of event object to retrieve type of
   * @return demangled type name in string format and its version number
   */
  virtual std::pair<std::string,int> type(const std::string& path) final override;

  /**
   * Get the name of this file
   * @return name of the file we are reading
   */
  virtual std::string name() const final override;

  /**
   * List the entries inside of a group.
   *
   * @note This is low-level and is only being used in 
   * io::ParameterStorage for more flexibility in the parameter maps
   * and Event::setInputFile to obtain the event products within a file.
   *
   * @param[in] group_path full in-file path to H5 group to list elements of
   * @return list of elements in the group, empty if group does not exist
   */
  std::vector<std::string> list(const std::string& group_path) const;

  /**
   * Deduce the type of the dataset requested.
   *
   * @note This is low-level and is only being used in io::ParameterStorage
   * for more flexibility in the parameter maps.
   *
   * @param[in] dataset full in-file path to H5 dataset
   * @return HighFive::DataType specifying the atomic type of the set
   */
  HighFive::DataType getDataSetType(const std::string& dataset) const;

  /**
   * Get the H5 type of object at the input path
   * @param[in] path in-file path to an HDF5 object
   * @return HighFive::ObjectType defining the type that the object there is
   */
  HighFive::ObjectType getH5ObjectType(const std::string& path) const;

  /**
   * Get the 'type' attribute from the input group name
   *
   * We retrieve the attribute named TYPE_ATTR_NAME from the group
   * located at EVENT_GROUP/obj_name. This enables us to construct
   * the list of products within an inputfile in Event::setInputFile.
   *
   * @see io::Writer::setTypeName for where the type attr is set
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
  inline std::size_t entries() const final override { return entries_; }

  /**
   * Get the number of runs in the file
   *
   * This value was determined upon construction
   *
   * @return number of runs within the file
   */
  inline std::size_t runs() const final override { return runs_; }

  /**
   * We can copy
   * @return true
   */
  virtual bool canCopy() const final override { return true; }

  /**
   * Copy the input data set to the output file
   *
   * This happens when the input data set has passed all of the drop/keep
   * rules so it is supposed to be copied into the output file; however,
   * noone has accessed it with Event::get yet so an in-memory class object
   * has not been created for it yet.
   *
   * @param[in] i_entry entry we are currently on
   * @param[in] path full event object name
   * @param[in] output handle to the writer writing the output file
   */
  virtual void copy(unsigned long int i_entry, const std::string& path, 
      Writer& output) final override;

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

  /// never want to copy a reader
  Reader(const Reader&) = delete;
  /// never want to copy a reader
  void operator=(const Reader&) = delete;

 private:
  /**
   * Mirror the structure of the passed path from us into the output file
   *
   * @param[in] path path to the group (and potential subgroups) to mirror
   * @param[in] output output file to mirror this structure to
   */
  void mirror(const std::string& path, Writer& output);

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
     * due to the specialization of it **and** to translate
     * our custom enum fire::io::Bool into bools.
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
        /**
         * compile-time split for bools which
         * 1. gets around the std::vector<bool> specialization
         * 2. allows us to translate io::Bool into bools
         */
        std::vector<Bool> buff;
        buff.resize(request_len);
        this->set_.select({i_file_}, {request_len})
          .read(buff.data(),create_enum_bool());
        buffer_.reserve(buff.size());
        for (const auto& v : buff) buffer_.push_back(v == Bool::TRUE);
      } else {
        this->set_.select({i_file_}, {request_len}).read(buffer_);
      }
      // update indices
      i_file_ += buffer_.size();
      i_memory_ = 0;
    }
  };

 private:
  /**
   * A mirror event object
   *
   * This type of event object is merely present to "reflect" (pun intended)
   * the recursive nature of the io::Data pattern _without_ knowledge of
   * any user classes. We need this so we can effectively copy event objects
   * that are not accessed during processing from the input to the output
   * file. (The choice on whether to copy or not copy these files is 
   * handled by Event).
   */
  class MirrorObject {
    /// handle to the reader we are reading from
    Reader& reader_;
    /// handle to the atomic data type once we get down to that point
    std::unique_ptr<BaseData> data_;
    /// handle to the size member of this object (if it exists)
    std::unique_ptr<BaseData> size_member_;
    /// list of sub-objects within this object
    std::vector<std::unique_ptr<MirrorObject>> obj_members_;
    /// the last entry that was copied
    unsigned long int last_entry_{0};

   public:
    /**
     * Construct this mirror object and any of its (data or object) children
     */
    MirrorObject(const std::string& path, Reader& reader);

    /**
     * Copy the n entries starting from i_entry
     */
    void copy(unsigned long int i_entry, unsigned long int n, Writer& output);
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
  /// our in-memory mirror objects for data being copied to the output file without processing
  std::unordered_map<std::string, std::unique_ptr<MirrorObject>> mirror_objects_;
};  // Reader

}  // namespace fire::io::h5

#endif  // FIRE_IO_H5_READER_H
