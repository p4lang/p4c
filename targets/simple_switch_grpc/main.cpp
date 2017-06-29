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

/* Switch instance */

#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/target_parser.h>

#include <bm/PI/pi.h>

#include <PI/frontends/proto/device_mgr.h>

#include <PI/proto/pi_server.h>

#include <PI/pi.h>
#include <PI/target/pi_imp.h>

#include "simple_switch.h"

namespace {
SimpleSwitch *simple_switch;
bm::TargetParserBasic *simple_switch_parser;
}  // namespace

// We use the PI native RPC server (built on top of nanomsg) as a debugging
// tool, since we do not have a CLI using p4runtime.proto yet.
extern "C" {

extern pi_status_t pi_rpc_server_run(const pi_remote_addr_t *remote_addr);

}

int
main(int argc, char* argv[]) {
  simple_switch = new SimpleSwitch();
  simple_switch_parser = new bm::TargetParserBasic();
  simple_switch_parser->add_flag_option("enable-swap",
                                        "enable JSON swapping at runtime");
  simple_switch_parser->add_string_option(
      "grpc-server-addr",
      "bind gRPC server to given address [default is 0.0.0.0:50051]");
  simple_switch_parser->add_int_option(
      "cpu-port",
      "set CPU port, will be used for packet-in / packet-out; "
      "do not add an interface with this port number");
  int status = simple_switch->init_from_command_line_options(
      argc, argv, simple_switch_parser);
  if (status != 0) std::exit(status);

  bool enable_swap_flag = false;
  {
    auto rc = simple_switch_parser->get_flag_option(
        "enable-swap", &enable_swap_flag);
    if (rc != bm::TargetParserBasic::ReturnCode::SUCCESS) std::exit(1);
  }
  if (enable_swap_flag) simple_switch->enable_config_swap();

  std::string grpc_server_addr;
  {
    auto rc = simple_switch_parser->get_string_option(
        "grpc-server-addr", &grpc_server_addr);
    if (rc == bm::TargetParserBasic::ReturnCode::OPTION_NOT_PROVIDED)
      grpc_server_addr = "0.0.0.0:50051";
    else if (rc != bm::TargetParserBasic::ReturnCode::SUCCESS)
      std::exit(1);
  }

  int cpu_port = -1;
  {
    auto rc = simple_switch_parser->get_int_option("cpu-port", &cpu_port);
    if (rc == bm::TargetParserBasic::ReturnCode::OPTION_NOT_PROVIDED)
      cpu_port = -1;
    else if (rc != bm::TargetParserBasic::ReturnCode::SUCCESS || cpu_port < 0)
      std::exit(1);
  }

  // check if CPU port number is also used by --interface
  // TODO(antonin): ports added dynamically?
  if (cpu_port >= 0) {
    auto port_info = simple_switch->get_port_info();
    if (port_info.find(cpu_port) != port_info.end()) {
      bm::Logger::get()->error("Cpu port {} is used as a data port", cpu_port);
      std::exit(1);
    }
  }

  if (cpu_port >= 0) {
    auto transmit_fn = [cpu_port](int port_num, const char *buf, int len) {
      if (port_num == cpu_port) {
        BMLOG_DEBUG("Transmitting packet-in");
        auto status = pi_packetin_receive(
            simple_switch->get_device_id(), buf, static_cast<size_t>(len));
        if (status != PI_STATUS_SUCCESS)
          bm::Logger::get()->error("Error when transmitting packet-in");
      } else {
        simple_switch->transmit_fn(port_num, buf, len);
      }
    };
    simple_switch->set_transmit_fn(transmit_fn);
  }

  bm::pi::register_switch(simple_switch, cpu_port);

  using pi::fe::proto::DeviceMgr;
  DeviceMgr::init(256);
  PIGrpcServerRunAddr(grpc_server_addr.c_str());

  simple_switch->start_and_return();

#ifdef PI_INTERNAL_RPC
  char *opt_rpc_addr = NULL;
  char *opt_notifications_addr = NULL;
  pi_remote_addr_t remote_addr = {opt_rpc_addr, opt_notifications_addr};
  std::thread pi_CLI_thread(
      [&remote_addr] { pi_rpc_server_run(&remote_addr); });
#endif

  PIGrpcServerWait();
  PIGrpcServerCleanup();
  DeviceMgr::destroy();

  return 0;
}
