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

#include <bm/bm_sim/dev_mgr.h>
#include <bm/bm_sim/port_monitor.h>
#include <bm/bm_apps/packet_pipe.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "utils.h"

using namespace bm;

using testing::Types;

class TestDevMgrImp : public DevMgrIface {

public:
  TestDevMgrImp() {
    // 0 is device_id
    p_monitor = std::move(PortMonitorIface::make_active(0));
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
  bool port_is_up_(port_t port) const override {
    std::lock_guard<std::mutex> lock(status_mutex);
    auto it = port_status.find(port);
    bool exists = (it != port_status.end());

    return (exists && ((it->second == PortStatus::PORT_ADDED) ||
                       (it->second == PortStatus::PORT_UP)));
  }

  std::map<port_t, PortInfo> get_port_info_() const override {
    // TODO(antonin): add test
    return {};
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

  void register_callback() {
    port_cb = std::bind(&DevMgrTest::port_status, this, std::placeholders::_1,
                        std::placeholders::_2);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_ADDED, port_cb);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_REMOVED, port_cb);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_UP, port_cb);
    g_mgr->register_status_cb(DevMgrIface::PortStatus::PORT_DOWN, port_cb);
  }

  void reset_counts() {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    for (auto &kv : cb_counts) {
      kv.second = 0;
    }
  }

  uint32_t get_count(DevMgrIface::PortStatus status) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    return cb_counts.at(status);
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
  ASSERT_EQ(NPORTS, get_count(DevMgrIface::PortStatus::PORT_ADDED))
      << "Port add callbacks incorrect" << std::endl;
  reset_counts();

  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrIface::PortStatus::PORT_DOWN);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(NPORTS, get_count(DevMgrIface::PortStatus::PORT_DOWN))
      << "Port down callbacks incorrect" << std::endl;

  for (int i = 0; i < NPORTS; i++) {
    g_mgr->set_port_status(i, DevMgrIface::PortStatus::PORT_UP);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(NPORTS, get_count(DevMgrIface::PortStatus::PORT_UP))
      << "Port up callbacks incorrect" << std::endl;

  for (int i = 0; i < NPORTS; i++) {
    g_mgr->port_remove(i);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(NPORTS, get_count(DevMgrIface::PortStatus::PORT_REMOVED))
      << "Number of port remove callbacks incorrect" << std::endl;
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

  void SetUp_(bool enforce_ports,
              std::shared_ptr<TransportIface> notifications_transport) {
    // 0 is device id
    sw.set_dev_mgr_packet_in(0, addr, notifications_transport, enforce_ports);
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
    SetUp_(false, nullptr);
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
    SetUp_(true, nullptr);
    register_callback();
  }

  virtual void TearDown() {
  }

  void port_status(DevMgrIface::port_t port_num,
                   const DevMgrIface::PortStatus status) {
    std::lock_guard<std::mutex> lock(cnt_mutex);
    cb_counts[status]++;
  }

  void register_callback() {
    port_cb = std::bind(&PacketInDevMgrPortStatusTest::port_status, this,
                        std::placeholders::_1, std::placeholders::_2);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_ADDED, port_cb);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_REMOVED, port_cb);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_UP, port_cb);
    sw.register_status_cb(DevMgrIface::PortStatus::PORT_DOWN, port_cb);
  }

  void reset_counts() {
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

struct PMActive { };
struct PMPassive { };

// testing the port status nanomsg notifications
template <typename PMType>
class PortMonitorTest : public ::testing::Test {
 protected:
  typedef DevMgrIface::port_t port_t;
  typedef DevMgrIface::PortStatus PortStatus;

  static constexpr int device_id = 0;

  PortMonitorTest()
      : notifications_writer(new MemoryAccessor(4096)) {
    make_monitor();
  }

  virtual void SetUp() {
    auto status_fn = std::bind(&PortMonitorTest::port_is_up,
                               this, std::placeholders::_1);
    p_monitor->start(status_fn);
  }

  // virtual void TearDown() { }

  void make_monitor();

  bool port_is_up(port_t port);

  void set_port_status(port_t port, PortStatus status);

  const PortMonitorIface::msg_hdr_t *get_msg_hdr(const char *buffer) const {
    return reinterpret_cast<const PortMonitorIface::msg_hdr_t *>(buffer);
  }

  const PortMonitorIface::one_status_t *get_statuses(const char *buffer) const {
    return reinterpret_cast<const PortMonitorIface::one_status_t *>(
        buffer + sizeof(PortMonitorIface::msg_hdr_t));
  }

  std::unordered_map<port_t, PortStatus> port_status{};
  mutable std::mutex status_mutex{};
  std::unique_ptr<PortMonitorIface> p_monitor{nullptr};
  std::shared_ptr<MemoryAccessor> notifications_writer{nullptr};
};

template <typename PMType>
constexpr int PortMonitorTest<PMType>::device_id;

template<>
void
PortMonitorTest<PMPassive>::make_monitor() {
  p_monitor = PortMonitorIface::make_passive(device_id, notifications_writer);
}

template<>
void
PortMonitorTest<PMActive>::make_monitor() {
  p_monitor = PortMonitorIface::make_active(device_id, notifications_writer);
}

template<>
bool
PortMonitorTest<PMPassive>::port_is_up(port_t port) {
  assert(0);
  return false;
}

template<>
bool
PortMonitorTest<PMActive>::port_is_up(port_t port) {
  std::lock_guard<std::mutex> lock(status_mutex);
  return (port_status.at(port) == PortStatus::PORT_UP);
}

template<>
void
PortMonitorTest<PMPassive>::set_port_status(port_t port, PortStatus status) {
  p_monitor->notify(port, status);
}

template<>
void
PortMonitorTest<PMActive>::set_port_status(port_t port, PortStatus status) {
  if (status == PortStatus::PORT_ADDED || status == PortStatus::PORT_UP) {
    std::lock_guard<std::mutex> lock(status_mutex);
    port_status[port] = PortStatus::PORT_UP;
  }
  if (status == PortStatus::PORT_REMOVED || status == PortStatus::PORT_DOWN) {
    std::lock_guard<std::mutex> lock(status_mutex);
    port_status[port] = PortStatus::PORT_DOWN;
  }
  if (status == PortStatus::PORT_ADDED || status == PortStatus::PORT_REMOVED) {
    p_monitor->notify(port, status);
  }
}

typedef Types<PMPassive, PMActive> PMTypes;

TYPED_TEST_CASE(PortMonitorTest, PMTypes);

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

TYPED_TEST(PortMonitorTest, Basic) {
  using PortStatus = DevMgrIface::PortStatus;
  using port_t = DevMgrIface::port_t;
  using Status = MemoryAccessor::Status;
  const port_t port = 1;

  char buffer[128];

  this->set_port_status(port, PortStatus::PORT_ADDED);
  this->set_port_status(port, PortStatus::PORT_UP);
  this->notifications_writer->read(buffer, sizeof(buffer));
  auto msg_hdr = this->get_msg_hdr(buffer);
  auto statuses = this->get_statuses(buffer);
  ASSERT_EQ(0, memcmp("PRT|", msg_hdr->sub_topic, sizeof(msg_hdr->sub_topic)));
  ASSERT_EQ(this->device_id, msg_hdr->switch_id);
  ASSERT_EQ(1u, msg_hdr->num_statuses);
  ASSERT_EQ(port, statuses[0].port);
  ASSERT_EQ(1, statuses[0].status);

  this->set_port_status(port, PortStatus::PORT_UP);
  sleep_for(milliseconds(300));
  ASSERT_NE(Status::CAN_READ, this->notifications_writer->check_status());

  this->set_port_status(port, PortStatus::PORT_DOWN);
  this->notifications_writer->read(buffer, sizeof(buffer));
  msg_hdr = this->get_msg_hdr(buffer);
  statuses = this->get_statuses(buffer);
  ASSERT_EQ(0, memcmp("PRT|", msg_hdr->sub_topic, sizeof(msg_hdr->sub_topic)));
  ASSERT_EQ(this->device_id, msg_hdr->switch_id);
  ASSERT_EQ(1u, msg_hdr->num_statuses);
  ASSERT_EQ(port, statuses[0].port);
  ASSERT_EQ(0, statuses[0].status);
}
