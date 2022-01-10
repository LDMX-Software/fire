#ifndef FIRE_H5_READER_H
#define FIRE_H5_READER_H

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/h5/Atomic.h"

namespace fire::h5 {

/**
 * A HighFive::File specialized to fire's usecase.
 *
 * @TODO should we cache dataset handles?
 * @TODO need to write vecotr<Atomic> load?
 */
class Reader {
 public:
  /// the name of the event header data set
  static const std::string EVENT_HEADER_NAME;
  /// the name of the group holding all of the events
  static const std::string EVENT_GROUP;
  /// the name of the run header data set
  static const std::string RUN_HEADER_NAME;
  /// the name of the attribute that stores the type of event object
  static const std::string TYPE_ATTR_NAME;

 public:
  /**
   * Open the file in read mode
   *  our read  == HDF5 Read Only mode
   */
  Reader(const std::string& name);

  /**
   * Get the name of this file
   */
  const std::string& name() const;

  /**
   * List the entries inside of a group.
   *
   * @note This is low-level and is only being used
   * in the EventHeader and RunHeader serialization specializations
   * for more flexibility in their parameter maps.
   */
  std::vector<std::string> list(const std::string& group_path) const;

  /**
   * Deduce the type of the dataset requested.
   */
  HighFive::DataTypeClass getDataSetType(const std::string& dataset) const;

  /**
   * Get the 'type' attribute from the input group name
   */
  std::string getTypeName(const std::string& obj_name) const;

  /**
   * Get the number of entries in the file
   */
  inline std::size_t entries() const { return entries_; }

  /**
   * Get the number of runs in the file
   *  Actually looks at entries on disk, so use should be limited.
   */
  std::size_t runs() const;

  /**
   * Try to load a single value of an atomic type into the input handle
   */
  template <typename AtomicType>
  void load(const std::string& path, long unsigned int i, AtomicType& val) {
    static_assert(
        is_atomic_v<AtomicType>,
        "Type not supported by HighFive atomic made its way to Reader::load");
    if (buffers_.find(path) == buffers_.end()) {
      // first load attempt, we will find out if dataset exists in file here
      buffers_.emplace(path, std::make_unique<Buffer<AtomicType>>(
                                 rows_per_chunk_, file_.getDataSet(path)));
      // if dataset exists, then lets load the first chunk into memory
      buffers_[path]->load();
    }

    try {
      dynamic_cast<Buffer<AtomicType>&>(*buffers_[path]).read(val);
    } catch (const std::bad_cast&) {
      throw Exception("Attempting to read mis-matched type.");
    }
  }

  friend std::ostream& operator<<(std::ostream& s, const Reader& r) {
    return s << "Reader(" << r.file_.getName() << ")";
  }

  Reader(const Reader&) = delete;
  void operator=(const Reader&) = delete;

 private:
  class BufferHandle {
   protected:
    std::size_t max_len_;
    HighFive::DataSet set_;

   public:
    explicit BufferHandle(std::size_t max, HighFive::DataSet s)
        : max_len_{max}, set_{s} {}
    virtual ~BufferHandle() = default;
    virtual void load() = 0;
  };

  template <typename AtomicType>
  class Buffer : public BufferHandle {
    std::vector<AtomicType> buffer_;
    std::size_t i_file_;
    std::size_t i_memory_;
    std::size_t entries_;

   public:
    explicit Buffer(std::size_t max, HighFive::DataSet s)
        : BufferHandle(max, s), buffer_{}, i_file_{0}, i_memory_{0} {
      // get the number of entries for later checking
      entries_ = this->set_.getDimensions().at(0);
      // do first load upon creation
      this->load();
    }
    virtual ~Buffer() = default;
    void read(AtomicType& out) {
      if (i_memory_ == buffer_.size()) this->load();
      out = buffer_[i_memory_];
      i_memory_++;
    }
    virtual void load() final override {
      // determine the length we want to request depending
      // on the number of entries left in the file
      std::size_t request_len = this->max_len_;
      if (request_len + i_file_ > entries_) {
        request_len = entries_ - i_file_;
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
  /// the number of entries in this file
  std::size_t entries_{0};
  /// the number of rows to keep in each chunk
  std::size_t rows_per_chunk_{10000};
  /// our in-memory buffers for the data to be read in from disk
  std::unordered_map<std::string, std::unique_ptr<BufferHandle>> buffers_;
};  // namespace fire::h5

}  // namespace fire::h5

#endif  // FIRE_H5_READER_H
