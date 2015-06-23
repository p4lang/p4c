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

#include <cassert>

#include <boost/program_options.hpp>

#include "bm_sim/options_parse.h"

void
OptionsParser::parse(int argc, char *argv[])
{
  namespace po = boost::program_options;

  po::options_description description("Options");

  description.add_options()
    ("help,h", "Display this help message")
    ("interface,i", po::value<std::vector<std::string> >()->composing(),
     "Attach this network interface at startup")
    ("pcap", "Generate pcap files for interfaces");

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

  if(vm.count("interface")) {
    for (const auto &iface : vm["interface"].as<std::vector<std::string> >()) {
      ifaces.append(iface);
    }
  }

  if(vm.count("pcap")) {
    pcap = true;
  }

  assert(vm.count("input-config"));
  config_file_path = vm["input-config"].as<std::string>();
}
