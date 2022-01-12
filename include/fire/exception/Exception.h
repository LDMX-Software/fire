#ifndef FIRE_EXCEPTION_EXCEPTION_H
#define FIRE_EXCEPTION_EXCEPTION_H

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <exception>
#include <string>

namespace fire {

/**
 * Standard base exception class with some useful output information
 */
class Exception : public std::exception {
 public:
  /**
   * Empty constructor.
   *
   * Don't build stack trace for empty exceptions.
   */
  Exception() noexcept {}

  /**
   * Class constructor.
   * @param[in] cat category of this exception
   * @param[in] message message describing the exception.
   * @param[in] build_trace true if we want the stack trace to be built
   */
  Exception(const std::string& cat, const std::string& msg, bool build_trace = true) noexcept;

  /**
   * Class destructor.
   */
  virtual ~Exception() = default;

  /**
   * get the category of this exception
   * @return the category for this exception
   */
  const std::string& category() const noexcept { return category_; }

  /**
   * Get the message of the exception.
   * @return The message of the exception.
   */
  const std::string& message() const noexcept { return message_; }

  /**
   * Get the stack trace
   *
   * The stack trace is only built if the build_trace argument to
   * the constructor was true, so this might return an empty string.
   *
   * @return stack trace in the form of a string
   */
  const std::string& trace() const noexcept { return stack_trace_; }

  /**
   * The error message.
   * @return The error message.
   */
  virtual const char *what() const noexcept override {
    return message_.c_str();
  }

 private:
  /// the category of this exception
  std::string category_;
  /// the error message to print with this exception
  std::string message_;
  /// the stored stack trace at the throw point (if created)
  std::string stack_trace_;
};
}  // namespace fire

#endif
