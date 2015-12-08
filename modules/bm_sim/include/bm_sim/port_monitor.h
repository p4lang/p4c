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
 * Hari Thantry (thantry@barefootnetworks.com)
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_SIM_INCLUDE_BM_SIM_PORT_MONITOR_H_
#define BM_SIM_INCLUDE_BM_SIM_PORT_MONITOR_H_

#include <mutex>
#include <thread>
#include <unordered_map>

#include "dev_mgr.h"

class PortMonitor {
 public:
  explicit PortMonitor(uint32_t sleep_dur = 1000);
  void notify(DevMgrInterface::port_t port_num,
              const DevMgrInterface::PortStatus &evt);
  void register_cb(const DevMgrInterface::PortStatus &evt,
                   const DevMgrInterface::PortStatusCB &cb);
  void start(DevMgrInterface *dev_mgr);
  void stop(void);
  ~PortMonitor();

 private:
  void port_handler(DevMgrInterface::port_t port,
                    const DevMgrInterface::PortStatus &evt);
  void monitor(void);
  uint32_t ms_sleep;
  bool run_monitor;
  std::unique_ptr<std::thread> p_monitor;
  std::unordered_multimap<unsigned int, const DevMgrInterface::PortStatusCB &>
      cb_map;
  std::unordered_map<DevMgrInterface::port_t, bool> curr_ports;
  mutable std::mutex cb_map_mutex;
  mutable std::mutex port_mutex;
  DevMgrInterface *peer;
};

#endif  // BM_SIM_INCLUDE_BM_SIM_PORT_MONITOR_H_
