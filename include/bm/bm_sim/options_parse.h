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

/* TODO(antonin): does this code really belong in this module */

#ifndef BM_BM_SIM_OPTIONS_PARSE_H_
#define BM_BM_SIM_OPTIONS_PARSE_H_

#include <string>
#include <map>

#include "logger.h"
#include "target_parser.h"

namespace bm {

class InterfaceList {
 public:
  using iterator = std::map<int, std::string>::iterator;
  using const_iterator = std::map<int, std::string>::const_iterator;

 public:
  void add(int port, const std::string &iface) {
    ifaces[port] = iface;
  }
  bool empty() { return ifaces.empty(); }
  void clear() { ifaces.clear(); }
  // iterators
  iterator begin() { return ifaces.begin(); }
  const_iterator begin() const { return ifaces.begin(); }
  iterator end() { return ifaces.end(); }
  const_iterator end() const { return ifaces.end(); }

 private:
  std::map<int, std::string> ifaces{};
};

class OptionsParser {
  friend class SwitchWContexts;

 public:
  void parse(int argc, char *argv[], TargetParserIface *tp);

 private:
  std::string config_file_path{};
  bool no_p4{false};
  InterfaceList ifaces{};
  bool pcap{false};
  int thrift_port{0};
  int device_id{};
  // if true read/write packets from files instead of interfaces
  bool use_files{false};
  // time to wait (in seconds) before starting packet processing
  int wait_time{0};
  // if true read/write packets from nanomsg socket instead of interfaces
  bool packet_in{false};
  std::string packet_in_addr{};
  std::string event_logger_addr{};
  std::string file_logger{};
  bool console_logging{false};
  // by default everything is logged
  Logger::LogLevel log_level{Logger::LogLevel::TRACE};
  // by default file logs are not "force-flushed" to disk
  bool log_flush{false};
  std::string notifications_addr{};
  bool debugger{false};
  std::string debugger_addr{};
  std::string state_file_path{};
  size_t dump_packet_data{0};
};

}  // namespace bm

#endif  // BM_BM_SIM_OPTIONS_PARSE_H_
