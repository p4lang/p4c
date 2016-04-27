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

#include <bm/bm_apps/packet_pipe.h>

#include <boost/filesystem.hpp>

#include <string>
#include <memory>
#include <vector>
#include <algorithm>  // for std::fill_n
#include <fstream>
#include <streambuf>

#include "simple_switch.h"

#include "utils.h"

namespace fs = boost::filesystem;

using bm::MatchErrorCode;
using bm::ActionData;
using bm::MatchKeyParam;
using bm::entry_handle_t;

namespace {

void
packet_handler(int port_num, const char *buffer, int len, void *cookie) {
  static_cast<SimpleSwitch *>(cookie)->receive(port_num, buffer, len);
}

}  // namespace

class SimpleSwitch_SwapP4 : public ::testing::Test {
 protected:
  static constexpr size_t kMaxBufSize = 512;

  static constexpr int device_id{0};

  SimpleSwitch_SwapP4()
      : packet_inject(packet_in_addr) { }

  // Per-test-case set-up.
  // We make the switch a shared resource for all tests. This is mainly because
  // the simple_switch target detaches threads
  static void SetUpTestCase() {
    // bm::Logger::set_logger_console();

    test_switch = new SimpleSwitch(8, true);  // 8 ports, with swapping

    test_switch->init_objects(json_path_1());

    // packet in - packet out
    test_switch->set_dev_mgr_packet_in(device_id, packet_in_addr, nullptr);
    test_switch->Switch::start();  // there is a start member in SimpleSwitch
    test_switch->set_packet_handler(packet_handler,
                                    static_cast<void *>(test_switch));
    test_switch->start_and_return();
  }

  // Per-test-case tear-down.
  static void TearDownTestCase() {
    delete test_switch;
  }

  virtual void SetUp() {
    packet_inject.start();
    auto cb = std::bind(&PacketInReceiver::receive, &receiver,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4);
    packet_inject.set_packet_receiver(cb, nullptr);
  }

  virtual void TearDown() {
    // kind of experimental, so reserved for testing
    test_switch->reset_state();
  }

  static std::string json_path_1() {
    return (fs::path(testdata_dir) / fs::path(test_json_1)).string();
  }
  static std::string json_path_2() {
    return (fs::path(testdata_dir) / fs::path(test_json_2)).string();
  }

  void configure_swap_1() {
    test_switch->mt_set_default_action(0, "t_ingress_1", "a11", ActionData());

    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\xab"));
    entry_handle_t handle;
    MatchErrorCode rc = test_switch->mt_add_entry(
        0, "t_ingress_1", match_key, "a12", ActionData(), &handle);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  }

  void configure_swap_2() {
    test_switch->mt_set_default_action(0, "t_ingress_2", "a21", ActionData());

    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\xcd"));
    entry_handle_t handle;
    MatchErrorCode rc = test_switch->mt_add_entry(
        0, "t_ingress_2", match_key, "a22", ActionData(), &handle);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  }

 protected:
  static const std::string packet_in_addr;
  static SimpleSwitch *test_switch;
  bm_apps::PacketInject packet_inject;
  PacketInReceiver receiver{};

 private:
  static const std::string testdata_dir;
  static const std::string test_json_1;
  static const std::string test_json_2;
};

const std::string SimpleSwitch_SwapP4::packet_in_addr =
    "inproc://packets";

SimpleSwitch *SimpleSwitch_SwapP4::test_switch = nullptr;

const std::string SimpleSwitch_SwapP4::testdata_dir = TESTDATADIR;
const std::string SimpleSwitch_SwapP4::test_json_1 = "swap_1.json";
const std::string SimpleSwitch_SwapP4::test_json_2 = "swap_2.json";

TEST_F(SimpleSwitch_SwapP4, Swap) {
  const int port_in = 0;

  bm::RuntimeInterface::ErrorCode rc;
  const auto rc_success = bm::RuntimeInterface::ErrorCode::SUCCESS;

  auto swap_1_check = [this, port_in]() {
    int recv_port = -1;
    char recv_buffer[2];

    const char pkt_1_miss[2] = {'\x00', '\x00'};
    const char pkt_1_hit[2] = {'\xab', '\x00'};
    const int port_1_miss = 1;
    const int port_1_hit = 2;

    packet_inject.send(port_in, pkt_1_miss, sizeof(pkt_1_miss));
    receiver.read(recv_buffer, sizeof(pkt_1_miss), &recv_port);
    ASSERT_EQ(port_1_miss, recv_port);

    packet_inject.send(port_in, pkt_1_hit, sizeof(pkt_1_hit));
    receiver.read(recv_buffer, sizeof(pkt_1_hit), &recv_port);
    ASSERT_EQ(port_1_hit, recv_port);
  };

  auto swap_2_check = [this, port_in]() {
    int recv_port = -1;
    char recv_buffer[4];

    const char pkt_2_miss[4] = {'\x00', '\x00', '\x00', '\x00'};
    const char pkt_2_hit[4] = {'\xcd', '\x00', '\x00', '\x00'};
    const int port_2_miss = 3;
    const int port_2_hit = 4;

    packet_inject.send(port_in, pkt_2_miss, sizeof(pkt_2_miss));
    receiver.read(recv_buffer, sizeof(pkt_2_miss), &recv_port);
    ASSERT_EQ(port_2_miss, recv_port);

    packet_inject.send(port_in, pkt_2_hit, sizeof(pkt_2_hit));
    receiver.read(recv_buffer, sizeof(pkt_2_hit), &recv_port);
    ASSERT_EQ(port_2_hit, recv_port);
  };

  configure_swap_1();

  swap_1_check();

  // load second JSON
  std::ifstream fs(json_path_2(), std::ios::in);
  rc = test_switch->load_new_config(
      std::string(std::istreambuf_iterator<char>(fs),
                  std::istreambuf_iterator<char>()));
  ASSERT_EQ(rc_success, rc);

  swap_1_check();

  configure_swap_2();

  swap_1_check();

  // order swap
  rc = test_switch->swap_configs();
  ASSERT_EQ(rc_success, rc);

  // SimpleSwitch is going to make the swap here

  swap_2_check();
}
