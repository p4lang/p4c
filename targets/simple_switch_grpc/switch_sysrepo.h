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

#ifndef SIMPLE_SWITCH_GRPC_SWITCH_SYSREPO_H_
#define SIMPLE_SWITCH_GRPC_SWITCH_SYSREPO_H_

#include <bm/bm_sim/device_id.h>

#include <memory>
#include <string>

namespace bm {

class DevMgr;

}  // namespace bm

namespace sswitch_grpc {

class PortStateMap;
class SysrepoSubscriber;
class SysrepoStateProvider;

class SysrepoDriver {
 public:
  SysrepoDriver(bm::device_id_t device_id, bm::DevMgr *dev_mgr);
  ~SysrepoDriver();

  bool start();

  // Used to add interfaces provided on the command-line with --interface / -i
  void add_iface(int port, const std::string &name);

 private:
  const bm::device_id_t my_device_id;
  const bm::DevMgr *dev_mgr;  // non-owning pointer
  std::unique_ptr<PortStateMap> port_state_map;
  std::unique_ptr<SysrepoSubscriber> sysrepo_subscriber;
  std::unique_ptr<SysrepoStateProvider> sysrepo_state_provider;
};

}  // namespace sswitch_grpc

#endif  // SIMPLE_SWITCH_GRPC_SWITCH_SYSREPO_H_
