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

#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/target_parser.h>

#include <string>

#include "switch_runner.h"

int
main(int argc, char* argv[]) {
  bm::TargetParserBasic simple_switch_parser;
  simple_switch_parser.add_flag_option(
      "disable-swap",
      "disable JSON swapping at runtime");
  simple_switch_parser.add_string_option(
      "grpc-server-addr",
      "bind gRPC server to given address [default is 0.0.0.0:50051]");
  simple_switch_parser.add_uint_option(
      "cpu-port",
      "set non-zero CPU port, will be used for packet-in / packet-out; "
      "do not add an interface with this port number");
  simple_switch_parser.add_string_option(
      "dp-grpc-server-addr",
      "use a gRPC channel to inject and receive dataplane packets; "
      "bind this gRPC server to given address, e.g. 0.0.0.0:50052");

  bm::OptionsParser parser;
  parser.parse(argc, argv, &simple_switch_parser);

  std::string dp_grpc_server_addr;
  {
    auto rc = simple_switch_parser.get_string_option(
        "dp-grpc-server-addr", &dp_grpc_server_addr);
    if (rc != bm::TargetParserBasic::ReturnCode::OPTION_NOT_PROVIDED &&
        rc != bm::TargetParserBasic::ReturnCode::SUCCESS)
      std::exit(1);
  }

  bool disable_swap_flag = false;
  {
    auto rc = simple_switch_parser.get_flag_option(
        "disable-swap", &disable_swap_flag);
    if (rc != bm::TargetParserBasic::ReturnCode::SUCCESS) std::exit(1);
  }

  std::string grpc_server_addr;
  {
    auto rc = simple_switch_parser.get_string_option(
        "grpc-server-addr", &grpc_server_addr);
    if (rc == bm::TargetParserBasic::ReturnCode::OPTION_NOT_PROVIDED)
      grpc_server_addr = "0.0.0.0:50051";
    else if (rc != bm::TargetParserBasic::ReturnCode::SUCCESS)
      std::exit(1);
  }

  uint32_t cpu_port = 0xFFFFFFFF;
  {
    auto rc = simple_switch_parser.get_uint_option("cpu-port", &cpu_port);
    if (rc == bm::TargetParserBasic::ReturnCode::OPTION_NOT_PROVIDED)
      cpu_port = 0;
    else if (rc != bm::TargetParserBasic::ReturnCode::SUCCESS || cpu_port == 0)
      std::exit(1);
  }

  auto &runner = sswitch_grpc::SimpleSwitchGrpcRunner::get_instance(
      512, !disable_swap_flag, grpc_server_addr, cpu_port, dp_grpc_server_addr);
  int status = runner.init_and_start(parser);
  if (status != 0) std::exit(status);

  runner.wait();
  return 0;
}
