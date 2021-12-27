#ifndef MYMODULE_MYOBJECT_H
#define MYMODULE_MYOBJECT_H

// STL
#include <iostream>

// H5
#include "fire/h5/DataSet.h"

namespace mymodule {

class MyObject {
 public:
  void Print() const;
  void clear();
  int getID() const { return id_; };
  int getLayerID() const { return layerID_; };
  int getModuleID() const { return moduleID_; };
  std::vector<float> getPosition() const { return {x_, y_, z_}; };
  float getEdep() const { return edep_; };
  float getEnergy() const { return energy_; };
  float getTime() const { return time_; };
  std::vector<double> getMomentum() const { return {px_, py_, pz_}; };
  int getTrackID() const { return trackID_; };
  void setID(const long id) { this->id_ = id; };
  void setLayerID(const int layerID) { this->layerID_ = layerID; };
  void setModuleID(const int moduleID) { this->moduleID_ = moduleID; };
  void setPosition(const float x, const float y, const float z);
  void setEdep(const float edep) { this->edep_ = edep; };
  void setEnergy(const float energy) { energy_ = energy; };
  void setTime(const float time) { this->time_ = time; };
  void setMomentum(const float px, const float py, const float pz);
  void setTrackID(const int simTrackID) { this->trackID_ = simTrackID; };
 private:
  friend class fire::h5::DataSet<MyObject>;
  void attach(fire::h5::DataSet<MyObject>& set) {
    set.attach("id",id_);
    set.attach("layerID", layerID_);
    set.attach("moduleID", moduleID_);
    set.attach("edep", edep_);
    set.attach("time", time_);
    set.attach("px", px_);
    set.attach("py", py_);
    set.attach("pz", pz_);
    set.attach("energy",energy_);
    set.attach("x", x_);
    set.attach("y", y_);
    set.attach("z", z_);
    set.attach("trackID",trackID_);
  }

 private:
  int id_{0};
  int layerID_{0};
  int moduleID_{0};
  float edep_{0};
  float time_{0};
  float px_{0};
  float py_{0};
  float pz_{0};
  float energy_{0};
  float x_{0};
  float y_{0};
  float z_{0};
  int trackID_{0};
};  // MyObject
}  // namespace bench

#endif
