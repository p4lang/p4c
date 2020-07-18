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

#ifndef SIMPLE_SWITCH_GRPC_SWITCH_RUNNER_H_
#define SIMPLE_SWITCH_GRPC_SWITCH_RUNNER_H_

#include <bm/bm_sim/dev_mgr.h>

#include <grpcpp/server.h>

#include <memory>
#include <string>

class SimpleSwitch;

namespace bm {

class OptionsParser;

}  // namespace bm

namespace sswitch_grpc {

class SysrepoDriver;

class DataplaneInterfaceServiceImpl;

class SimpleSwitchGrpcRunner {
 public:
  static constexpr bm::DevMgrIface::port_t default_drop_port = 511;

  // there is no real need for a singleton here, except for the fact that we use
  // PIGrpcServerRunAddr, ... which uses static state
  static SimpleSwitchGrpcRunner &get_instance(
      bool enable_swap = false,
      std::string grpc_server_addr = "0.0.0.0:50051",
      bm::DevMgrIface::port_t cpu_port = 0,
      std::string dp_grpc_server_addr = "",
      bm::DevMgrIface::port_t drop_port = default_drop_port) {
    static SimpleSwitchGrpcRunner instance(
        enable_swap, grpc_server_addr, cpu_port, dp_grpc_server_addr,
        drop_port);
    return instance;
  }

  int init_and_start(const bm::OptionsParser &parser);
  void wait();
  void shutdown();
  int get_dp_grpc_server_port() {
    return dp_grpc_server_port;
  }
  void block_until_all_packets_processed();
  bool is_dp_service_active();

 private:
  SimpleSwitchGrpcRunner(bool enable_swap = false,
                         std::string grpc_server_addr = "0.0.0.0:50051",
                         bm::DevMgrIface::port_t cpu_port = 0,
                         std::string dp_grpc_server_addr = "",
                         bm::DevMgrIface::port_t drop_port = default_drop_port);
  ~SimpleSwitchGrpcRunner();

  void port_status_cb(bm::DevMgrIface::port_t port,
                      const bm::DevMgrIface::PortStatus port_status);

  std::unique_ptr<SimpleSwitch> simple_switch;
  std::string grpc_server_addr;
  bm::DevMgrIface::port_t cpu_port;
  std::string dp_grpc_server_addr;
  int dp_grpc_server_port;
  DataplaneInterfaceServiceImpl *dp_service;
  std::unique_ptr<grpc::Server> dp_grpc_server;
#ifdef WITH_SYSREPO
  std::unique_ptr<SysrepoDriver> sysrepo_driver;
#endif  // WITH_SYSREPO
};

}  // namespace sswitch_grpc

#endif  // SIMPLE_SWITCH_GRPC_SWITCH_RUNNER_H_
