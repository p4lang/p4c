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

#ifndef BM_BM_SIM_LOGGER_H_
#define BM_BM_SIM_LOGGER_H_

#include <bm/spdlog/spdlog.h>

#include <string>

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

  //! Log all messages to the given file. If \p force_flush is true, the logger
  //! will flush to disk after every log message.
  static void set_logger_file(const std::string &filename,
                              bool force_flush = false);

 private:
  static spdlog::logger *init_logger();

  static spdlog::level::level_enum to_spd_level(LogLevel level);

  static void set_pattern();

  static void unset_logger();

 private:
  static spdlog::logger *logger;
};

}  // namespace bm

#ifdef BMLOG_DEBUG_ON
//! Preferred way (because can be disabled at compile time) to log a debug
//! message. Is enabled by preprocessor BMLOG_DEBUG_ON.
#define BMLOG_DEBUG(...) bm::Logger::get()->debug(__VA_ARGS__);
#else
#define BMLOG_DEBUG(...)
#endif

#ifdef BMLOG_TRACE_ON
//! Preferred way (because can be disabled at compile time) to log a trace
//! message. Is enabled by preprocessor BMLOG_TRACE_ON.
#define BMLOG_TRACE(...) bm::Logger::get()->trace(__VA_ARGS__);
#else
#define BMLOG_TRACE(...)
#endif

//! Same as for BMLOG_DEBUG but for messages regarding a specific packet. Will
//! automatically print the packet id and packet context, along with your
//! message.
#define BMLOG_DEBUG_PKT(pkt, s, ...)                     \
  BMLOG_DEBUG("[{}] [cxt {}] " s, (pkt).get_unique_id(), \
              (pkt).get_context(), ##__VA_ARGS__)
//! Same as for BMLOG_TRACE but for messages regarding a specific packet. Will
//! automatically print the packet id and packet context, along with your
//! message.
#define BMLOG_TRACE_PKT(pkt, s, ...)                     \
  BMLOG_TRACE("[{}] [cxt {}] " s, (pkt).get_unique_id(), \
              (pkt).get_context(), ##__VA_ARGS__)

#define BMLOG_ERROR(...) bm::Logger::get()->error(__VA_ARGS__)

#define BMLOG_ERROR_PKT(pkt, s, ...)                                    \
  bm::Logger::get()->error("[{}] [cxt {}] " s, (pkt).get_unique_id(),   \
                           (pkt).get_context(), ##__VA_ARGS__)

#endif  // BM_BM_SIM_LOGGER_H_
