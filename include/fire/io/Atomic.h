#ifndef FIRE_IO_ATOMIC_H
#define FIRE_IO_ATOMIC_H

#include <type_traits>

#include <highfive/H5DataType.hpp>

namespace fire::io {

/**
 * HighFive supports many C++ "atomic" types that are used regularly.
 * In order to conform to our more flexible structure, I have isolated
 * their deduction into this (currently small) header file.
 *
 * This will make it easier to fold in future atomic types if we see fit.
 */
template <typename AtomicType>
using is_atomic =
    std::integral_constant<bool,
                           std::is_arithmetic<AtomicType>::value ||
                               std::is_same<AtomicType, std::string>::value>;

/// shorthand for easier use
template <typename AtomicType>
inline constexpr bool is_atomic_v = is_atomic<AtomicType>::value;

/**
 * Boolean enum aligned with h5py
 *
 * We serialize bools in the same method as h5py so that
 * Python-based analyses are easier.
 */
enum class Bool : bool {
  TRUE  = true,
  FALSE = false
};

/**
 * HighFive method for creating the enum data type
 */
HighFive::EnumType<Bool> create_enum_bool();

}  // namespace fire::h5

template<>
HighFive::DataType HighFive::create_datatype<fire::io::Bool>();

#endif
