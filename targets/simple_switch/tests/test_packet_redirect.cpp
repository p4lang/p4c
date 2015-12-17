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

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include <nanomsg/pubsub.h>

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <condition_variable>
#include <algorithm>  // for copy

#include <iostream>

#include "bm_apps/packet_pipe.h"

#include "simple_switch.h"

namespace fs = boost::filesystem;

namespace {

class NNEventListener {
 public:
  enum NNEventType {
    TABLE_HIT = 12,
    TABLE_MISS = 13,
    ACTION_EXECUTE = 14
  };

  struct NNEvent {
    NNEventType type;
    int id;
  };

  explicit NNEventListener(const std::string &addr)
      : addr(addr), s(AF_SP, NN_SUB) {
    s.connect(addr.c_str());
    int rcv_timeout_ms = 100;
    s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO,
                 &rcv_timeout_ms, sizeof(rcv_timeout_ms));
    s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  }

  ~NNEventListener() {
    {
      std::unique_lock<std::mutex> lock(mutex);
      if (!started) return;
      stop_receive_thread = true;
    }
    receive_thread.join();
  }

  void start() {
    if (started || stop_receive_thread)
      return;
    receive_thread = std::thread(&NNEventListener::receive_loop, this);
    started = true;
  }

  void get_and_remove_events(const std::string &pid,
                             std::vector<NNEvent> *pevents,
                             size_t num_events,
                             unsigned int timeout_ms = 1000) {
    typedef std::chrono::system_clock clock;
    clock::time_point tp_start = clock::now();
    clock::time_point tp_end = tp_start + std::chrono::milliseconds(timeout_ms);
    std::unique_lock<std::mutex> lock(mutex);
    while (true) {
      if (clock::now() > tp_end) {
        *pevents = {};
        return;
      }
      auto it = events.find(pid);
      if (it == events.end() || it->second.size() < num_events) {
        cond_new_event.wait_until(lock, tp_end);
      } else {
        *pevents = it->second;
        events.erase(it);
        return;
      }
    }
  }

 private:
  void receive_loop();

  std::string addr{};
  nn::socket s;
  std::unordered_map<std::string, std::vector<NNEvent> > events;

  std::thread receive_thread{};
  bool stop_receive_thread{false};
  bool started{false};
  mutable std::mutex mutex{};
  mutable std::condition_variable cond_new_event{};
};

void
NNEventListener::receive_loop() {
  struct msg_hdr_t {
    int type;
    int switch_id;
    uint64_t sig;
    uint64_t id;
    uint64_t copy_id;
  } __attribute__((packed));

  while (true) {
    std::aligned_storage<128>::type storage;
    msg_hdr_t *msg_hdr = reinterpret_cast<msg_hdr_t *>(&storage);
    char *buf = reinterpret_cast<char *>(&storage);

    if (s.recv(buf, sizeof(storage), 0) <= 0) {
      std::unique_lock<std::mutex> lock(mutex);
      if (stop_receive_thread) return;
      continue;
    }

    int object_id;

    switch (msg_hdr->type) {
      case TABLE_HIT:
        {
          struct msg_t : msg_hdr_t {
            int table_id;
            int entry_hdl;
          } __attribute__((packed));
          msg_t *msg = reinterpret_cast<msg_t *>(&storage);
          object_id = msg->table_id;
        }
        break;
      case TABLE_MISS:
        {
          struct msg_t : msg_hdr_t {
            int table_id;
          } __attribute__((packed));
          msg_t *msg = reinterpret_cast<msg_t *>(&storage);
          object_id = msg->table_id;
        }
        break;
      case ACTION_EXECUTE:
        {
          struct msg_t : msg_hdr_t {
            int action_id;
          } __attribute__((packed));
          msg_t *msg = reinterpret_cast<msg_t *>(&storage);
          object_id = msg->action_id;
        }
        break;
      default:
        continue;
    }

    std::string pid =
        std::to_string(msg_hdr->id) + "." + std::to_string(msg_hdr->copy_id);

    std::unique_lock<std::mutex> lock(mutex);
    events[pid].push_back({static_cast<NNEventType>(msg_hdr->type), object_id});
    cond_new_event.notify_all();
  }
}

class PacketInReceiver {
 public:
  enum class Status { CAN_READ, CAN_RECEIVE };

  explicit PacketInReceiver(size_t max_size)
      : max_size(max_size) {
    buffer_.reserve(max_size);
  }

