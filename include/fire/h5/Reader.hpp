#ifndef FIRE_H5_READER_HPP
#define FIRE_H5_READER_HPP

// using HighFive
#include <highfive/H5File.hpp>

namespace fire {
namespace h5 {

/**
 * A HighFive::File specialized to fire's usecase.
 *
 * @TODO should we cache dataset handles?
 * @TODO need to write vecotr<Atomic> load?
 */
class Reader {
 public:
  /**
   * Open the file in read mode
   *  our read  == HDF5 Read Only mode
   */
  Reader(const std::string& name)
      : file_(name) {
          entries_ = file_.getDataSet("events/"+EventHeader::NAME+"/number").getSize();
        }

  /**
   * Get the number of entries in the file
   */
  inline std::size_t entries() const {
    return entries_;
  }

  /**
   * Try to load a single value of an atomic type into the input handle
   */
  template <typename AtomicType>
  void load(const std::string& path, long unsigned int i, AtomicType& val) {
    static_assert(std::is_arithmetic_v<AtomicType>,
                  "Non-arithmetic type made its way to load");
    HighFive::DataSet ds = file_.getDataSet(path);
    ds.select({i}, {1}).read(val);
  }

  friend std::ostream& operator<<(std::ostream& s, const File& f) {
    return s << "Reader(" << file_.getName() << ")";
  }

 private:
  /// our highfive file
  HighFive::File file_;
  /// the number of entries in this file
  std::size_t entries_{0};
};

}  // namespace h5
}  // namespace fire

#endif  // FIRE_H5_READER_HPP
