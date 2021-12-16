#ifndef FIRE_CONDITIONS_BASE_HPP
#define FIRE_CONDITIONS_BASE_HPP

#include <string>

namespace fire {
namespace conditions {

/**
 * @class Base
 * @brief Base class for all conditions objects, very simple
 */
class Base {
 public:
  /**
   * Class constructor
   */
  Base(const std::string& name) noexcept : name_(name) {}

  /**
   * Destructor
   */
  virtual ~Base() = default;

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

}  // namespace conditions
}  // namespace fire

#endif
