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

#include <bm/config.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/P4Objects.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <cassert>

#include "version.h"

namespace fs = boost::filesystem;

namespace bm {

struct interface {
  interface(const std::string &name, uint32_t port)
    : name(name), port(port) { }

  std::string name{};
  uint32_t port{};
};

void validate(boost::any& v,  // NOLINT(runtime/references)
              const std::vector<std::string> &values,
              interface* /* target_type */,
              int) {
  namespace po = boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  po::validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  const std::string &s = po::validators::get_single_string(values);

  std::istringstream stream(s);
  std::string tok;
  std::getline(stream, tok, '@');
  uint32_t port;
  try {
    port = std::stoi(tok, nullptr);
  }
  catch (...) {
    throw po::validation_error(po::validation_error::invalid_option_value,
                               "interface");
  }
  if (tok == s) {
    throw po::validation_error(po::validation_error::invalid_option_value,
                               "interface");
  }
  std::getline(stream, tok);
  v = boost::any(interface(tok, port));
}

/* static */
constexpr int OptionsParser::default_max_port_count;

void
OptionsParser::parse(int argc, char *argv[], TargetParserIface *tp,
                     // NOLINTNEXTLINE(runtime/references)
                     std::ostream &outstream) {
  namespace po = boost::program_options;

  po::options_description description("Options");

  description.add_options()
      ("help,h", "Display this help message")
      ("interface,i", po::value<std::vector<interface> >()->composing(),
       "<port-num>@<interface-name>: "
       "Attach network interface <interface-name> as port <port-num> at "
       "startup. Can appear multiple times")
      ("pcap", po::value<std::string>()->default_value("")->implicit_value("."),
       "Generate pcap files for interfaces. "
       "Argument is optional and is the directory where pcap files should be "
       "written. If omitted, files will be written in current directory.")
      ("use-files", po::value<int>(), "Read/write packets from files "
       "(interface X corresponds to two files X_in.pcap and X_out.pcap). "
       "Argument is the time to wait (in seconds) before starting to process "
       "the packet files.")
#ifdef BM_NANOMSG_ON
      ("packet-in", po::value<std::string>(),
       "Enable receiving packet on this (nanomsg) socket. "
       "The --interface options will be ignored.")
#endif
#ifdef BM_THRIFT_ON
      ("thrift-port", po::value<int>(),
       "TCP port on which to run the Thrift runtime server")
#endif
      ("device-id", po::value<device_id_t>(),
       "Device ID, used to identify the device in IPC messages (default 0)")
      ("nanolog", po::value<std::string>(),
       "IPC socket to use for nanomsg pub/sub logs "
       "(default: no nanomsg logging")
      ("log-console",
       "Enable logging on stdout")
      ("log-file", po::value<std::string>(),
       "Enable logging to given file")
      ("log-level,L", po::value<std::string>(),
       "Set log level, supported values are "
       "'trace', 'debug', 'info', 'warn', 'error', off'; default is 'trace'")
      ("log-flush", "If used with '--log-file', the logger will flush to disk "
       "after every log message")
#ifdef BM_NANOMSG_ON
      ("notifications-addr", po::value<std::string>(),
       "Specify the nanomsg address to use for notifications "
       "(e.g. learning, ageing, ...); "
       "default is ipc:///tmp/bmv2-<device-id>-notifications.ipc")
#endif
#ifdef BM_DEBUG_ON
      ("debugger", "Activate debugger")
      ("debugger-addr", po::value<std::string>(),
       "Specify the nanomsg address to use for debugger communication; "
       "there is no need to use --debugger in addition to this option; "
       "default is ipc:///tmp/bmv2-<device-id>-debug.ipc")
#endif
      ("restore-state", po::value<std::string>(),
       "Restore state from file")
      ("dump-packet-data", po::value<size_t>(),
       "Specify how many bytes of packet data to dump upon receiving & sending "
       "a packet. We use the logger to dump the packet data, with log level "
       "'info', so make sure the log level you have set does not exclude "
       "'info' messages; default is 0, which means that nothing is logged.")
      ("version,v", "Display version information")
      ("json-version", "Display max bmv2 JSON version supported in the format "
       "<major>.<minor>; all bmv2 JSON versions with the same <major> version "
       "number are also supported.")
      ("no-p4", "Enable the switch to start without an inout configuration")
      ("max-port-count",
       po::value<int>()->default_value(default_max_port_count),
       "Maximum number of interfaces that can be bound to the switch; "
       "this is not an upper bound on each port number, which can be arbitrary."
       " Depending on the target, this max value may or may not be enforced.")
      ;  // NOLINT(whitespace/semicolon)

  po::options_description hidden;
  hidden.add_options()
      ("input-config", po::value<std::string>(), "input config")
      ("to-pass-further", po::value<std::vector<std::string> >(), "");

  po::options_description options;
  options.add(description).add(hidden);

  po::positional_options_description positional;
  positional.add("input-config", 1);
  positional.add("to-pass-further", -1);

  options_provided.clear();

  po::variables_map vm;
  auto parser = po::command_line_parser(argc, argv);
  parser.options(options).positional(positional);
  std::vector<std::string> to_pass_further = {};
  try {
    po::parsed_options parsed = parser.run();
    po::store(parsed, vm);
    po::notify(vm);
    if (tp && vm.count("to-pass-further")) {
      to_pass_further = vm["to-pass-further"].as<std::vector<std::string> >();
    }
  }
  catch(...) {
    outstream << "Error while parsing command line arguments\n";
    outstream << "Usage: SWITCH_NAME [options] <path to JSON config file>\n";
    outstream << description;
    if (tp) {
      outstream << "Your target comes with its own command line parser, "
                << "make sure you separate general bmv2 options from, "
                << "target specific options with '--'\n";
    }
    exit(1);
  }

  for (const auto &p : vm) options_provided.insert(p.first);

  if (vm.count("help")) {
    outstream << "Usage: SWITCH_NAME [options] <path to JSON config file>\n";
    outstream << description;
    if (tp) {
      outstream << "This target also comes with its own command line parser, "
                << "make sure you separate general bmv2 options from, "
                << "target specific options with '--'\n"
                << "Target specific options:\n";
      tp->help_msg(&outstream);
    }
    exit(0);
  }

  if (vm.count("version")) {
    std::string version_str(bm_version_str);
    outstream << version_str << "\n";
    exit(0);
  }

  if (vm.count("json-version")) {
    outstream << P4Objects::get_json_version_string() << "\n";
    exit(0);
  }

  no_p4 = vm.count("no-p4");
  if (!no_p4 && !vm.count("input-config")) {
    outstream << "Error: please specify an input JSON configuration file\n";
    outstream << "Usage: SWITCH_NAME [options] <path to JSON config file>\n";
    outstream << description;
    exit(1);
  }
  // this is a little hacky because we are mixing positional arguments; ideally
  // the input config would be just a regular option
  if (no_p4 && vm.count("input-config")) {
    auto path = vm["input-config"].as<std::string>();
    auto is_pos = (path.size() > 2 && path.substr(0, 2) != "--");
    auto dot = path.find_last_of(".");
    auto has_json_extension =
        (dot != std::string::npos && path.substr(dot) == ".json");
    if (tp && (!is_pos || !has_json_extension)) {
      to_pass_further.insert(to_pass_further.begin(), path);
    } else {
      outstream << "Warning: ignoring input config as '--no-p4' was used\n";
    }
  }
  if (!no_p4 && vm.count("input-config"))
    config_file_path = vm["input-config"].as<std::string>();

  device_id = 0;
  if (vm.count("device-id")) {
    device_id = vm["device-id"].as<device_id_t>();
  }

#ifdef BM_NANOMSG_ON
  if (vm.count("notifications-addr")) {
    notifications_addr = vm["notifications-addr"].as<std::string>();
  } else {
    notifications_addr = std::string("ipc:///tmp/bmv2-")
        + std::to_string(device_id) + std::string("-notifications.ipc");
  }
#endif

  if (vm.count("nanolog")) {
#ifndef BM_ELOG_ON
    outstream << "Warning: you requested the nanomsg event logger, but bmv2 "
              << "was compiled without -DBMELOG, and the event logger cannot "
              << "be activated\n";
#else
    event_logger_addr = vm["nanolog"].as<std::string>();
    auto event_transport = TransportIface::make_nanomsg(event_logger_addr);
    event_transport->open();
    EventLogger::init(std::move(event_transport), device_id);
#endif
  }

  if (vm.count("log-console") && vm.count("log-file")) {
    outstream << "Error: --log-console and --log-file are exclusive\n";
    exit(1);
  }

  if (vm.count("log-console")) {
    console_logging = true;
  }

  if (vm.count("log-file")) {
    file_logger = vm["log-file"].as<std::string>();
  }

  if (vm.count("log-level")) {
    const std::string log_level_str = vm["log-level"].as<std::string>();
    std::unordered_map<std::string, Logger::LogLevel> levels_map = {
      {"trace", Logger::LogLevel::TRACE},
      {"debug", Logger::LogLevel::DEBUG},
      {"info", Logger::LogLevel::INFO},
      {"warn", Logger::LogLevel::WARN},
      {"error", Logger::LogLevel::ERROR},
      {"off", Logger::LogLevel::OFF} };
    if (!levels_map.count(log_level_str)) {
      outstream << "Invalid value " << log_level_str << " for --log-level\n"
                << "Run with -h to see possible values\n";
      exit(1);
    }
    log_level = levels_map[log_level_str];
  }

  auto log_requested = console_logging || !file_logger.empty();
  auto missing_macros = false;
#ifndef BM_LOG_TRACE_ON
  missing_macros |= log_level <= Logger::LogLevel::TRACE;
#endif
#ifndef BM_LOG_DEBUG_ON
  missing_macros |= log_level <= Logger::LogLevel::DEBUG;
#endif
  if (log_requested && missing_macros) {
    outstream << "You disabled logging macros when building this binary; "
              << "however, the log level is currenty set to include 'debug' "
              << "and possibly 'trace' messages; therefore the logs will be "
              << "missing most messages\n";
  }

  if (vm.count("log-flush")) {
    if (!vm.count("log-file")) {
      outstream << "Ignoring --log-flush option because --log-file "
                << "not specified\n";
    } else {
      log_flush = true;
    }
  }

  if (vm.count("dump-packet-data")) {
    dump_packet_data = vm["dump-packet-data"].as<size_t>();
    if (dump_packet_data > 0 && log_level > Logger::LogLevel::INFO) {
      outstream << "You asked for some packet data to be dumped for each "
                << "packet on ingress and egress, but you set a log level "
                << "which excludes 'info' messages. Therefore, "
                << "'--dump-packet-data' will be ignored.\n";
    }
  }

  if (vm.count("interface")) {
    for (const auto &iface : vm["interface"].as<std::vector<interface> >()) {
      ifaces.add(iface.port, iface.name);
    }
  }

  pcap_dir = vm["pcap"].as<std::string>();
  if (!pcap_dir.empty()) {
    pcap = true;
    if (!fs::is_directory(fs::path(pcap_dir))) {
      outstream << "Directory '" << pcap_dir << "' specified with '--pcap' "
                << "does not exist.\n";
      exit(1);
    }
  }

  if (vm.count("use-files")) {
    use_files = true;
    wait_time = vm["use-files"].as<int>();
    if (wait_time < 0)
      wait_time = 0;
  }

#ifdef BM_NANOMSG_ON
  if (vm.count("packet-in")) {
    packet_in = true;
    packet_in_addr = vm["packet-in"].as<std::string>();
    // very important to clear interface list
    ifaces.clear();
  }
#endif

  if (use_files && packet_in) {
    outstream << "Error: --use-files and --packet-in are exclusive\n";
    exit(1);
  }

  if (vm.count("debugger-addr")) {
    debugger = true;
    debugger_addr = vm["debugger-addr"].as<std::string>();
  } else if (vm.count("debugger")) {
    debugger = true;
    debugger_addr = std::string("ipc:///tmp/bmv2-")
        + std::to_string(device_id) + std::string("-debug.ipc");
  }

#ifdef BM_THRIFT_ON
  int default_thrift_port = 9090;
  if (vm.count("thrift-port")) {
    thrift_port = vm["thrift-port"].as<int>();
  } else {
    thrift_port = default_thrift_port;
  }
#endif

  max_port_count = vm["max-port-count"].as<int>();
  if (max_port_count < 0) {
    outstream << "Invalid --max-port-count argument, cannot be negative\n";
    exit(1);
  }
  if (max_port_count == 0) {
    outstream << "You provided a --max-port-count value of 0, "
              << "that does not seem quite right\n";
  }

  if (vm.count("restore-state")) {
    state_file_path = vm["restore-state"].as<std::string>();
  }

  if (tp) {
    outstream << "Calling target program-options parser\n";
    if (tp->parse(to_pass_further, &outstream)) {
      outstream << "Target parser returned an error\n";
    }
  }
}

void
OptionsParser::parse(int argc, char *argv[], TargetParserIface *tp) {
  parse(argc, argv, tp, std::cout);
}

bool
OptionsParser::option_was_provided(const std::string &option) const {
  return options_provided.find(option) != options_provided.end();
}

}  // namespace bm
