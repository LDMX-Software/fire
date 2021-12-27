#ifndef FIRE_CONDITIONSOBJECT_H
#define FIRE_CONDITIONSOBJECT_H

#include <string>

namespace fire {

/**
 * @class ConditionsObject
 * @brief Base class for all conditions objects, very simple
 */
class ConditionsObject {
 public:
  /**
   * Class constructor
   */
  ConditionsObject(const std::string& name) noexcept : name_(name) {}

  /**
   * Destructor
   */
  virtual ~ConditionsObject() = default;

  /**
   * Get the name of this object
   */
  inline std::string getName() const { return name_; }

 private:
  /**
   * Name of the object
   */
  std::string name_;
};

}  // namespace fire

#endif
