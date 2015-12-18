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
#include "bm_apps/packet_pipe.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

class TestDevMgrImp : public DevMgrIface {

public:
  TestDevMgrImp() {
    p_monitor = std::move(PortMonitorIface::make_active());
  }

  // Not part of public interface, just for testing
  void set_port_status(port_t port, const PortStatus status) {
    ASSERT_TRUE(status == PortStatus::PORT_UP ||
                status == PortStatus::PORT_DOWN);
    std::lock_guard<std::mutex> lock(status_mutex);
    auto it = port_status.find(port);
    bool exists = it != port_status.end();
    ASSERT_TRUE(exists) << "Incorrect op sequence on port: " << port
                        << std::endl;
    if ((it->second == PortStatus::PORT_ADDED ||
         it->second == PortStatus::PORT_DOWN) &&
        status == PortStatus::PORT_UP) {
      port_status[port] = PortStatus::PORT_UP;
    } else if ((it->second == PortStatus::PORT_ADDED ||
                it->second == PortStatus::PORT_UP) &&
               status == PortStatus::PORT_DOWN) {
        port_status[port] = PortStatus::PORT_DOWN;
    } else {
      ASSERT_TRUE(false) << "Incorrect op sequence on port: " << port
                         << std::endl;
    }
  }

private:
  bool port_is_up_(port_t port) override {
    std::lock_guard<std::mutex> lock(status_mutex);
    auto it = port_status.find(port);
    bool exists = (it != port_status.end());

    return (exists && ((it->second == PortStatus::PORT_ADDED) ||
                       (it->second == PortStatus::PORT_UP)));
  }

  ReturnCode port_add_(const std::string &iface_name, port_t port_num,
                       const char *in_pcap, const char *out_pcap) override {
    (void) iface_name;
    (void) in_pcap;
    (void) out_pcap;
    std::lock_guard<std::mutex> lock(status_mutex);
    auto it = port_status.find(port_num);
    if (it != port_status.end()) return ReturnCode::ERROR;
    port_status.insert(std::make_pair(port_num, PortStatus::PORT_ADDED));
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove_(port_t port_num) override {
    std::lock_guard<std::mutex> lock(status_mutex);
    auto it = port_status.find(port_num);
    if (it == port_status.end()) return ReturnCode::ERROR;
    port_status.erase(it);
    return ReturnCode::SUCCESS;
  }

  ReturnCode set_packet_handler_(const PacketHandler &handler, void *cookie)
      override{
    (void) handler;
    (void) cookie;
    return ReturnCode::SUCCESS;
  }

  void transmit_fn_(int port_num, const char *buffer, int len) override {}

  void start_() override {}

  std::map<port_t, PortStatus> port_status{};
  mutable std::mutex status_mutex{};
};

class DevMgrTest : public ::testing::Test {
public:

  void port_status(DevMgrIface::port_t port_num,
                   const DevMgrIface::PortStatus status) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    cb_counts[status]++;
  }

protected:
  DevMgrTest()
      : g_mgr(new TestDevMgrImp()) {
    g_mgr->start();
  }

  void register_callback(void) {
    port_cb = std::bind(&DevMgrTest::port_status, this, std::placeholders::_1,
                        std::placeholders::_2);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_ADDED, port_cb);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_REMOVED, port_cb);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_UP, port_cb);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_DOWN, port_cb);
  }

  void reset_counts(void) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    for (auto &kv : cb_counts) {
      kv.second = 0;
    }
  }

  DevMgrIface::PortStatusCb port_cb{};
  std::map<DevMgrIface::PortStatus, uint32_t> cb_counts{};
  mutable std::mutex cnt_mutex{};
  std::unique_ptr<TestDevMgrImp> g_mgr{nullptr};
};

