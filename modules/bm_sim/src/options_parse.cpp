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

#include <vector>
#include <list>
#include <iostream>
#include <sstream>

#include <cassert>

#include <boost/program_options.hpp>

#include "bm_sim/options_parse.h"
#include "bm_sim/event_logger.h"

struct interface {
  interface(const std::string &name, int port)
    : name(name), port(port) { }

  std::string name{};
  int port{};
};

void validate(boost::any& v, 
              std::vector<std::string> const& values,
              interface* /* target_type */,
              int)
{
  namespace po = boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  po::validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  const std::string &s = po::validators::get_single_string(values);

  std::istringstream stream(s);
  std::string tok;
  std::getline(stream, tok, '@');
  int port;
  try {
    port = std::stoi(tok, nullptr);
  }
  catch (...) {
    throw po::validation_error(po::validation_error::invalid_option_value,
			       "interface");
  }
  if(tok == s) {
    throw po::validation_error(po::validation_error::invalid_option_value,
			       "interface");
  }
  std::getline(stream, tok);
  v = boost::any(interface(tok, port));
}

void
OptionsParser::parse(int argc, char *argv[])
{
  namespace po = boost::program_options;

  po::options_description description("Options");

  description.add_options()
    ("help,h", "Display this help message")
    ("interface,i", po::value<std::vector<interface> >()->composing(),
     "<port-num>@<interface-name>: "
     "Attach network interface <interface-name> as port <port-num> at startup. "
     "Can appear multiple times")
    ("pcap", "Generate pcap files for interfaces")
    ("useFiles", po::value<int>(), "Read/write packets from files (interface X "
     "corresponds to two files X_in.pcap and X_out.pcap).  Argument is the time"
     "to wait (in seconds) before starting to process the packet files.")
    ("thrift-port", po::value<int>(),
     "TCP port on which to run the Thrift runtime server")
    ("device-id", po::value<int>(),
     "Device ID, used to identify the device in IPC messages (default 0)")
    ("nanolog", po::value<std::string>(),
     "IPC socket to use for nanomsg pub/sub logs (default: no nanomsg logging")
    ("log-console",
     "Enable logging on stdout")
    ("log-file", po::value<std::string>(),
     "Enable logging to given file");

  po::options_description hidden;
  hidden.add_options()
    ("input-config", po::value<std::string>(), "input config");

  po::options_description options;
  options.add(description).add(hidden);

  po::positional_options_description positional;
  positional.add("input-config", 1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).
	      options(options).
	      positional(positional).run(), vm);
    po::notify(vm);
  }
  catch(...) {
    std::cout << "Error while parsing command line arguments\n";
    std::cout << "Usage: SWITCH_NAME [options] <path to JSON config file>\n";
    std::cout << description;
    exit(1);
  }

  if(vm.count("help")) {
    std::cout << "Usage: SWITCH_NAME [options] <path to JSON config file>\n";
    std::cout << description;
    exit(0);
  }

  if(!vm.count("input-config")) {
    std::cout << "Error: please specify an input JSON configuration file\n";
    std::cout << "Usage: SWITCH_NAME [options] <path to JSON config file>\n";
    std::cout << description;
    exit(1);
  }

  device_id = 0;
  if(vm.count("device-id")) {
    device_id = vm["device-id"].as<int>();
  }

  // event_logger_addr = std::string("ipc:///tmp/bm-")
  //   .append(std::to_string(device_id))
  //   .append("-log.ipc");
  if(vm.count("nanolog")) {
    event_logger_addr = vm["nanolog"].as<std::string>();
    event_logger = new EventLogger(
      TransportIface::create_instance<TransportNanomsg>(event_logger_addr)
    );
  }

  if(vm.count("log-console") && vm.count("log-file")) {
    std::cout << "Error: --log-console and --log-file are exclusive\n";
    exit(1);
  }

  if(vm.count("log-console")) {
    console_logging = true;
  }

  if(vm.count("log-file")) {
    file_logger = vm["log-file"].as<std::string>();
  }

  if(vm.count("interface")) {
    for (const auto &iface : vm["interface"].as<std::vector<interface> >()) {
      ifaces.add(iface.port, iface.name);
    }
  }

  if(vm.count("pcap")) {
    pcap = true;
  }

  if(vm.count("useFiles"))
  {
    useFiles = true;
    waitTime = vm["useFiles"].as<int>();
    if (waitTime < 0)
      waitTime = 0;
  }

  assert(vm.count("input-config"));
  config_file_path = vm["input-config"].as<std::string>();

  int default_thrift_port = 9090;
  if(vm.count("thrift-port")) {
    thrift_port = vm["thrift-port"].as<int>();
  }
  else {
    std::cout << "Thrift port was not specified, will use "
	      << default_thrift_port
	      << std::endl;
    thrift_port = default_thrift_port;
  }
}
