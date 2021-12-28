#ifndef FIRE_H5_WRITER_H
#define FIRE_H5_WRITER_H

// using HighFive
#include <highfive/H5File.hpp>

#include "fire/h5/Atomic.h"

// configuration parameters
#include "fire/config/Parameters.h"

namespace fire::h5 {

/**
 * A HighFive::File specialized to fire's usecase.
 *
 * @TODO implement compression settings
 * @TODO should we cache dataset handles?
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
   * Get the name of this file
   */
  const std::string& name() const;

  /**
   * Get the number of entries in the file
   */
  inline std::size_t entries() const {
    return entries_;
  }

  template <typename AtomicType>
  void save(const std::string& path, long unsigned int i,
            const AtomicType& val) {
    static_assert(is_atomic_v<AtomicType>,
        "Type unsupported by HighFive as Atomic made its way to Writer::save");
    if (file_.exist(path)) {
      // we've already created the dataset for this path
      HighFive::DataSet set = file_.getDataSet(path);
      std::vector<std::size_t> curr_dims = set.getDimensions();
      if (curr_dims.at(0) < i+1) {
        set.resize({i+1});
      }
      set.select({i}, {1}).write(val);
    } else {
      // we need to create a fully new dataset
      static const std::vector<size_t> limit = {HighFive::DataSpace::UNLIMITED};
      std::vector<size_t> initial_size = {i+1}; // or {entries_}
      HighFive::DataSpace space(initial_size, limit);
      HighFive::DataSetCreateProps props;
      // NOTE this is where chunking is done
      props.add(HighFive::Chunking({rows_per_chunk_}));
      props.add(HighFive::Shuffle()); // not sure what this does
      props.add(HighFive::Deflate(compression_level_));
      // set access properties to their default
      // and create parents to 'true' (default - create necessary groups along the way)
      HighFive::DataSet set = file_.createDataSet(path, space, HighFive::AtomicType<AtomicType>(), props);
      set.select({i}, {1}).write(val);
    }
  }

  friend std::ostream& operator<<(std::ostream& s, const Writer& w) {
    return s << "Writer(" << w.file_.getName() << ")";
  }

 private:
  /// our highfive file
  HighFive::File file_;
  /// number of rows to keep in each chunk
  std::size_t rows_per_chunk_;
  /// compression level (0-9), 0 being no compression
  int compression_level_;
  /// the expected number of entries in this file
  std::size_t entries_;
};

}  // namespace fire::h5

#endif  // FIRE_H5_WRITER_H
