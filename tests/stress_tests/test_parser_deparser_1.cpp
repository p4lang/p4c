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
#include <string>
#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>

#include "stress_utils.h"

using ::stress_tests_utils::SwitchTest;
using ::stress_tests_utils::TestChrono;

namespace fs = boost::filesystem;

int main(int argc, char* argv[]) {
  size_t num_repeats = 1000;
  if (argc > 1) num_repeats = std::stoul(argv[1]);

  SwitchTest sw;
  fs::path config_path =
      fs::path(TESTDATADIR) / fs::path("parser_deparser_1.json");
  sw.init_objects(config_path.string());

  fs::path traffic_path =
      fs::path(TESTDATADIR) / fs::path("udp_tcp_traffic.bin");
  auto packets = sw.read_traffic(traffic_path.string());

  auto parser = sw.get_parser("parser");
  auto deparser = sw.get_deparser("deparser");

  size_t packet_cnt = packets.size();
  TestChrono chrono(packet_cnt * num_repeats);
  chrono.start();
  for (size_t iter = 0; iter < num_repeats; iter++) {
    for (size_t p = 0; p < packet_cnt; p++) {
      auto pkt = packets[p].get();
      parser->parse(pkt);
      deparser->deparse(pkt);
    }
  }
  chrono.end();
  chrono.print_summary();
}
