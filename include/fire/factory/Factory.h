#ifndef FIRE_FACTORY_FACTORY_H
#define FIRE_FACTORY_FACTORY_H

#include <memory>         // for the unique_ptr default
#include <string>         // for the keys in the library map
#include <unordered_map>  // for the library of prototypes

#include "fire/exception/Exception.h"

/**
 * used to isolate base templated Factory class
 *
 * This namespace is used to isolate the templated Factory
 * from where other Factories are defined. There should be
 * nothing else in this namespace in order to avoid potential
 * name conflicts.
 */
namespace fire::factory {

/**
 * The Factory's exception object
 * Used by both the loadLibrary function as well as the base
 * factory class for registration and creation of objects
 * from prototypes.
 */
ENABLE_EXCEPTIONS();

/**
 * load a library by name
 *
 * Loading a library by name is similar to linking a shared library
 * to an executable; however, it does not include some assumptions
 * about the library naming that some linkers make.
 *
 * libname needs to include the 'lib' prefix and the '.so' suffix
 * as well as be "findable" by the linker ld. You can make any library
 * "findable" by passing the full path as 'libname' or you can add
 * the directory that the library is in to LD_LIBRARY_PATH.
 *
 * We maintain a cache of libraries that are already loaded
 * so that the same library is not loaded twice.
 *
 * @throws Exception if library failed to load
 * The Exception includes the loader error message to help debug.
 *
 * @param[in] libname name of library to load
 */
void loadLibrary(const std::string& libname);

/**
 * Factory to dynamically create objects derived from a specific prototype
 * class.
 *
 * This factory is a singleton class meaning it cannot be created by the user.
 *
 * @tparam Prototype the type of object that this factory creates.
 *    This should be the base class that all types in this factory derive from.
 * @tparam PrototypePtr the type of pointer to the object
 *    By defeault, we use 
 *    [`std::unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr)
 *    for good memory management.
 * @tparam PrototypeMakerArgs parameter pack of arguments to pass 
 *    to the object makers and subsequently the constructor.
 *
 * ## Terminology
 *
 * - Factory: An object that has a look-up table between class names and
 *   pointers to functions that can create them
 * - Maker: A function that can create a specific class
 * - Prototype: An abstract base class from which derived classes can be used
 *
 * ## Design
 *
 * The factory itself works in two steps.
 * 1. All of the different derived classes "register" or "declare" themselves
 *    so that the factory knowns how to create them.
 *    This registration is done by providing a "maker" function as well as the
 *    name they should be referred to by.
 * 2. The factory creates any of the registered classes and returns a pointer
 *    to it in the form of a prototype-class pointer.
 *
 * ## Usage
 *
 * Using the factory effecitvely can be done in situations where many classes
 * all follow the same design structure, but have different implementations
 * for specific steps. In order to reflect this "same design structure",
 * we define an abstract base class for all of our derived classes from
 * which to inherit. This abstract base class is our "prototype".
 *
 * Below is a rudimentary example that shows you the basics of this class.
 *
 * ### A Prototype LibraryEntry
 * This `LibraryEntry` prototype class satisfies our requirements.
 * It also defines a helpful "delcaration" macro for derived classes to use.
 * ```cpp
 * // LibraryEntry.hpp
 * #ifndef LIBRARYENTRY_HPP
 * #define LIBRARYENTRY_HPP
 * // we need the factory template
 * #include "Factory/Factory.hpp"
 * 
 * // this class is our prototype
 * class LibraryEntry {
 *  public:
 *   // virtual destructor so we can dynamically create derived classes
 *   virtual ~LibraryEntry() = default;
 *   // pure virtual function that our derived classes will implement
 *   virtual std::string name() = 0;
 *   // the factory type that we will use here
 *   using Factory = ::factory::Factory<LibraryEntry>;
 * };  // LibraryEntry
 * 
 * // a macro to help with registering our library entries with our factory
 * #define DECLARE_LIBRARYENTRY(NS, CLASS)                                \
 *   namespace NS {                                                       \
 *   std::unique_ptr<::LibraryEntry> CLASS##Maker() {                     \
 *     return std::make_unique<CLASS>();                                  \
 *   }                                                                    \
 *   __attribute__((constructor)) static void CLASS##Declare() {          \
 *     ::LibraryEntry::Factory.get().declare(                             \
 *         std::string(#NS) + "::" + std::string(#CLASS), &CLASS##Maker); \
 *   }                                                                    \
 *   }
 * #endif // LIBRARYENTRY_HPP
 * ```
 *
 * ### Example Derived Classes
 * Here are a few example derived classes.

 * ```cpp
 * // Book.cpp
 * #include "LibraryEntry.hpp"
 * namespace library {
 * class Book : public LibraryEntry {
 *  public :
 *   virtual std::string name() final override {
 *     return "Where the Red Fern Grows";
 *   }
 * };
 * }
 * 
 * DECLARE_LIBRARYENTRY(library,Book)
 * ```
 * 
 * ```cpp
 * // Podcast.cpp
 * #include "LibraryEntry.hpp"
 * namespace library {
 * namespace audio {
 * class Podcast : public LibraryEntry {
 *  public :
 *   virtual std::string name() final override {
 *     return "538 Politics Podcast";
 *   }
 * };
 * }
 * }
 * 
 * DECLARE_LIBRARYENTRY(library::audio,Podcast)
 * ```
 * 
 * ```cpp
 * // Album.cpp
 * #include "LibraryEntry.hpp"
 * namespace library {
 * namespace audio {
 * class Album : public LibraryEntry {
 *  public :
 *   virtual std::string name() final override {
 *     return "Kind of Blue";
 *   }
 * };
 * }
 * }
 * 
 * DECLARE_LIBRARYENTRY(library::audio,Album)
 * ```
 *
 * ### Executable
 * Since the `DECLARE_LIBRARYENTRY` macro defines a function that is decorated
 * with a compiler attribute causing the function to be called at library-load
 * time, the registration of our various library entries is automatically done
 * before the execution of `main` (or after if the loadLibrary function is
 * used). For simplicity, let's compile these sources files together with a
 * main defined below.
 *
 * ```cpp
 * // main.cxx
 * #include "LibraryEntry.hpp"
 * 
 * int main(int argc, char* argv[]) {
 *   std::string full_cpp_name{argv[1]}; 
 *   try {
 *     auto entry_ptr{LibraryEntry::Factory::get().make(full_cpp_name)};
 *     std::cout << entry_ptr->name() << std::endl;
 *   } catch (const factory::FactoryException& e) {
 *     std::cerr << "ERROR: " <<  e.what() << std::endl;
 *   }
 * }
 * ```
 * 
 * Compiling these files together into the `fave-things` executable would
 * then lead to the following behavior.
 *
 * ```
 * $ fave-things library::Book
 * Where the Red Fern Grows
 * $ fave-things library::audio::Podcast
 * 538 Politics Podcast
 * $ fave-things library::audio::Album
 * Kind of Blue
 * $ fave-things library::DoesNotExist
 * ERROR: An object named library::DoesNotExist has not been declared.
 * ```
 */
template <typename Prototype,
          typename PrototypePtr = std::unique_ptr<Prototype>,
          typename... PrototypeMakerArgs>
class Factory {
 public:
  /**
   * the signature of a function that can be used by this factory
   * to dynamically create a new object.
   *
   * This is merely here to make the definition of the Factory simpler.
   */
  using PrototypeMaker = PrototypePtr (*)(PrototypeMakerArgs...);

