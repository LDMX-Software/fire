#ifndef FIRE_H5_WRITER_H
#define FIRE_H5_WRITER_H

#include <unordered_map>

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/config/Parameters.h"
#include "fire/h5/Atomic.h"

namespace fire::h5 {

/**
 * Write the fire DataSets into a deterministic structure
 * in the output HDF5 data file.
 *
 * @see h5::Reader for where our files are read
 */
class Writer {
 public:
  /**
   * Open the file in write mode
   *
   * Our write mode is the  HDF5 TRUNC (overwrite) mode.
   *
   * @param[in] event_limit Maximum number of events that could end up here
   * @param[in] ps Parameters used to configure the writing of the output file
   */
  Writer(const int& event_limit, const config::Parameters& ps);

  /**
   * Close up our file, making sure to flush contents to disk
   */
  ~Writer();

  /**
   * Flush the data to disk
   *
   * We flush all buffers and then flush the file.
   */
  void flush();

  /**
   * Get the name of this file
   */
  const std::string& name() const;

  /**
   * Set the name of the type stored in this Group (or DataSet if atomic)
   *
   * We look for the HDF5 object (DataSet or Group) at
   *  constants::EVENT_GROUP/full_obj_name
   * and then create an attribute for that object named
   *  constants::TYPE_ATTR_NAME
   * with the input value for the type.
   *
   * @throws HighFive::FileException if object doesn't exist
   *
   * @param[in] full_obj_name event object name including pass
   * @param[in] type demangled type name of object
   */
  void setTypeName(const std::string& full_obj_name, const std::string& type);

  /**
   * Get the number of entries in the file
   */
  inline std::size_t entries() const { return entries_; }

  /**
   * Save an atomic type into the dataset at the passed path
   *
   * If the path does not have a Buffer created for it yet,
   * we create a new one. Creating this new buffer is when
   * HDF5 creates the DataSet in the file and so it may
   * throw an exception at this time if it is unable to create
   * the DataSet for some reason.
   *
   * If the path already has a Buffer, we simply pass the new
   * value to be saved into the Buffer.
   *
   * @throws std::bad_cast if mismatched type is passed to buffer
   * @throws HighFive::DataSetException if unable to create data set
   */
  template <typename AtomicType>
  void save(const std::string& path, const AtomicType& val) {
    static_assert(
        is_atomic_v<AtomicType>,
        "Type unsupported by HighFive as Atomic made its way to Writer::save");
    if (buffers_.find(path) == buffers_.end()) {
      // first save attempt, need to create the data set to be saved
      // - we pass the newly created dataset to the buffer to hold onto
      //    for flushing purposes
      // - the length of the buffer is the same size as the chunks in
      //    HDF5, this is done on purpose
      buffers_.emplace(
          path, std::make_unique<Buffer<AtomicType>>(
                    rows_per_chunk_,
                    file_.createDataSet(path, space_,
                                        HighFive::AtomicType<AtomicType>(),
                                        create_props_)));
    }
    dynamic_cast<Buffer<AtomicType>&>(*buffers_.at(path)).save(val);
  }

  /**
   * Stream this writer
   *
   * We stream 'Writer' and the name of the file we are writing to
   *
   * @param[in] s ostream to stream into
   * @param[in] w writer to stream
   * @return modified ostream
   */
  friend std::ostream& operator<<(std::ostream& s, const Writer& w) {
    return s << "Writer(" << w.name() << ")";
  }

  /// don't allow copies of the Writer
  Writer(const Writer&) = delete;
  /// don't allow copies of the Writer
  void operator=(const Writer&) = delete;

 private:
  /**
   * Type-less handle to buffers
   *
   * This type-less handle allows us to flush the buffers
   * at the end of processing safely as well as maintain
   * a single map of all buffers we are managing.
   */
  class BufferHandle {
   protected:
    /// the maximum size of the buffer
    std::size_t max_len_;
    /// the H5 dataset we are writing to
    HighFive::DataSet set_;

