#ifndef FIRE_H5_FILE_HPP
#define FIRE_H5_FILE_HPP

// using HighFive
#include <highfive/H5File.hpp>

namespace fire {
namespace h5 {

/**
 * A HighFive::File specialized to fire's usecase.
 */
class File {
 public:
  /**
   * Open the file in write or read mode
   *  our write == HDF5 TRUNC (overwrite) mode
   *  our read  == HDF5 Read Only mode
   */
  File(std::string const& name, bool write = false)
      : file_(name, write ? HighFive::File::Create | HighFive::File::Truncate : HighFive::File::ReadOnly),
        writing_{write} {}

  /**
   * Close up our file, making sure to flush contents to disk if writing
   */
  ~File() {
    if (writing_) file_.flush();
  }

  /**
   * Try to load a single value of an atomic type into the input handle
   */
  template <typename AtomicType>
  void load(const std::string& path, long unsigned int i, AtomicType& val) {
    static_assert(std::is_arithmetic_v<AtomicType>,
                  "Non-arithmetic type made its way to load");
    if (writing_) {
      // should never call 'load' on an output file
      throw std::runtime_error("Attempted to load data from the output file.");
    }

    HighFive::DataSet ds = file_.getDataSet(path);
    ds.select({i}, {1}).read(val);
  }

  template <typename AtomicType>
  void save(const std::string& path, long unsigned int i,
            AtomicType const& val) {
    static_assert(std::is_arithmetic_v<AtomicType>,
                  "Non-arithmetic type made its way to save");
    if (not writing_) {
      throw std::runtime_error("Attempted to save data to the input file.");
    }

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
      std::vector<size_t> initial_size = {i+1};
      HighFive::DataSpace space(initial_size, limit);
      HighFive::DataSetCreateProps props;
      props.add(HighFive::Chunking({10}));  // NOTE this is where chunking is done
      // TODO compression
      // TODO creation options in this function
      HighFive::DataSet set = file_.createDataSet(
          path, space, HighFive::AtomicType<AtomicType>(), props, {}, true);
      set.select({i},{1}).write(val);
    }
  }

 private:
  /// our highfive file
  HighFive::File file_;
  /// we are writing
  bool writing_;
};

}  // namespace h5
}  // namespace fire

#endif  // FIRE_H5_FILE_HPP
