#ifndef FIRE_IO_CLASSVERSION_H
#define FIRE_IO_CLASSVERSION_H

namespace fire::io {

template<typename T>
struct class_version_deducer {
  using version = std::integral_constant<int,0>;
};

template <typename T>
inline constexpr int class_version = class_version_deducer<T>::version::value;

}

#define fire_class_version(CLASS,VERS) \
  template<> struct class_version_deducer<CLASS> { \
    using version = std::integral_constant<int,VERS>; \
  };

#endif  // FIRE_IO_CLASSVERSION_H

