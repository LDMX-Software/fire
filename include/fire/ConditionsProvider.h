#ifndef FIRE_CONDITIONSPROVIDER_H
#define FIRE_CONDITIONSPROVIDER_H

#include <map>

#include "fire/ConditionsIntervalOfValidity.h"
#include "fire/ConditionsObject.h"
#include "fire/RunHeader.h"
#include "fire/config/Parameters.h"
#include "fire/exception/Exception.h"
#include "fire/factory/Factory.h"
#include "fire/logging/Logger.h"

namespace fire {

/// forward declaration for attachment
class Conditions;
/// forward declaration for legacy make
class Process;

/**
 * Base class for all providers of conditions objects
 *
 * Besides defining the necessary virtual callbacks,
 * we also provide the factory infrastructure for dynamically
 * creating providers from loaded libraries and we define
 * a requestParentCondition function that can be used by
 * other providers to obtain conditions other conditions
 * depend on.
 *
 * ## Usage
 * Defining a conditions provider is done similarly to a Processor.
 * Since ConditionsProviders are generally only used after some
 * experience with Processors is obtained, I will not go into as much
 * detail. Perhaps the best way is to provide an example from which
 * you can learn.
 * ```cpp
 * // MyConditionsProvider.cpp
 * #include <fire/ConditionsProvider.h>
 * class MyConditionsProvider : public fire::ConditionsProvider {
 *  public:
 *   MyConditionsProvider(const fire::config::Parameters& p)
 *     : fire::ConditionsProvider(p) {
 *     // deduce configuration of provider from parameter set p
 *   }
 *   ~MyConditionsProvider() = default;
 *   virtual std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
 *   getCondition(const EventHeader& context) final override {
 *     // use context to determine correct configuration of conditions object
 *     // as well as the range of run numbers and type of data that the
 *     // condition is valid for
 *   }
 * };
 * DECLARE_CONDITIONS_PROVIDER(MyConditionsProvider);
 * ```
 *
 * One special (but somewhat common) use case is a global condition
 * that is light in memory. These criteria are satisifed by the
 * RandomNumberSeedService and allow the conditions provider and
 * the conditions object to be the same object.
 * ```cpp
 * // LightAndGlobalCondition.cpp
 * #include <fire/ConditionsProvider.h>
 * class LightAndGlobalCondition : public fire::ConditionsProvider, 
 *                                        fire::ConditionsObject {
 *  public:
 *   LightAndGlobalCondition(const fire::config::Parameters& p)
 *     : fire::ConditionsProvider(p) {
 *     // deduce configuration of both provider and object from parameter set p
 *   }
 *   ~LightAndGlobalCondition() = default;
 *   // return ourselves and infinite validity
 *   virtual std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
 *   getCondition(const EventHeader& context) final override {
 *     return std::make_pair(this, ConditionsIntervalOfValidity(true,true)); 
 *   }
 *   // don't allow conditions system to delete us
 *   virtual void release(const ConditionsObject* co) final override {}
 * };
 * DECLARE_CONDITIONS_PROVIDER(LightAndGlobalCondition);
 * ```
 * One could also image a light condition which may still change with
 * the context, but that can be implemented from this example.
 */
class ConditionsProvider {
 public:
  /**
   * Configure the registered provider
   * @param[in] ps Parameters to configure the provider
   */
  ConditionsProvider(const fire::config::Parameters& ps);

  /**
   * default destructor, virtual so derived types can be destructed
   */
  virtual ~ConditionsProvider() = default;

  /**
   * Pure virtual getCondition function.
   *
   * Must be implemented by any Conditions providers.
   *
   * @param[in] context EventHeader for the condition
   * @return pair of condition and its interval of validity
   */
  virtual std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
  getCondition(const EventHeader& context) = 0;

  /**
   * Called by conditions system when done with a conditions object, 
   * appropriate point for cleanup.
   *
   * @note Default behavior is to delete the object!
   *
   * @param[in] co condition to cleanup
   */
  virtual void release(const ConditionsObject* co) { delete co; }

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events starts.
   */
  virtual void onProcessStart() {}

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events finishes, 
   * such as closing database connections.
   */
  virtual void onProcessEnd() {}

  /**
   * Callback for the ConditionsProvider to take any necessary
   * action when the processing of events starts for a given run.
   */
  virtual void onNewRun(RunHeader&) {}

  /**
   * Get the condition object available from this provider.
   */
  const std::string& getConditionObjectName() const { return objectName_; }

  /**
   * Access the tag name
   */
  const std::string& getTagName() const { return tagname_; }

  /**
   * Attach the central conditions system to this provider
   */
  virtual void attach(Conditions* c) final { conditions_ = c; }

 protected:
  /** 
   * Request another condition needed to construct this condition
   *
   * This is where we use the handle to the central conditions system
   * and this allows us to recursively depend on other instances 
   * ConditionsProvider.
   *
   * @param[in] name condition name that is needed
   * @param[in] context EventHeader for which to get condition
   * @return pair of parent condition and its interval of validity
   */
  std::pair<const ConditionsObject*, ConditionsIntervalOfValidity>
  requestParentCondition(const std::string& name, const EventHeader& context);

  /// The logger for this ConditionsProvider
  mutable logging::logger theLog_;

 private:
  /** Handle to the central conditions system. */
  Conditions* conditions_{nullptr};

