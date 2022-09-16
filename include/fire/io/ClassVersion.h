#ifndef FIRE_IO_CLASSVERSION_H
#define FIRE_IO_CLASSVERSION_H

namespace fire::io {

/**
 * https://stackoverflow.com/a/9644512
 */
template<class T, class R = void>
struct enable_if_type { using type = R; };

template<class T, class Enable = void>
struct class_version_deducer {
  using version = std::integral_constant<int,0>;
};

template<class T>
struct class_version_deducer<T, typename enable_if_type<typename T::version>::type> {
  using version = typename T::version;
};

template <typename T>
inline constexpr int class_version = class_version_deducer<T>::version::value;

}

#define fire_class_version(VERS) \
  using version = std::integral_constant<int,VERS>;

#endif  // FIRE_IO_CLASSVERSION_H

