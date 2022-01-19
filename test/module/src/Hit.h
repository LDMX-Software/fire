#ifndef BENCH_EVENT_HIT_H_
#define BENCH_EVENT_HIT_H_

// STL
#include <iostream>

// H5
#include "fire/h5/Data.h"

namespace bench {

/**
 * @class Hit
 * @brief Represents a simulated tracker hit in the simulation
 */
class Hit {
 public:
  /**
   * Class constructor.
   */
  Hit();

  /**
   * Class destructor.
   */
  virtual ~Hit();

  /**
   * Print a description of this object.
   */
  void Print() const;

  /**
   * Reset the Hit object.
   */
  void clear();

  /**
   * Get the geometric layer ID of the hit.
   * @return The layer ID of the hit.
   */
  int getLayerID() const { return layerID_; };

  /**
   * Get the module ID associated with a hit.  This is used to
   * uniquely identify a sensor within a layer.
   * @return The module ID associated with a hit.
   */
  int getModuleID() const { return moduleID_; };

  /**
   * Get the XYZ position of the hit [mm].
   * @return The position of the hit.
   */
  std::vector<float> getPosition() const { return {x_, y_, z_}; };

  /**
   * Get the energy
   * @return The energy of the hit.
   */
  float getEnergy() const { return energy_; };

  /**
   * Get the global time of the hit [ns].
   * @return The global time of the hit.
   */
  float getTime() const { return time_; };

  /**
   * Get the XYZ momentum of the particle at the position at which
   * the hit took place [MeV].
   * @return The momentum of the particle.
   */
  std::vector<double> getMomentum() const { return {px_, py_, pz_}; };

  /**
   * Get the Sim particle track ID of the hit.
   * @return The Sim particle track ID of the hit.
   */
  int getTrackID() const { return trackID_; };

  /**
   * Get the Sim particle track ID of the hit.
   * @return The Sim particle track ID of the hit.
   */
  int getPdgID() const { return pdgID_; };

  /**
   * Set the geometric layer ID of the hit.
   * @param layerID The layer ID of the hit.
   */
  void setLayerID(const int layerID) { this->layerID_ = layerID; };

  /**
   * Set the module ID associated with a hit.  This is used to
   * uniquely identify a sensor within a layer.
   * @return moduleID The module ID associated with a hit.
   */
  void setModuleID(const int moduleID) { this->moduleID_ = moduleID; };

  /**
   * Set the position of the hit [mm].
   * @param x The X position.
   * @param y The Y position.
   * @param z The Z position.
   */
  void setPosition(const float x, const float y, const float z);

  /**
   * Set the energy of the hit.
   * @param e The energy of the hit.
   */
  void setEnergy(const float energy) { energy_ = energy; };

  /**
   * Set the global time of the hit [ns].
   * @param time The global time of the hit.
   */
  void setTime(const float time) { this->time_ = time; };

  /**
   * Set the momentum of the particle at the position at which
   * the hit took place [GeV].
   * @param px The X momentum.
   * @param py The Y momentum.
   * @param pz The Z momentum.
   */
  void setMomentum(const float px, const float py, const float pz);

  /**
   * Set the Sim particle track ID of the hit.
   * @return The Sim particle track ID of the hit.
   */
  void setTrackID(const int simTrackID) { this->trackID_ = simTrackID; };

  /**
   * Set the Sim particle track ID of the hit.
   * @return The Sim particle track ID of the hit.
   */
  void setPdgID(const int simPdgID) { this->pdgID_ = simPdgID; };

  /**
   * Sort by time of hit
   */
  bool operator<(const bench::Hit &rhs) const {
    return this->getTime() < rhs.getTime();
  }

  /**
   * The layer ID.
   */
  int layerID_{0};

  /** The module ID. */
  int moduleID_{0};

  /**
   * The global time of the hit.
   */
  float time_{0};

  /**
   * The X momentum.
   */
  float px_{0};

  /**
   * The Y momentum.
   */
  float py_{0};

  /**
   * The Z momentum.
   */
  float pz_{0};

  /**
   * The total energy.
   */
  float energy_{0};

  /**
   * The X position.
   */
  float x_{0};

  /**
   * The Y position.
   */
  float y_{0};

  /**
   * The Z position.
   */
  float z_{0};

  /**
   * The Sim Track ID.
   */
  int trackID_{0};

  /**
   * The Sim PDG ID.
   */
  int pdgID_{0};

  friend class fire::h5::Data<Hit>;
  void attach(fire::h5::Data<Hit>& d) {
    d.attach("layerID", layerID_);
    d.attach("moduleID", moduleID_);
    d.attach("time", time_);
    d.attach("px", px_);
    d.attach("py", py_);
    d.attach("pz", pz_);
    d.attach("energy",energy_);
    d.attach("x", x_);
    d.attach("y", y_);
    d.attach("z", z_);
    d.attach("trackID",trackID_);
    d.attach("pdgID",pdgID_);
  }

};  // Hit
}  // namespace bench

#endif