  void receive(int port_num, const char *buffer, int len, void *cookie) {
    (void) cookie;
    if (len > max_size) return;
    std::unique_lock<std::mutex> lock(mutex);
    while (status != Status::CAN_RECEIVE) {
      can_receive.wait(lock);
    }
    buffer_.insert(buffer_.end(), buffer, buffer + len);
    port = port_num;
    status = Status::CAN_READ;
    can_read.notify_one();
  }

  void read(char *dst, size_t len, int *recv_port) {
    len = (len > max_size) ? max_size : len;
    std::unique_lock<std::mutex> lock(mutex);
    while (status != Status::CAN_READ) {
      can_read.wait(lock);
    }
    std::copy(buffer_.begin(), buffer_.begin() + len, dst);
    buffer_.clear();
    *recv_port = port;
    status = Status::CAN_RECEIVE;
    can_receive.notify_one();
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

void
packet_handler(int port_num, const char *buffer, int len, void *cookie) {
  static_cast<SimpleSwitch *>(cookie)->receive(port_num, buffer, len);
}

}  // namespace

class SimpleSwitch_PacketRedirectP4 : public ::testing::Test {
 protected:
  static constexpr size_t kMaxBufSize = 512;

  SimpleSwitch_PacketRedirectP4()
      : packet_inject(packet_in_addr),
        events(event_logger_addr) { }

  // Per-test-case set-up.
  // We make the switch a shared resource for all tests. This is mainly because
  // the simple_switch target detaches threads
  static void SetUpTestCase() {
    // Logger::set_logger_console();
    // TODO(antonin): remove when event-logger cleaned-up
    delete event_logger;
    event_logger = new EventLogger(
        TransportIface::create_instance<TransportNanomsg>(event_logger_addr));

    test_switch = new SimpleSwitch(8);  // 8 ports

    // load JSON
    fs::path json_path = fs::path(testdata_dir) / fs::path(test_json);
    test_switch->init_objects(json_path.string());

    // packet in - packet out
    test_switch->set_dev_mgr_packet_in(packet_in_addr);
    test_switch->Switch::start();  // there is a start member in SimpleSwitch
    test_switch->set_packet_handler(packet_handler,
                                    static_cast<void *>(test_switch));
    test_switch->start_and_return();
  }

  // Per-test-case tear-down.
  static void TearDownTestCase() {
    Packet::unset_phv_factory();
    delete event_logger;
    event_logger = nullptr;
    delete test_switch;
  }

  virtual void SetUp() {
    // TODO(antonin): a lot of manual work here, can I add some of it?

    packet_inject.start();
    auto cb = std::bind(&PacketInReceiver::receive, &receiver,
                          std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4);
    packet_inject.set_packet_receiver(cb, nullptr);

    events.start();

    // default actions for all tables
    test_switch->mt_set_default_action("t_ingress_1", "_nop", ActionData());
    test_switch->mt_set_default_action("t_ingress_2", "_nop", ActionData());
    test_switch->mt_set_default_action("t_egress", "_nop", ActionData());
  }

  virtual void TearDown() {
    // kind of experimental, so reserved for testing
    test_switch->reset_state();
  }

  bool check_event_table_hit(const NNEventListener::NNEvent &event,
                             const std::string &name) {
    return (event.type == NNEventListener::TABLE_HIT) &&
        (event.id == test_switch->get_table_id(name));
  }

  bool check_event_table_miss(const NNEventListener::NNEvent &event,
                              const std::string &name) {
    return (event.type == NNEventListener::TABLE_MISS) &&
        (event.id == test_switch->get_table_id(name));
  }

  bool check_event_action_execute(const NNEventListener::NNEvent &event,
                                  const std::string &name) {
    return (event.type == NNEventListener::ACTION_EXECUTE) &&
        (event.id == test_switch->get_action_id(name));
  }

 protected:
  static const std::string event_logger_addr;
  static const std::string packet_in_addr;
  static SimpleSwitch *test_switch;
  bm_apps::PacketInject packet_inject;
  PacketInReceiver receiver{kMaxBufSize};
  NNEventListener events;

