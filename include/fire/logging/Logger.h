/**
 * @file Logger.h
 * @brief Interface to Boost.Logging
 */

#ifndef FIRE_LOGGING_LOGGER_H
#define FIRE_LOGGING_LOGGER_H

/**
 * Necessary to get linking to work?
 *
 * ## References
 * - [Stackoverflow Question](https://stackoverflow.com/questions/23137637/linker-error-while-linking-boost-log-tutorial-undefined-references)
 * - [Boost Docs](https://www.boost.org/doc/libs/1_54_0/libs/log/doc/html/log/rationale/namespace_mangling.html)
 * - [Another Stackoverflow](https://stackoverflow.com/a/40016057)
 */
#define BOOST_ALL_DYN_LINK 1

#include <boost/log/core.hpp>                 //core logging service
#include <boost/log/expressions.hpp>          //for attributes and expressions
#include <boost/log/sinks/sync_frontend.hpp>  //syncronous sink frontend
#include <boost/log/sinks/text_ostream_backend.hpp>  //output stream sink backend
#include <boost/log/sources/global_logger_storage.hpp>  //for global logger default
#include <boost/log/sources/severity_channel_logger.hpp>  //for the severity logger
#include <boost/log/sources/severity_feature.hpp>  //for the severity feature in a logger
#include <boost/log/utility/setup/common_attributes.hpp>

// TODO check which headers are required
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>

/**
 * Housing for logging infrastructure
 *
 * This namespace holds the functions that allow fire to interact
 * with boost's central logging framework in a helpful way.
 */
namespace fire::logging {

/**
 * Different logging levels available to fire's log
 *
 * Boost calls these "severity" levels becuase the 
 * higher numbers are assigned to "more sever" logging messages.
 */
enum level {
  /// 0
  debug = 0,
  /// 1
  info,
  /// 2
  warn,
  /// 3
  error,
  /// 5
  fatal
};

/**
 * Convert an integer to the severity level enum
 *
 * Any integer below zero will be set to 0 (debug),
 * and any integer above four will be set to 4 (fatal).
 *
 * @param[in] iLvl integer level to be converted
 * @return converted enum level
 */
level convertLevel(int iLvl);

/// Short names for boost::log
namespace log = boost::log;

/// short name for boost::log::sinks
namespace sinks = boost::log::sinks;

/**
 * Define the type of logger we will be using in fire
 *
 * We choose a logger that allows us to define a severity/logging level,
 * has different "channels" for the log, and is multi-thread capable.
 */
using logger = log::sources::severity_channel_logger_mt<level, std::string>;

/**
 * Gets a logger for the user
 *
 * Returns a logger type with some extra initialization procedures done.
 * Should _only be called ONCE_ during a run.
 *
 * @note Use the ENABLE_LOGGING macro in your class declaration instead
 * of this function directly.
 *
 * @param name name of this logging channel (e.g. processor name)
 * @return logger with the input channel name
 */
logger makeLogger(const std::string& name);

/**
 * Initialize the logging backend
 *
 * This function setups up the terminal and file sinks.
 * Sets their format and filtering level for this run.
 *
 * @note Will not setup printing log messages to file if fileName is empty
 * string.
 *
 * @param termLevel minimum level to print to terminal (everything above it is
 * also printed)
 * @param fileLevel minimum level to print to file log (everything above it is
 * also printed)
 * @param fileName name of file to print log to
 */
void open(const level termLevel, const level fileLevel,
          const std::string& fileName);

/**
 * Close up the logging
 */
void close();

}  // namespace fire::logging

/**
 * Enables logging in a class.
 *
 * Should be put in the `private` section of the class
 * and before the closing bracket `};`
 *
 * Defines the member variable `theLog_` with the input
 * name as the channel name.
 *
 * Makes `theLog_` mutable so that the log can be used
 * in any class function.
 *
 * @param[in] name Name of logging channel
 */
#define ENABLE_LOGGING(name) \
  mutable logging::logger theLog_{logging::makeLogger(#name)};

/**
 * Log a message at the input level through fire
 *
 * Assumes to have access to a variable named `theLog_` of type 
 * fire::logging::logger.
 *
 * @param[in] lvl logging level without namespace or enum (e.g. `info`)
 */
#define fire_log(lvl) BOOST_LOG_SEV(theLog_, fire::logging::level::lvl)

#endif  // FRAMEWORK_LOGGER_H