TEST_F(DevMgrTest, cb_test) {
  const unsigned int NPORTS = 10;

  register_callback();
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->port_add("dummyport", i, nullptr, nullptr);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    ASSERT_EQ(cb_counts[DevMgrIface::PortStatus::PORT_ADDED], NPORTS)
        << "Port add callbacks incorrect" << std::endl;
  }
  reset_counts();
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrIface::PortStatus::PORT_DOWN);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    ASSERT_EQ(cb_counts[DevMgrIface::PortStatus::PORT_DOWN], NPORTS)
        << "Port down callbacks incorrect" << std::endl;
  }
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrIface::PortStatus::PORT_UP);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    ASSERT_EQ(cb_counts[DevMgrIface::PortStatus::PORT_UP], NPORTS)
        << "Port up callbacks incorrect" << std::endl;
  }
  for (int i = 0; i < NPORTS; i++) {
    g_mgr->port_remove(i);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    ASSERT_EQ(NPORTS, cb_counts[DevMgrIface::PortStatus::PORT_REMOVED])
        << "Number of port remove callbacks incorrect" << std::endl;
  }
}

class PacketInReceiver {
 public:
  enum class Status { CAN_READ, CAN_RECEIVE };

  PacketInReceiver(size_t max_size)
      : max_size(max_size) {
    buffer_.reserve(max_size);
  }

  void receive(int port_num, const char *buffer, int len, void *cookie) {
    (void) cookie;
    if(len > max_size) return;
    std::unique_lock<std::mutex> lock(mutex);
    while (status != Status::CAN_RECEIVE) {
      can_receive.wait(lock);
    }
    buffer_.insert(buffer_.end(), buffer, buffer + len);
    port = port_num;
    status = Status::CAN_READ;
    can_read.notify_one();
  }

  bool read(char *dst, size_t len, int *recv_port,
            unsigned int timeout_ms = 1000) {
    len = (len > max_size) ? max_size : len;
    typedef std::chrono::system_clock clock;
    clock::time_point tp_start = clock::now();
    clock::time_point tp_end = tp_start + std::chrono::milliseconds(timeout_ms);
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_READ) {
      if (clock::now() > tp_end)
        return false;
      can_read.wait_until(lock, tp_end);
    }
    std::copy(buffer_.begin(), buffer_.begin() + len, dst);
    buffer_.clear();
    *recv_port = port;
    status = Status::CAN_RECEIVE;
    can_receive.notify_one();
    return true;
  }

  Status check_status() {
    std::unique_lock<std::mutex> lock(mutex);
    return status;
  }

 private:
  size_t max_size;
  std::vector<char> buffer_;
  int port;
  Status status{Status::CAN_RECEIVE};
  mutable std::mutex mutex{};
  mutable std::condition_variable can_receive{};
  mutable std::condition_variable can_read{};
};

// is here because DevMgr has a protected destructor
class PacketInSwitch : public DevMgr {
};

class PacketInDevMgrTest : public ::testing::Test {
 protected:
  static constexpr size_t max_buffer_size = 512;

  PacketInDevMgrTest()
      : packet_inject(addr) { }

  void SetUp_(bool enforce_ports) {
    sw.set_dev_mgr_packet_in(addr, enforce_ports);
    sw.start();
    auto cb_switch = std::bind(&PacketInReceiver::receive, &recv_switch,
                               std::placeholders::_1, std::placeholders::_2,
                               std::placeholders::_3, std::placeholders::_4);
    sw.set_packet_handler(cb_switch, nullptr);

    packet_inject.start();
    auto cb_lib = std::bind(&PacketInReceiver::receive, &recv_lib,
                            std::placeholders::_1, std::placeholders::_2,
                            std::placeholders::_3, std::placeholders::_4);
    packet_inject.set_packet_receiver(cb_lib, nullptr);
  }

  virtual void SetUp() {
    SetUp_(false);
  }

  virtual void TearDown() {
  }

  bool check_recv(PacketInReceiver *receiver,
                  int send_port, const char *send_buffer, size_t size,
                  unsigned int timeout_ms = 1000) {
    char recv_buffer[max_buffer_size];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    if (size > sizeof(recv_buffer)) return false;
    int recv_port = -1;
    if (!receiver->read(recv_buffer, size, &recv_port, timeout_ms))
      return false;
    if (recv_port != send_port) return false;
    return !memcmp(recv_buffer, send_buffer, size);
  }