 private:
  static const std::string testdata_dir;
  static const std::string test_json;
};

const std::string SimpleSwitch_PacketRedirectP4::event_logger_addr =
    "ipc:///tmp/test_events_abc123";
const std::string SimpleSwitch_PacketRedirectP4::packet_in_addr =
    "ipc:///tmp/test_packet_in_abc123";

SimpleSwitch *SimpleSwitch_PacketRedirectP4::test_switch = nullptr;

const std::string SimpleSwitch_PacketRedirectP4::testdata_dir = "testdata";
const std::string SimpleSwitch_PacketRedirectP4::test_json =
    "packet_redirect.json";

TEST_F(SimpleSwitch_PacketRedirectP4, Baseline) {
  static constexpr int port_in = 1;
  static constexpr int port_out = 2;

  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01"));
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00", 1));
  ActionData data;
  data.push_back_action_data(port_out);
  entry_handle_t handle;
  MatchErrorCode rc = test_switch->mt_add_entry("t_ingress_1", match_key,
                                                "_set_port", std::move(data),
                                                &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  const char pkt[] = {'\x01', '\x00'};
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kMaxBufSize];
  int recv_port = -1;
  receiver.read(recv_buffer, sizeof(pkt), &recv_port);
  ASSERT_EQ(port_out, recv_port);

  // event check
  std::vector<NNEventListener::NNEvent> pevents;
  events.get_and_remove_events("0.0", &pevents, 6u);
  ASSERT_EQ(6u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_miss(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_nop"));
  ASSERT_TRUE(check_event_table_miss(pevents[4], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[5], "_nop"));
}

TEST_F(SimpleSwitch_PacketRedirectP4, Multicast) {
  static constexpr int port_in = 1;
  static constexpr int mgrp = 1;

  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x02"));
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00", 1));
  ActionData data;
  data.push_back_action_data(mgrp);
  entry_handle_t handle;
  MatchErrorCode rc = test_switch->mt_add_entry("t_ingress_1", match_key,
                                                "_multicast", std::move(data),
                                                &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  auto pre_ptr = test_switch->get_component<McSimplePreLAG>();
  McSimplePreLAG::mgrp_hdl_t mgrp_hdl;
  ASSERT_EQ(McSimplePreLAG::SUCCESS, pre_ptr->mc_mgrp_create(mgrp, &mgrp_hdl));
  McSimplePreLAG::l1_hdl_t node_1, node_2;
  {
    std::string port_map = "10";
    std::string lag_map = "";
    ASSERT_EQ(McSimplePreLAG::SUCCESS,
              pre_ptr->mc_node_create(
                  1, McSimplePreLAG::PortMap(port_map),
                  McSimplePreLAG::LagMap(lag_map), &node_1));
    ASSERT_EQ(McSimplePreLAG::SUCCESS,
              pre_ptr->mc_node_associate(mgrp_hdl, node_1));
  }
  {
    std::string port_map = "100";
    std::string lag_map = "";
    ASSERT_EQ(McSimplePreLAG::SUCCESS,
              pre_ptr->mc_node_create(
                  2, McSimplePreLAG::PortMap(port_map),
                  McSimplePreLAG::LagMap(lag_map), &node_2));
    ASSERT_EQ(McSimplePreLAG::SUCCESS,
              pre_ptr->mc_node_associate(mgrp_hdl, node_2));
  }

  const char pkt[] = {'\x02', '\x00'};
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kMaxBufSize];
  int recv_port_1 = -1;
  int recv_port_2 = -1;
  receiver.read(recv_buffer, sizeof(pkt), &recv_port_1);
  receiver.read(recv_buffer, sizeof(pkt), &recv_port_2);
  ASSERT_TRUE((recv_port_1 == 1 && recv_port_2 == 2) ||
              (recv_port_1 == 2 && recv_port_2 == 1));

  // event check
  std::vector<NNEventListener::NNEvent> pevents;

  events.get_and_remove_events("1.0", &pevents, 4u);
  ASSERT_EQ(4u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_multicast"));
  ASSERT_TRUE(check_event_table_miss(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_nop"));

  events.get_and_remove_events("1.1", &pevents, 2u);
  ASSERT_EQ(2u, pevents.size());
  ASSERT_TRUE(check_event_table_miss(pevents[0], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_nop"));

  events.get_and_remove_events("1.2", &pevents, 2u);
  ASSERT_EQ(2u, pevents.size());
  ASSERT_TRUE(check_event_table_miss(pevents[0], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_nop"));

  // reset PRE
  ASSERT_EQ(McSimplePreLAG::SUCCESS, pre_ptr->mc_node_destroy(node_1));
  ASSERT_EQ(McSimplePreLAG::SUCCESS, pre_ptr->mc_node_destroy(node_2));
  ASSERT_EQ(McSimplePreLAG::SUCCESS, pre_ptr->mc_mgrp_destroy(mgrp_hdl));
}

TEST_F(SimpleSwitch_PacketRedirectP4, CloneI2E) {
  static constexpr int port_in = 1;
  static constexpr int port_out = 2;
  static constexpr int port_out_copy = 3;
  static constexpr int mirror_id = 1;

  std::vector<MatchKeyParam> match_key_1;
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x03"));
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00", 1));
  ActionData data_1;
  data_1.push_back_action_data(port_out);
  entry_handle_t h_1;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_1", match_key_1,
                                      "_set_port", std::move(data_1), &h_1));

  std::vector<MatchKeyParam> match_key_2;
  match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x03"));
  match_key_2.emplace_back(MatchKeyParam::Type::TERNARY,
                           std::string(4, '\x00'), std::string(4, '\x00'));
  ActionData data_2;
  data_2.push_back_action_data(mirror_id);
  entry_handle_t h_2;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_2", match_key_2, "_clone_i2e",
                                      std::move(data_2), &h_2, 1));

  test_switch->mirroring_mapping_add(mirror_id, port_out_copy);

  const char pkt[] = {'\x03', '\x00'};
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kMaxBufSize];
  int recv_port_1 = -1;
  int recv_port_2 = -1;
  receiver.read(recv_buffer, sizeof(pkt), &recv_port_1);
  receiver.read(recv_buffer, sizeof(pkt), &recv_port_2);
  // TODO(antonin): make sure the right packet comes out of the right port
  ASSERT_TRUE((recv_port_1 == port_out && recv_port_2 == port_out_copy) ||
              (recv_port_1 == port_out_copy && recv_port_2 == port_out));

  test_switch->mirroring_mapping_delete(mirror_id);

  // event check
  std::vector<NNEventListener::NNEvent> pevents;

  events.get_and_remove_events("2.0", &pevents, 6u);
  ASSERT_EQ(6u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_hit(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_clone_i2e"));
  ASSERT_TRUE(check_event_table_miss(pevents[4], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[5], "_nop"));

  events.get_and_remove_events("2.1", &pevents, 2u);
  ASSERT_EQ(2u, pevents.size());
  ASSERT_TRUE(check_event_table_miss(pevents[0], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_nop"));
}

TEST_F(SimpleSwitch_PacketRedirectP4, CloneE2E) {
  static constexpr int port_in = 1;
  static constexpr int port_out = 2;
  static constexpr int port_out_copy = 3;
  static constexpr int mirror_id = 1;

  std::vector<MatchKeyParam> match_key_1;
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x04"));
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00", 1));
  ActionData data_1;
  data_1.push_back_action_data(port_out);
  entry_handle_t h_1;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_1", match_key_1,
                                      "_set_port", std::move(data_1), &h_1));

  std::vector<MatchKeyParam> match_key_2;
  match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x04"));
  // only PKT_INSTANCE_TYPE_NORMAL (= 0)
  match_key_2.emplace_back(MatchKeyParam::Type::TERNARY,
                           std::string(4, '\x00'), std::string(4, '\xff'));
  ActionData data_2;
  data_2.push_back_action_data(mirror_id);
  entry_handle_t h_2;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_egress", match_key_2, "_clone_e2e",
                                      std::move(data_2), &h_2, 1));

  test_switch->mirroring_mapping_add(mirror_id, port_out_copy);

  const char pkt[] = {'\x04', '\x00'};
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kMaxBufSize];
  int recv_port_1 = -1;
  int recv_port_2 = -1;
  receiver.read(recv_buffer, sizeof(pkt), &recv_port_1);
  receiver.read(recv_buffer, sizeof(pkt), &recv_port_2);
  // TODO(antonin): make sure the right packet comes out of the right port
  ASSERT_TRUE((recv_port_1 == port_out && recv_port_2 == port_out_copy) ||
              (recv_port_1 == port_out_copy && recv_port_2 == port_out));

  test_switch->mirroring_mapping_delete(mirror_id);

  // event check
  std::vector<NNEventListener::NNEvent> pevents;

  events.get_and_remove_events("3.0", &pevents, 6u);
  ASSERT_EQ(6u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_miss(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_nop"));
  ASSERT_TRUE(check_event_table_hit(pevents[4], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[5], "_clone_e2e"));

  events.get_and_remove_events("3.1", &pevents, 2u);
  ASSERT_EQ(2u, pevents.size());
  ASSERT_TRUE(check_event_table_miss(pevents[0], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_nop"));
}

TEST_F(SimpleSwitch_PacketRedirectP4, Resubmit) {
  /* In this test, the egress port is first set to 2, but because the packet is
     selected for resubmission, and because of the resubmitted metadata, the
     egress port will be set to 3 */
  static constexpr int port_in = 1;
  static constexpr int port_out_1 = 2;
  static constexpr int port_out_2 = 3;

  std::vector<MatchKeyParam> match_key_1;
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x05"));
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00", 1));
  ActionData data_1;
  data_1.push_back_action_data(port_out_1);
  entry_handle_t h_1;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_1", match_key_1,
                                      "_set_port", std::move(data_1), &h_1));

  std::vector<MatchKeyParam> match_key_2;
  match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x05"));
  match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01", 1));
  ActionData data_2;
  data_2.push_back_action_data(port_out_2);
  entry_handle_t h_2;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_1", match_key_2,
                                      "_set_port", std::move(data_2), &h_2));

  std::vector<MatchKeyParam> match_key_3;
  match_key_3.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x05"));
  // only PKT_INSTANCE_TYPE_NORMAL (= 0)
  match_key_3.emplace_back(MatchKeyParam::Type::TERNARY,
                           std::string(4, '\x00'), std::string(4, '\xff'));
  ActionData data_3;
  entry_handle_t h_3;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_2", match_key_3, "_resubmit",
                                      std::move(data_3), &h_3, 1));

  const char pkt[] = {'\x05', '\x00'};
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kMaxBufSize];
  int recv_port = -1;
  receiver.read(recv_buffer, sizeof(pkt), &recv_port);
  ASSERT_EQ(port_out_2, recv_port);

  // event check
  std::vector<NNEventListener::NNEvent> pevents;

  events.get_and_remove_events("4.0", &pevents, 4u);
  ASSERT_EQ(4u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_hit(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_resubmit"));

  // TODO(antonin): if we consider that it is the same packet, then the copy_id
  // should be the same? Update this if this changes in simple_switch
  events.get_and_remove_events("4.1", &pevents, 6u);
  ASSERT_EQ(6u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_miss(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_nop"));
  ASSERT_TRUE(check_event_table_miss(pevents[4], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[5], "_nop"));
}

TEST_F(SimpleSwitch_PacketRedirectP4, Recirculate) {
  static constexpr int port_in = 1;
  static constexpr int port_out_1 = 2;
  static constexpr int port_out_2 = 3;

  std::vector<MatchKeyParam> match_key_1;
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x06"));
  match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00", 1));
  ActionData data_1;
  data_1.push_back_action_data(port_out_1);
  entry_handle_t h_1;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_1", match_key_1,
                                      "_set_port", std::move(data_1), &h_1));

  std::vector<MatchKeyParam> match_key_2;
  match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x06"));
  match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01", 1));
  ActionData data_2;
  data_2.push_back_action_data(port_out_2);
  entry_handle_t h_2;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_ingress_1", match_key_2,
                                      "_set_port", std::move(data_2), &h_2));

  std::vector<MatchKeyParam> match_key_3;
  match_key_3.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x06"));
  // only PKT_INSTANCE_TYPE_NORMAL (= 0)
  match_key_3.emplace_back(MatchKeyParam::Type::TERNARY,
                           std::string(4, '\x00'), std::string(4, '\xff'));
  ActionData data_3;
  entry_handle_t h_3;
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            test_switch->mt_add_entry("t_egress", match_key_3, "_recirculate",
                                      std::move(data_3), &h_3, 1));

