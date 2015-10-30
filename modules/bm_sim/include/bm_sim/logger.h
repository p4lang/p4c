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

#ifndef _BM_LOGGER_H_
#define _BM_LOGGER_H_

#ifdef BMLOG_DEBUG_ON
#define SPDLOG_DEBUG_ON
#endif

#ifdef BMLOG_TRACE_ON
#define SPDLOG_TRACE_ON
#endif

#include <string>

#include "spdlog/spdlog.h"

class Logger {
public:
  enum class LogLevel {
    TRACE, DEBUG, INFO, NOTICE, WARN, ERROR, CRITICAL, ALERT, EMERG, OFF
  };

public:
  static spdlog::logger *get() {
    static spdlog::logger *logger_ = init_logger();
    (void) logger_;
    return logger;
  }

  static void set_log_level(LogLevel level);

  static void set_logger_console();

  static void set_logger_file(const std::string &filename);

private:
  static spdlog::logger *init_logger();

  static spdlog::level::level_enum to_spd_level(LogLevel level);

  static void set_pattern();

  static void unset_logger();

private:
  static spdlog::logger *logger;
};

#define BMLOG_DEBUG(...) SPDLOG_DEBUG(Logger::get(), __VA_ARGS__)
#define BMLOG_TRACE(...) SPDLOG_TRACE(Logger::get(), __VA_ARGS__)

#define BMLOG_DEBUG_PKT(pkt, s, ...) \
  SPDLOG_DEBUG(Logger::get(), "[{}] " s, (pkt).get_unique_id(), __VA_ARGS__)
#define BMLOG_TRACE_PKT(pkt, s, ...) \
  SPDLOG_TRACE(Logger::get(), "[{}] " s, (pkt).get_unique_id(), __VA_ARGS__)

#undef SPDLOG_DEBUG_ON
#undef SPDLOG_TRACE_ON

#endif
