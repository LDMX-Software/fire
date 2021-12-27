#ifndef FIRE_EXCEPTION_EXCEPTION_H
#define FIRE_EXCEPTION_EXCEPTION_H

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <exception>
#include <string>

namespace fire::exception {

/**
 * @class Exception
 * @brief Standard base exception class with some useful output information
 *
 * @note It is strongly recommended to use the EXCEPTION_RAISE macro to throw
 * exceptions.
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
   * @param message message describing the exception.
   */
  Exception(const std::string &msg) noexcept : message_{msg} {}

  /**
   * Class destructor.
   */
  virtual ~Exception() = default;

  /**
   * Get the message of the exception.
   * @return The message of the exception.
   */
  const std::string &message() const noexcept { return message_; }

  /**
   * The error message.
   * @return The error message.
   */
  virtual const char *what() const noexcept override {
    return message_.c_str();
  }

  /**
   * Get the full stack trace
   */
  std::string buildStackTrace() const;

 private:
  /** Error message. */
  std::string message_;
};
}  // namespace fire::exception

/**
 * @macro ENABLE_EXCEPTIONS
 * This macro defines a simple derived class
 * within the current namespace or class
 *
 * If the user puts this macro inside a namespace
 * or 'public' part of a class, then the class
 * Exception is available to be thrown within that
 * namespace or class.
 */
#define ENABLE_EXCEPTIONS()                               \
  class Exception : public ::fire::exception::Exception { \
   public:                                                \
    Exception(const std::string &msg) noexcept            \
        : ::fire::exception::Exception(msg) {}            \
  }

#endif