  const char pkt[] = {'\x06', '\x00'};
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kMaxBufSize];
  int recv_port = -1;
  receiver.read(recv_buffer, sizeof(pkt), &recv_port);
  ASSERT_EQ(port_out_2, recv_port);

  // event check
  std::vector<NNEventListener::NNEvent> pevents;

  events.get_and_remove_events("5.0", &pevents, 6u);
  ASSERT_EQ(6u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_miss(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_nop"));
  ASSERT_TRUE(check_event_table_hit(pevents[4], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[5], "_recirculate"));

  // TODO(antonin): if we consider that it is the same packet, then the copy_id
  // should be the same? Update this if this changes in simple_switch
  events.get_and_remove_events("5.1", &pevents, 6u);
  ASSERT_EQ(6u, pevents.size());
  ASSERT_TRUE(check_event_table_hit(pevents[0], "t_ingress_1"));
  ASSERT_TRUE(check_event_action_execute(pevents[1], "_set_port"));
  ASSERT_TRUE(check_event_table_miss(pevents[2], "t_ingress_2"));
  ASSERT_TRUE(check_event_action_execute(pevents[3], "_nop"));
  ASSERT_TRUE(check_event_table_miss(pevents[4], "t_egress"));
  ASSERT_TRUE(check_event_action_execute(pevents[5], "_nop"));
}
