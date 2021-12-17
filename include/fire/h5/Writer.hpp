#ifndef FIRE_H5_WRITER_HPP
#define FIRE_H5_WRITER_HPP

// using HighFive
#include <highfive/H5File.hpp>

// configuration parameters
#include "fire/config/Parameters.hpp"

namespace fire {
namespace h5 {

/**
 * A HighFive::File specialized to fire's usecase.
 *
 * @TODO implement compression and chunking settings
 * @TODO should we cache dataset handles?
 * @TODO need to write vecotr<Atomic> save?
 */
class Writer {
 public:
  /**
   * Open the file in write mode
   *  our write == HDF5 TRUNC (overwrite) mode
   */
  Writer(const config::Parameters& ps)
      : file_(ps.get<std::string>("name"), HighFive::File::Create | HighFive::File::Truncate),
        rows_per_chunk_{ps.get<int>("rows_per_chunk")} {
          entries_ = ps.get<int>("event_limit");
        }

  /**
   * Close up our file, making sure to flush contents to disk if writing
   */
  ~Writer() {
    file_.flush();
  }

  /**
   * Get the number of entries in the file
   */
  inline std::size_t entries() const {
    return entries_;
  }

  template <typename AtomicType>
  void save(const std::string& path, long unsigned int i,
            const AtomicType& val) {
    static_assert(std::is_arithmetic_v<AtomicType>,
                  "Non-arithmetic type made its way to save");
    if (file_.exist(path)) {
      // we've already created the dataset for this path
      HighFive::DataSet set = file_.getDataSet(path);
      std::vector<std::size_t> curr_dims = set.getDimensions();
      if (curr_dims.at(0) < i + 1) {
        set.resize({i + 1});
      }
      set.select({i}, {1}).write(val);
    } else {
      // we need to create a fully new dataset
      static const std::vector<size_t> limit = {HighFive::DataSpace::UNLIMITED};
      std::vector<size_t> initial_size = {entries_+1};
      HighFive::DataSpace space(initial_size, limit);
      HighFive::DataSetCreateProps props;
      props.add(HighFive::Chunking(
          {rows_per_chunk_}));  // NOTE this is where chunking is done
      // TODO compression
      // TODO creation options in this function
      HighFive::DataSet set = file_.createDataSet(
          path, space, HighFive::AtomicType<AtomicType>(), props, {}, true);
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
  /// the expected number of entries in this file
  std::size_t entries_{0};
};

}  // namespace h5
}  // namespace fire

#endif  // FIRE_H5_WRITER_HPP
