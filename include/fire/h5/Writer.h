#ifndef FIRE_H5_WRITER_H
#define FIRE_H5_WRITER_H

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/h5/Atomic.h"

// configuration parameters
#include "fire/config/Parameters.h"

namespace fire::h5 {

class BufferHandle {
 public:
  BufferHandle() = default;
  virtual ~BufferHandle() = default;
  virtual void flush(HighFive::File&, const std::string&) = 0;
};

template <typename AtomicType>
class Buffer : public BufferHandle {
  std::vector<AtomicType> buffer_;
  std::size_t i_output_;
  std::size_t max_buffer_leng_;

 public:
  explicit Buffer(std::size_t max)
      : BufferHandle(),
        buffer_{},
        i_output_{0},
        max_buffer_leng_{max} {}
  virtual ~Buffer() { 
    buffer_.clear();
  }
  void save(HighFive::File& f, const std::string& path, const AtomicType& val) {
    buffer_.push_back(val);
    if (buffer_.size() > max_buffer_leng_) flush(f,path);
  }
  virtual void flush(HighFive::File& f, const std::string& path) final override {
    if (buffer_.size() == 0) return;
    std::size_t new_extent = i_output_ + buffer_.size();
    // throws if not created yet
    HighFive::DataSet set = f.getDataSet(path);
    if (set.getDimensions().at(0) < new_extent) {
      set.resize({new_extent});
    }
    if constexpr (std::is_same_v<AtomicType,bool>) {
      // handle bool specialization
      std::vector<short> buff;
      for (const auto& v : buffer_) buff.push_back(v);
      set.select({i_output_}, {buffer_.size()}).write(buff);
    } else {
      set.select({i_output_}, {buffer_.size()}).write(buffer_);
    }
    i_output_ += buffer_.size();
    buffer_.clear();
  }
};

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
      static const std::vector<std::size_t> initial = {0};
      static const std::vector<std::size_t> limit = {
          HighFive::DataSpace::UNLIMITED};
      HighFive::DataSpace space(initial, limit);
      HighFive::DataSetCreateProps props;
      props.add(HighFive::Chunking({rows_per_chunk_}));
      if (shuffle_) props.add(HighFive::Shuffle());
      props.add(HighFive::Deflate(compression_level_));
      file_.createDataSet(path, space, HighFive::AtomicType<AtomicType>(),
                          props);
      buffers_.emplace(path, std::make_unique<Buffer<AtomicType>>(rows_per_chunk_));
    }
    try {
      dynamic_cast<Buffer<AtomicType>&>(*buffers_.at(path)).save(file_, path, val);
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
  /// our highfive file
  HighFive::File file_;
  /// should we apply the HDF5 shuffle filter?
  bool shuffle_;
  /// compression level
  int compression_level_;
  /// the expected number of entries in this file
  std::size_t entries_;
  /// number of rows to keep in each chunk
  std::size_t rows_per_chunk_;
  std::unordered_map<std::string, std::unique_ptr<BufferHandle>> buffers_;
};

}  // namespace fire::h5

#endif  // FIRE_H5_WRITER_H