  /** The name of the object provided by this provider. */
  std::string objectName_;

  /** The tag name for the ConditionsProvider. */
  std::string tagname_;
 
 public:
  /**
   * The special factory used to create condition providers
   *
   * we need a special factory because it needs to be able to create providers
   * with two different construtor options
   *
   * When "old-style" providers can be abandoned, this redundant code can
   * be removed in favor of the templated factory:
   * ```cpp 
   * using Factory = factory::Factory<ConditionsProvider, 
   *   std::shared_ptr<ConditionsProvider>, 
   *   const config::Parameters &>;
   * ```
   * and then adding the following line to the loop constructing Processors
   * in the Process constructor:
   * ```cpp 
   * auto cp{ConditionsProvider::Factory::get().make(
   *     provider.get<std::string>("class_name"), provider)};
   * std::string provides{cp->getConditionObjectName()};
   * if (providers_.find(provides) != providers_.end()) {
   *   throw Exception("Config",
   *       "Multiple ConditonsProviders configured to provide " +
   *       provides,false);
   * }
   * cp->attach(this);
   * providers_[provides] = cp;
   * ```
   */
  class Factory {
   public:
    /**
     * The base pointer for processors
     */
    using PrototypePtr = std::shared_ptr<ConditionsProvider>;

    /**
     * the signature of a function that can be used by this factory
     * to dynamically create a new object.
     *
     * This is merely here to make the definition of the Factory simpler.
     */
    using PrototypeMaker = std::function<PrototypePtr(const config::Parameters&,Process*,Conditions*)>;
  
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
     * @note This uses the demangled name of the input type
     * as the key in our library of objects. Using the demangled
     * name effectively assumes that all of the libraries being
     * loaded were compiled with the same compiler version.
     * We could undo this assumption by having the key be an
     * input into this function.
     *
     * @tparam DerivedType type of processor to declare
     * @return value to define a static variable to force running this function
     *  at library load time. It relates to variables so that it cannot be
     *  optimized away.
     */
    template<typename DerivedType>
    uint64_t declare() {
      std::string full_name{boost::core::demangle(typeid(DerivedType).name())};
      library_[full_name] = &maker<DerivedType>;
      return reinterpret_cast<std::uintptr_t>(&library_);
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
     * @param[in] ps Parameters to configure the Provider
     * @param[in] p handle to current Process
     *
     * @returns a pointer to the parent class that the objects derive from.
     */
    PrototypePtr make(const std::string& full_name,
                      const config::Parameters& ps,
                      Process* p, Conditions* c) {
      auto lib_it{library_.find(full_name)};
      if (lib_it == library_.end()) {
        throw Exception("Factory","A provider of class " + full_name +
                         " has not been declared.",false);
      }
      return lib_it->second(ps,p,c);
    }
  
    /// delete the copy constructor
    Factory(Factory const&) = delete;
  
    /// delete the assignment operator
    void operator=(Factory const&) = delete;
  
   private:
    /**
     * make a DerivedType returning a PrototypePtr
     *
     * We do a constexpr check on which type of processor it is. If it can be constructed
     * from a Parameters alone, then it is a "new" type. Otherewise, it is a "legacy" type
     * as defined in the framework namespace.
     *
     * @tparam DerivedType type of derived object we should create
     * @param[in] parameters config::Parameters to configure Processor
     * @param[in] process handle to current Process
     */
    template <typename DerivedType>
    static PrototypePtr maker(const config::Parameters& parameters, Process* p, Conditions* c) {
      std::shared_ptr<ConditionsProvider> ptr;
      if constexpr (std::is_constructible<DerivedType, const config::Parameters&>::value) {
        // new type
        ptr = std::make_shared<DerivedType>(parameters);
      } else {
        if (not p) {
          throw Exception("BadConf","Passed a null Process to ConditionsProvider maker",false);
        }
        // old type
        ptr = std::make_shared<DerivedType>(parameters.get<std::string>("obj_name"),
                                            parameters.get<std::string>("tag_name"),
                                            parameters, *p);
      }
      ptr->attach(c);
      return ptr;
    }
  
    /// private constructor to prevent creation
    Factory() = default;
  
    /// library of possible objects to create
    std::unordered_map<std::string, PrototypeMaker> library_;
  };  // Factory

};

}  // namespace fire

/**
 * @def DECLARE_CONDITIONS_PROVIDER(CLASS)
 * @param CLASS The name of the class to register including namespaces
 * @brief Macro which allows fire to construct a provider given its
 * name during configuration.
 * @attention Every ConditionsProvider class must call this macro in
 * the associated implementation (.cxx) file.
 *
 * If you are getting a 'redefinition of v' compilation error from
 * this macro, then that means you have more than one ConditionsProvider
 * defined within a single compilation unit. This is not a problem,
 * you just need to expand the macro yourself:
 * ```cpp
 * // in source file
 * //   the names of the variables don't matter as long as they are different
 * namespace {
 * auto v0 = ::fire::ConditionsProvider::Factory.get().declare<One>();
 * auto v1 = ::fire::ConditionsProvider::Factory.get().declare<Two>();
 * }
 * ```
 */
#define DECLARE_CONDITIONS_PROVIDER(CLASS)                            \
  namespace {                                                         \
  auto v = fire::ConditionsProvider::Factory::get().declare<CLASS>(); \
  }

#endif
