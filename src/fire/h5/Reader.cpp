#include "fire/h5/Reader.hpp"

namespace fire::h5 {

Reader::Reader(const std::string& name) : file_{name} {
  // TODO unify this with the event header constant
  entries_ =
      file_.getDataSet("events/EventHeader/number").getDimensions().at(0);
}

const std::string& Reader::name() const { return file_.getName(); }

}  // namespace fire::h5
