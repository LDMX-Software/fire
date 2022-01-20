#include "Hit.h"

ClassImp(Hit);

namespace bench {

Hit::Hit() {}

Hit::~Hit() {}

void Hit::Print() const {
  std::cout << "Hit { "
            << "layerID: " << layerID_ << ", "
            << "moduleID: " << moduleID_ << ", "
            << "position: ( " << x_ << ", " << y_ << ", " << z_ << " ), "
            << "energy: " << energy_ << ", "
            << "time: " << time_ << ", "
            << "momentum: ( " << px_ << ", " << py_ << ", " << pz_ << " )"
            << " }" << std::endl;
}

void Hit::clear() {
  layerID_ = 0;
  moduleID_ = 0;
  time_ = 0;
  px_ = 0;
  py_ = 0;
  pz_ = 0;
  x_ = 0;
  y_ = 0;
  z_ = 0;
  energy_ = 0;
  trackID_ = -1;
  pdgID_ = 0;
}

void Hit::setPosition(const float x, const float y, const float z) {
  this->x_ = x;
  this->y_ = y;
  this->z_ = z;
}

void Hit::setMomentum(const float px, const float py, const float pz) {
  this->px_ = px;
  this->py_ = py;
  this->pz_ = pz;
}
}  // namespace bench
