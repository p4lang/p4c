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

#include "bm_sim/dev_mgr.h"
#include "bm_sim/port_monitor.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <thread>

class TestDevMgrImplementation : public DevMgrInterface {

public:
  TestDevMgrImplementation() { p_monitor.start(this); }

  bool port_is_up(port_t port) {
    auto it = port_status.find(port);
    bool exists = (it != port_status.end());

    return (exists && ((it->second == PortStatus::PORT_ADDED) ||
                       (it->second == PortStatus::PORT_UP)));
  }

   ~TestDevMgrImplementation() {
    p_monitor.stop();
    port_status.clear();
  }

  ReturnCode register_status_cb(const PortStatus &type,
                                const PortStatusCB &port_cb) {
    p_monitor.register_cb(type, port_cb);
    return ReturnCode::SUCCESS;
  }

  // Not part of public interface, just for testing
  void set_port_status(port_t port, const PortStatus &status) {
    auto it = port_status.find(port);
    bool exists = (it != port_status.end());
    std::string portname = "dummy";
    if (!exists && status == PortStatus::PORT_ADDED) {
      port_add(portname, port, NULL, NULL);
    } else if (exists && status == PortStatus::PORT_REMOVED) {
      port_remove(port);
    } else if (exists && ((it->second == PortStatus::PORT_ADDED) ||
                          (it->second == PortStatus::PORT_DOWN)) &&
               status == PortStatus::PORT_UP) {
      port_status[port] = PortStatus::PORT_UP;
    } else if (exists && ((it->second == PortStatus::PORT_ADDED ||
                           it->second == PortStatus::PORT_UP)) &&
               status == PortStatus::PORT_DOWN) {
      port_status[port] = PortStatus::PORT_DOWN;
    } else {
      ASSERT_TRUE(false) << "Incorrect op sequence on port: " << port
                         << std::endl;
    }
  }

private:
  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) {
    port_status.insert(
        std::pair<port_t, PortStatus>(port_num, PortStatus::PORT_ADDED));
    p_monitor.notify(port_num, PortStatus::PORT_ADDED);
    return ReturnCode::SUCCESS;
  }
  ReturnCode port_remove(port_t port_num) {
    auto it = port_status.find(port_num);
    port_status.erase(it);
    p_monitor.notify(port_num, PortStatus::PORT_REMOVED);
  }
  ReturnCode set_packet_handler(PacketHandler handler, void *cookie) {
    return ReturnCode::SUCCESS;
  }
  void transmit_fn(int port_num, const char *buffer, int len) {}
  void start() {}

  std::map<port_t, PortStatus> port_status;
  PortMonitor p_monitor;
};

class DevMgrTest : public ::testing::Test {
public:

  void port_status(DevMgrInterface::port_t port_num,
                   const DevMgrInterface::PortStatus &status) {
    cb_counts[status]++;
  }

protected:
  DevMgrTest() : g_mgr{new TestDevMgrImplementation()} {}
  void register_callback(void) {
    port_cb = std::bind(&DevMgrTest::port_status, this, std::placeholders::_1,
                        std::placeholders::_2);
    g_mgr->register_status_cb(DevMgrInterface::PortStatus::PORT_ADDED, port_cb);
    g_mgr->register_status_cb(DevMgrInterface::PortStatus::PORT_REMOVED,
                              port_cb);
    g_mgr->register_status_cb(DevMgrInterface::PortStatus::PORT_UP, port_cb);
    g_mgr->register_status_cb(DevMgrInterface::PortStatus::PORT_DOWN, port_cb);
  }

  void reset_counts(void) {
    for (auto &kv : cb_counts) {
      kv.second = 0;
    }
  }
  DevMgrInterface::PortStatusCB port_cb;
  std::map<DevMgrInterface::PortStatus, uint32_t> cb_counts;
  std::unique_ptr<TestDevMgrImplementation> g_mgr;
};

TEST_F(DevMgrTest, cb_test) {
  const unsigned int NPORTS = 10;

  register_callback();
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrInterface::PortStatus::PORT_ADDED);
  }
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(cb_counts[DevMgrInterface::PortStatus::PORT_ADDED], NPORTS) << "Port add callbacks incorrect"
                                    << std::endl;
  reset_counts();
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrInterface::PortStatus::PORT_DOWN);
  }
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(cb_counts[DevMgrInterface::PortStatus::PORT_DOWN], NPORTS) << "Port down callbacks incorrect"
                                    << std::endl;

  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrInterface::PortStatus::PORT_UP);
  }
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(cb_counts[DevMgrInterface::PortStatus::PORT_UP], NPORTS) << "Port up callbacks incorrect"
                                    << std::endl;
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrInterface::PortStatus::PORT_REMOVED);
  }
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(NPORTS, cb_counts[DevMgrInterface::PortStatus::PORT_REMOVED])
      << "Number of port remove callbacks incorrect" << std::endl;
}
