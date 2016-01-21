/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

//! @file logger.h
//! Provides logging facilities for bmv2 and targets written using bmv2 We
//! currently used the <a href="https://github.com/gabime/spdlog">spdlog</a> as
//! the logging backend so this module is just a wrapper around the spdlog
//! framework.
//!
//! The preferred way to log messages is using the logging macros:
//!   - BMLOG_DEBUG(), or BMLOG_DEBUG_PKT() for messages regarding a specific
//! packet
//!   - BMLOG_TRACE(), or BMLOG_TRACE_PKT() for messages regarding a specific
//! packet

#ifndef BM_SIM_INCLUDE_BM_SIM_LOGGER_H_
#define BM_SIM_INCLUDE_BM_SIM_LOGGER_H_

#ifdef BMLOG_DEBUG_ON
#define SPDLOG_DEBUG_ON
#endif

#ifdef BMLOG_TRACE_ON
#define SPDLOG_TRACE_ON
#endif

#include <string>

#include "spdlog/spdlog.h"

namespace bm {

//! Provides logging for bmv2.
//!
//! Log messages follow the following pattern:
//! `<time> bmv2 <msg level> <thread id> <MSG>`
//!
//! Usage:
//! @code
//! Logger::get()->info("Hi");
//! Logger::get()->debug("This is bmv2 amd my name is {}", name_str);
//! Logger::get()->error("An error? never!");
//! @endcode
//! However, the preferred way to log messages is using the logging macros:
//!   - BMLOG_DEBUG(), or BMLOG_DEBUG_PKT() for messages regarding a specific
//! packet
//!   - BMLOG_TRACE(), or BMLOG_TRACE_PKT() for messages regarding a specific
//! packet
class Logger {
 public:
  //! Different log levels
  enum class LogLevel {
    TRACE, DEBUG, INFO, NOTICE, WARN, ERROR, CRITICAL, ALERT, EMERG, OFF
  };

 public:
  //! Get an instance of the logger. It actually returns a pointer to a spdlog
  //! logger instance.
  //! Usage:
  //! @code
  //! Logger::get()->info("Hi");
  //! Logger::get()->debug("This is bmv2 amd my name is {}", name_str);
  //! Logger::get()->error("An error? never!");
  //! @endcode
  //!
  //! See the <a href="https://github.com/gabime/spdlog">spdlog</a>
  //! documentation for more advanced usage.
  static spdlog::logger *get() {
    static spdlog::logger *logger_ = init_logger();
    (void) logger_;
    return logger;
  }

  //! Set the log level. Messages with a lesser level will not be logged.
  static void set_log_level(LogLevel level);

  //! Log all messages to stdout.
  static void set_logger_console();

  //! Log all messages to the given file.
  static void set_logger_file(const std::string &filename);

 private:
  static spdlog::logger *init_logger();

  static spdlog::level::level_enum to_spd_level(LogLevel level);

  static void set_pattern();

  static void unset_logger();

 private:
  static spdlog::logger *logger;
};

}  // namespace bm

//! Preferred way (because can be disabled at compile time) to log a debug
//! message. Is enabled by preprocessor BMLOG_DEBUG_ON.
#define BMLOG_DEBUG(...) SPDLOG_DEBUG(bm::Logger::get(), __VA_ARGS__)
//! Preferred way (because can be disabled at compile time) to log a trace
//! message. Is enabled by preprocessor BMLOG_TRACE_ON.
#define BMLOG_TRACE(...) SPDLOG_TRACE(bm::Logger::get(), __VA_ARGS__)

//! Same as for BMLOG_DEBUG but for messages regarding a specific packet. Will
//! automatically print the packet id and packet context, along with your
//! message.
#define BMLOG_DEBUG_PKT(pkt, s, ...) \
  SPDLOG_DEBUG(bm::Logger::get(), "[{}] [cxt {}] " s, (pkt).get_unique_id(), \
               (pkt).get_context(), ##__VA_ARGS__)
//! Same as for BMLOG_TRACE but for messages regarding a specific packet. Will
//! automatically print the packet id and packet context, along with your
//! message.
#define BMLOG_TRACE_PKT(pkt, s, ...) \
  SPDLOG_TRACE(bm::Logger::get(), "[{}] [cxt {}] " s, (pkt).get_unique_id(), \
               (pkt).get_context(), ##__VA_ARGS__)

#undef SPDLOG_DEBUG_ON
#undef SPDLOG_TRACE_ON

#endif  // BM_SIM_INCLUDE_BM_SIM_LOGGER_H_
