#include "fire/RunHeader.h"

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <iostream>

#ifdef fire_USE_ROOT
ClassImp(ldmx::RunHeader);
namespace ldmx {
#else
namespace fire {
#endif

const std::string RunHeader::NAME = fire::io::constants::RUN_HEADER_NAME;

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

}  // namespace fire/ldmx

#ifdef fire_USE_ROOT
namespace fire::io {

Data<ldmx::RunHeader>::Data(const std::string& path, Reader* input_file, ldmx::RunHeader* eh)
  : AbstractData<ldmx::RunHeader>(path,input_file, eh), input_file_{input_file} {
  fire::io::access::connect(*this->handle_, *this);
}

void Data<ldmx::RunHeader>::load(h5::Reader& r) {
  for (auto& m : members_) m->load(r);
}

void Data<ldmx::RunHeader>::load(root::Reader& r) {
  // load ROOT stuff
  r.load(this->path_, *(this->handle_));

  // copy into new containers
  for (const auto& [key, val] : this->handle_->intParameters_)
    this->handle_->set(key,val);
  for (const auto& [key, val] : this->handle_->floatParameters_)
    this->handle_->set(key,val);
  for (const auto& [key, val] : this->handle_->stringParameters_)
    this->handle_->set(key,val);
}

void Data<ldmx::RunHeader>::save(Writer& w) {
  for (auto& m : members_) m->save(w);
}

void Data<ldmx::RunHeader>::structure(Writer& w) {
  w.structure(this->path_, this->save_type_);
  for (auto& m : members_) m->structure(w);
}

}
#endif
