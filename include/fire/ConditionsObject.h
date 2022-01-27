#ifndef FIRE_CONDITIONSOBJECT_H
#define FIRE_CONDITIONSOBJECT_H

#include <string>

namespace fire {

/**
 * Base class for all conditions objects, very simple
 */
class ConditionsObject {
 public:
  /**
   * Define the name of the condition
   */
  ConditionsObject(const std::string& name) noexcept : name_(name) {}

  /**
   * Default destructor
   *
   * Virtual so that derived conditions can be destructed
   */
  virtual ~ConditionsObject() = default;

  /**
   * Get the name of this object
   * @return name of this condition
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
