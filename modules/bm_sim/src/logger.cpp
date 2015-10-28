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

#include <memory>

#include "bm_sim/logger.h"

#include "spdlog/sinks/null_sink.h"

spdlog::logger *Logger::logger = nullptr;

void
Logger::set_logger_console() {
  unset_logger();
  auto logger_ = spdlog::stdout_logger_mt("bmv2");
  logger = logger_.get();
  set_pattern();
  logger_->set_level(to_spd_level(LogLevel::DEBUG));
}

void
Logger::set_logger_file(const std::string &filename) {
  unset_logger();
  auto logger_ = spdlog::rotating_logger_mt("bmv2", filename,
					    1024 * 1024 * 5, 3);
  logger = logger_.get();
  set_pattern();
  logger_->set_level(to_spd_level(LogLevel::DEBUG));
}

void
Logger::set_pattern() {
  logger->set_pattern("[%H:%M:%S.%e] [%n] [%L] [thread %t] %v");
}

void
Logger::unset_logger() {
  spdlog::drop("bmv2");
}

spdlog::logger *
Logger::init_logger() {
  if(logger != nullptr) return logger;
  auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  auto null_logger = std::make_shared<spdlog::logger>("bmv2", null_sink);
  spdlog::register_logger(null_logger);
  logger = null_logger.get();
  return logger;
}

void
Logger::set_log_level(LogLevel level) {
  spdlog::logger *logger = get();
  logger->set_level(to_spd_level(level));
}

spdlog::level::level_enum
Logger::to_spd_level(LogLevel level) {
  using namespace spdlog::level;
  switch(level) {
  case LogLevel::TRACE: return trace;
  case LogLevel::DEBUG: return debug;
  case LogLevel::INFO: return info;
  case LogLevel::NOTICE: return notice;
  case LogLevel::WARN: return warn;
  case LogLevel::ERROR: return err;
  case LogLevel::CRITICAL: return critical;
  case LogLevel::ALERT: return alert;
  case LogLevel::EMERG: return emerg;
  case LogLevel::OFF: return off;
  default: return off;
  }
}