   public:
    /**
     * Define the maximum size of the buffer and the dataset we are writing to
     *
     * @param[in] max size of buffer
     * @param[in] s dataset to write to
     */
    explicit BufferHandle(std::size_t max, HighFive::DataSet s)
        : max_len_{max}, set_{s} {}
    /**
     * virtual destructor so derived Buffer can be destructed properly
     */
    virtual ~BufferHandle() = default;
    /**
     * Pure virtual flush mechanism
     *
     * This is designed in the derived Buffer where we know the type of data
     * we are flushing to disk. This is also a helpful virtual method so
     * that we can flush the buffers in Writer::flush without know the types
     * stored within them.
     */
    virtual void flush() = 0;
  };

  /**
   * Buffer atomic data types in-memory for efficient disk operations
   *
   * @tparam AtomicType the data type we are buffering
   */
  template <typename AtomicType>
  class Buffer : public BufferHandle {
    /// the actual buffer of data in-memory
    std::vector<AtomicType> buffer_;
    /// the index of the file we will write to on the next flush
    std::size_t i_file_;

   public:
    /**
     * Define the buffer size and the set we will write to
     *
     * We also use std::vector::reserve to let the memory
     * handler know the size of our buffer. This can help
     * us avoid unnecessary copying and reallocation while
     * using std::vector::push_back to insert elements into
     * the vector.
     *
     * @param[in] max buffer size
     * @param[in] s dataset to write to
     */
    explicit Buffer(std::size_t max, HighFive::DataSet s)
        : BufferHandle(max, s), buffer_{}, i_file_{0} {
      buffer_.reserve(this->max_len_);
    }
    /// destruct the in-memory buffer
    virtual ~Buffer() = default;
    /**
     * Put the new value into the buffer
     *
     * If the buffer goes over the maximum length of the buffer,
     * then we call Buffer::flush
     *
     * @param[in] val data to append to the dataset
     */
    void save(const AtomicType& val) {
      buffer_.push_back(val);
      if (buffer_.size() > this->max_len_) flush();
    }
    /**
     * Flush our in-memory buffer onto disk
     *
     * We leave early if the buffer is empty.
     * This is helpful for the case where the number of elements
     * in the dataset happen to be an exact multiple of the buffer
     * size. Then the buffer would be empty at the time that
     * Writer::~Writer is called which calls all Buffers to flush
     * in order to avoid data loss.
     *
     * We determine the new extent of the dataset given how many
     * elements are in the buffer, then we resize the dataset
     * that is on disk to this new extent.
     *
     * Then, we copy the buffer into the DataSet on disk using
     * a compile-time choice that handles the std::vector<bool>
     * specialization
     * [bug in HighFive](https://github.com/BlueBrain/HighFive/issues/490).
     *
     * Finally, we update the file index, clear the buffer,
     * and re-reserve the maximum length of the buffer to prepare
     * for another chunk of data.
     *
     * @throws HighFive::DataSetException if unable to extend or
     * write to the DataSet.
     */
    virtual void flush() final override {
      if (buffer_.size() == 0) return;
      std::size_t new_extent = i_file_ + buffer_.size();
      // throws if not created yet
      if (this->set_.getDimensions().at(0) < new_extent) {
        this->set_.resize({new_extent});
      }
      if constexpr (std::is_same_v<AtomicType, bool>) {
        // handle bool specialization
        auto buff = std::make_unique<bool[]>(buffer_.size());
        for (std::size_t i{0}; i < buffer_.size(); i++)
          buff[i] = buffer_.at(i);
        this->set_.select({i_file_}, {buffer_.size()})
          .write_raw(buff.get(), HighFive::AtomicType<bool>());
      } else {
        this->set_.select({i_file_}, {buffer_.size()}).write(buffer_);
      }
      i_file_ += buffer_.size();
      buffer_.clear();
      buffer_.reserve(this->max_len_);
    }
  };

 private:
  /// our highfive file
  HighFive::File file_;
  /// the creation properties to be used on datasets we are writing
  HighFive::DataSetCreateProps create_props_;
  /// the dataspace shared amongst all of our datasets
  HighFive::DataSpace space_;
  /// the expected number of entries in this file
  std::size_t entries_;
  /// number of rows to keep in each chunk
  std::size_t rows_per_chunk_;
  /// our in-memory buffers for data to be written to disk
  std::unordered_map<std::string, std::unique_ptr<BufferHandle>> buffers_;
};

}  // namespace fire::h5

#endif  // FIRE_H5_WRITER_H
