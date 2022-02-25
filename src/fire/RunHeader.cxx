#include "fire/RunHeader.h"

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <iostream>

namespace fire {

const std::string RunHeader::NAME = io::constants::RUN_HEADER_NAME;

void RunHeader::stream(std::ostream &s) const {
  s << "RunHeader { run: " << getRunNumber()
    << ", detectorName: " << getDetectorName()
    << ", description: " << getDescription() << "\n";
  /*
  s << "  parameters: "
    << "\n";
  for (const auto &[key, val] : parameters_)
    s << "    " << key << " = " << val << "\n";
    */
  s << "}";
}

void RunHeader::Print() const { stream(std::cout); }

}  // namespace fire
