
#include "fire/EventHeader.h"

#ifdef fire_USE_ROOT
ClassImp(ldmx::EventHeader);
namespace ldmx {
#else
namespace fire {
#endif
const std::string EventHeader::NAME = fire::io::constants::EVENT_GROUP
  +"/"+fire::io::constants::EVENT_HEADER_NAME;

}

#ifdef fire_USE_ROOT
namespace fire::io {

Data<ldmx::EventHeader>::Data(const std::string& path, Reader* input_file, ldmx::EventHeader* eh)
  : AbstractData<ldmx::EventHeader>(path,input_file, eh), input_file_{input_file} {
  fire::io::access::connect(*this->handle_, *this);
}

void Data<ldmx::EventHeader>::load(h5::Reader& r) {
  for (auto& m : members_) m->load(r);
}

void Data<ldmx::EventHeader>::load(root::Reader& r) {
  // load ROOT stuff
  r.load(this->path_, *(this->handle_));

  // copy into new containers
  for (const auto& [key, val] : this->handle_->intParameters_)
    this->handle_->set(key,val);
  for (const auto& [key, val] : this->handle_->floatParameters_)
    this->handle_->set(key,val);
  for (const auto& [key, val] : this->handle_->stringParameters_)
    this->handle_->set(key,val);
  this->handle_->time_ = this->handle_->timestamp_.GetSec();
}

void Data<ldmx::EventHeader>::save(Writer& w) {
  for (auto& m : members_) m->save(w);
}

void Data<ldmx::EventHeader>::structure(Writer& w) {
  w.structure(this->path_, this->save_type_);
  for (auto& m : members_) m->structure(w);
}

}
#endif