  const std::string addr = "ipc:///tmp/test_packet_in_abc123";

  PacketInReceiver recv_switch{max_buffer_size};
  PacketInReceiver recv_lib{max_buffer_size};

  PacketInSwitch sw;

  bm_apps::PacketInject packet_inject;
};

TEST_F(PacketInDevMgrTest, PacketInTest) {
  constexpr int port = 2;
  const char pkt[] = {'\x0a', '\xba'};
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_switch.check_status());
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_lib.check_status());
  // switch -> lib
  sw.transmit_fn(port, pkt, sizeof(pkt));
  ASSERT_TRUE(check_recv(&recv_lib, port, pkt, sizeof(pkt)));
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_switch.check_status());
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_lib.check_status());
  // lib -> switch
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_TRUE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
}

class PacketInDevMgrPortStatusTest : public PacketInDevMgrTest {
 protected:

  PacketInDevMgrPortStatusTest() { }

  virtual void SetUp() {
    SetUp_(true);
    register_callback();
  }

  virtual void TearDown() {
  }

  void port_status(DevMgrIface::port_t port_num,
                   const DevMgrIface::PortStatus status) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    cb_counts[status]++;
  }

  void register_callback(void) {
    port_cb = std::bind(&PacketInDevMgrPortStatusTest::port_status, this,
                        std::placeholders::_1, std::placeholders::_2);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_ADDED, port_cb);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_REMOVED, port_cb);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_UP, port_cb);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_DOWN, port_cb);
  }

  void reset_counts(void) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    for (auto &kv : cb_counts) {
      kv.second = 0;
    }
  }

  uint32_t get_count(DevMgrIface::PortStatus status) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    return cb_counts[status];
  }

  void check_and_reset_counts(uint32_t count_added, uint32_t count_removed,
                              uint32_t count_up, uint32_t count_down) {
    ASSERT_EQ(count_added, get_count(DevMgrIface::PortStatus::PORT_ADDED));
    ASSERT_EQ(count_removed, get_count(DevMgrIface::PortStatus::PORT_REMOVED));
    ASSERT_EQ(count_up, get_count(DevMgrIface::PortStatus::PORT_UP));
    ASSERT_EQ(count_down, get_count(DevMgrIface::PortStatus::PORT_DOWN));
    reset_counts();
  }

  DevMgrIface::PortStatusCb port_cb{};
  std::map<DevMgrIface::PortStatus, uint32_t> cb_counts{};
  mutable std::mutex cnt_mutex{};
};

TEST_F(PacketInDevMgrPortStatusTest, BadPort) {
  constexpr int port = 2;
  const char pkt[] = {'\x0a', '\xba'};
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_switch.check_status());
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_lib.check_status());
  // lib -> switch
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_FALSE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
}

TEST_F(PacketInDevMgrPortStatusTest, Basic) {
  constexpr int port = 2;
  const char pkt[] = {'\x0a', '\xba'};
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_switch.check_status());
  ASSERT_EQ(PacketInReceiver::Status::CAN_RECEIVE, recv_lib.check_status());
  packet_inject.port_add(port);
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_TRUE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
  packet_inject.port_remove(port);
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_FALSE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
}

TEST_F(PacketInDevMgrPortStatusTest, Status) {
  constexpr int port = 2;
  const char pkt[] = {'\x0a', '\xba'};

  packet_inject.port_add(port);
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_TRUE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  check_and_reset_counts(1u, 0u, 1u, 0u);

  packet_inject.port_bring_down(port);
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_FALSE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  check_and_reset_counts(0u, 0u, 0u, 1u);

  packet_inject.port_bring_up(port);
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_TRUE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  check_and_reset_counts(0u, 0u, 1u, 0u);

  packet_inject.port_remove(port);
  packet_inject.send(port, pkt, sizeof(pkt));
  ASSERT_FALSE(check_recv(&recv_switch, port, pkt, sizeof(pkt)));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  check_and_reset_counts(0u, 1u, 0u, 0u);
}
