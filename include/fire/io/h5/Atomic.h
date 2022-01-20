#ifndef FIRE_IO_H5_ATOMIC_H
#define FIRE_IO_H5_ATOMIC_H

#include <highfive/H5DataType.hpp>

namespace fire::io::h5 {

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

}  // namespace fire::h5

#endif
