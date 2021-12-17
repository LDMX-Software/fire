/**
 * @file ProductTag.hpp
 * @brief Class defining the identity of a data product in the event
 * @author Jeremy Mans, University of Minnesota
 */

#ifndef FIRE_PRODUCTTAG_HPP
#define FIRE_PRODUCTTAG_HPP

#include <ostream>
#include <string>
#include <type_traits>
#include <regex>

namespace fire {

/**
 * @class ProductTag
 * @brief Defines the identity of a product and can be used for searches
 *
 */
class ProductTag {
 public:
  /**
   * Class constructor.
   *
   * Blank/empty parameters are generally considered as wildcards
   * when searches are performed.
   *
   * @param name Product name
   * @param pass Pass name
   * @param type Type name
   */
  ProductTag(const std::string& name, const std::string& pass,
             const std::string& type)
      : name_{name}, pass_{pass}, type_{type} {}

  /**
   * Get the product name
   */
  const std::string& name() const { return name_; }

  /**
   * Get the product pass name
   */
  const std::string& pass() const { return pass_; }

  /**
   * Get the product type name
   */
  const std::string& type() const { return type_; }

  /**
   * String method for printing this tag in a helpful manner
   */
  friend std::ostream& operator<<(std::ostream& os, const ProductTag& pt) {
    return os << pt.name() << "_" << pt.pass() << "_" << pt.type();
  }

  /**
   * Checks if we match the passed regex for name, pass, and type
   */
  inline bool match(const std::regex& name, const std::regex& pass, const std::regex& type) const {
    return std::regex_match(name_, name) and std::regex_match(pass_, pass) and std::regex_match(type_, type);
  }

 private:
  /**
   * Name given to the product
   */
  std::string name_;

  /**
   * Passname given when product was written
   */
  std::string pass_;

  /**
   * Typename of the product
   */
  std::string type_;
};

}  // namespace fire

#endif