 public:
  /**
   * get the factory instance
   *
   * Using a static function variable gaurantees that the factory
   * is created as soon as it is needed and that it is deleted
   * before the program completes.
   *
   * @returns reference to single Factory instance
   */
  static Factory& get() {
    static Factory the_factory;
    return the_factory;
  }

  /**
   * register a new object to be constructible
   *
   * We insert the new object into the library after
   * checking that it hasn't been defined before.
   *
   * @throws Exception if the object has been declared before.
   * This exception can easily be avoided by making sure the declaration
   * macro for a prototype links the name of the PrototypeMaker function to
   * the name of the derived class. This means the user would have a
   * compile-time error rather than a runtime exception.
   *
   * @param[in] full_name name to use as a reference for the declared object
   * @param[in] maker a pointer to a function that can dynamically create an instance
   */
  void declare(const std::string& full_name, PrototypeMaker maker) {
    auto lib_it{library_.find(full_name)};
    if (lib_it != library_.end()) {
      throw Exception("An object named " + full_name +
                      " has already been declared.");
    }
    library_[full_name] = maker;
  }

  /**
   * make a new object by name
   *
   * We look through the library to find the requested object.
   * If found, we create one and return a pointer to the newly
   * created object. If not found, we raise an exception.
   *
   * @throws Exception if the input object name could not be found
   *
   * The arguments to the maker are determined at compiletime
   * using the template parameters of Factory.
   *
   * @param[in] full_name name of object to create, same name as passed to declare
   * @param[in] maker_args parameter pack of arguments to pass on to maker
   *
   * @returns a pointer to the parent class that the objects derive from.
   */
  PrototypePtr make(const std::string& full_name,
                    PrototypeMakerArgs... maker_args) {
    auto lib_it{library_.find(full_name)};
    if (lib_it == library_.end()) {
      throw Exception("An object named " + full_name +
                       " has not been declared.");
    }
    return lib_it->second(maker_args...);
  }

  /// delete the copy constructor
  Factory(Factory const&) = delete;

  /// delete the assignment operator
  void operator=(Factory const&) = delete;

 private:
  /// private constructor to prevent creation
  Factory() = default;

  /// library of possible objects to create
  std::unordered_map<std::string, PrototypeMaker> library_;
};  // Factory

}  // namespace fire::factory

#endif  // FACTORY_FACTORY_H
