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

#include <bm/bm_sim/target_parser.h>

#include <bm/PI/pi.h>

#include <PI/frontends/proto/device_mgr.h>

#include <PI/proto/pi_server.h>

#include <PI/pi.h>

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
  int status = simple_switch->init_from_command_line_options(
      argc, argv, simple_switch_parser);
  if (status != 0) std::exit(status);

  bool enable_swap_flag = false;
  if (simple_switch_parser->get_flag_option("enable-swap", &enable_swap_flag)
      != bm::TargetParserBasic::ReturnCode::SUCCESS)
    std::exit(1);
  if (enable_swap_flag) simple_switch->enable_config_swap();

  bm::pi::register_switch(simple_switch);

  using pi::fe::proto::DeviceMgr;
  DeviceMgr::init(256);
  PIGrpcServerRun();

  simple_switch->start_and_return();

  char *opt_rpc_addr = NULL;
  char *opt_notifications_addr = NULL;
  pi_remote_addr_t remote_addr = {opt_rpc_addr, opt_notifications_addr};
  std::thread pi_CLI_thread(
      [&remote_addr] { pi_rpc_server_run(&remote_addr); });

  PIGrpcServerWait();
  PIGrpcServerCleanup();
  DeviceMgr::destroy();

  return 0;
}
