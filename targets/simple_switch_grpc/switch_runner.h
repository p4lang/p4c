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

#include <memory>
#include <string>

class SimpleSwitch;

namespace grpc {

class Server;

}  // namespace grpc

namespace bm {

class OptionsParser;

}  // namespace bm

namespace sswitch_grpc {

class SimpleSwitchGrpcRunner {
 public:
  // there is no real need for a singleton here, except for the fact that we use
  // PIGrpcServerRunAddr, ... which uses static state
  static SimpleSwitchGrpcRunner &get_instance(
      int max_port = 512, bool enable_swap = false,
      std::string grpc_server_addr = "0.0.0.0:50051",
      int cpu_port = -1,
      std::string dp_grpc_server_addr = "") {
    static SimpleSwitchGrpcRunner instance(
        max_port, enable_swap, grpc_server_addr, cpu_port, dp_grpc_server_addr);
    return instance;
  }

  int init_and_start(const bm::OptionsParser &parser);
  void wait();
  void shutdown();

 private:
  SimpleSwitchGrpcRunner(int max_port = 512, bool enable_swap = false,
                         std::string grpc_server_addr = "0.0.0.0:50051",
                         int cpu_port = -1,
                         std::string dp_grpc_server_addr = "");
  ~SimpleSwitchGrpcRunner();

  std::unique_ptr<SimpleSwitch> simple_switch;
  std::string grpc_server_addr;
  int cpu_port;
  std::string dp_grpc_server_addr;
  std::unique_ptr<grpc::Server> dp_grpc_server;
};

}  // namespace sswitch_grpc

#endif  // SIMPLE_SWITCH_GRPC_SWITCH_RUNNER_H_
