#ifndef FIRE_H5_WRITER_H
#define FIRE_H5_WRITER_H

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/config/Parameters.h"
#include "fire/h5/Atomic.h"

namespace fire::h5 {

/**
 * A HighFive::File specialized to fire's usecase.
 *
 * @TODO need to write vecotr<Atomic> save?
 */
class Writer {
 public:
  /**
   * Open the file in write mode
   *  our write == HDF5 TRUNC (overwrite) mode
   */
  Writer(const int& event_limit, const config::Parameters& ps);

  /**
   * Close up our file, making sure to flush contents to disk if writing
   */
  ~Writer();

  /**
   * Flush the data to disk
   */
  void flush();

  /**
   * Get the name of this file
   */
  const std::string& name() const;

  /**
   * Get the number of entries in the file
   */
  inline std::size_t entries() const { return entries_; }

  template <typename AtomicType>
  void save(const std::string& path, long unsigned int i,
            const AtomicType& val) {
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
    try {
      dynamic_cast<Buffer<AtomicType>&>(*buffers_.at(path)).save(val);
    } catch (const std::bad_cast&) {
      throw Exception("Attempting to insert incorrect type into buffer.");
    }
  }

  friend std::ostream& operator<<(std::ostream& s, const Writer& w) {
    return s << "Writer(" << w.file_.getName() << ")";
  }

  Writer(const Writer&) = delete;
  void operator=(const Writer&) = delete;

 private:
  class BufferHandle {
   protected:
    std::size_t max_len_;
    HighFive::DataSet set_;

   public:
    explicit BufferHandle(std::size_t max, HighFive::DataSet s)
        : max_len_{max}, set_{s} {}
    virtual ~BufferHandle() = default;
    virtual void flush() = 0;
  };

  template <typename AtomicType>
  class Buffer : public BufferHandle {
    std::vector<AtomicType> buffer_;
    std::size_t i_output_;

   public:
    explicit Buffer(std::size_t max, HighFive::DataSet s)
        : BufferHandle(max, s), buffer_{}, i_output_{0} {
      buffer_.reserve(this->max_len_);
    }
    virtual ~Buffer() = default;
    void save(const AtomicType& val) {
      buffer_.push_back(val);
      if (buffer_.size() > this->max_len_) flush();
    }
    virtual void flush() final override {
      if (buffer_.size() == 0) return;
      std::size_t new_extent = i_output_ + buffer_.size();
      // throws if not created yet
      if (this->set_.getDimensions().at(0) < new_extent) {
        this->set_.resize({new_extent});
      }
      if constexpr (std::is_same_v<AtomicType, bool>) {
        // handle bool specialization
        std::vector<short> buff;
        buff.reserve(buffer_.size());
        for (const auto& v : buffer_) buff.push_back(v);
        this->set_.select({i_output_}, {buff.size()}).write(buff);
      } else {
        this->set_.select({i_output_}, {buffer_.size()}).write(buffer_);
      }
      i_output_ += buffer_.size();
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
