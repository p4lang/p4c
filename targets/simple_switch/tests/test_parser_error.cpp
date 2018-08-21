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

class SimpleSwitch_ParserErrorP4 : public ::testing::Test {
 protected:
  static constexpr size_t kMaxBufSize = 512;

  static constexpr bm::device_id_t device_id{0};

  SimpleSwitch_ParserErrorP4()
      : packet_inject(packet_in_addr) { }

  // Per-test-case set-up.
  // We make the switch a shared resource for all tests. This is mainly because
  // the simple_switch target detaches threads
  static void SetUpTestCase() {
    // bm::Logger::set_logger_console();

    test_switch = new SimpleSwitch(8);  // 8 ports

    fs::path json_path = fs::path(testdata_dir) / fs::path(test_json);
    test_switch->init_objects(json_path.string());

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

 protected:
  static const std::string packet_in_addr;
  static SimpleSwitch *test_switch;
  bm_apps::PacketInject packet_inject;
  PacketInReceiver receiver{};

 private:
  static const std::string testdata_dir;
  static const std::string test_json;
};

const std::string SimpleSwitch_ParserErrorP4::packet_in_addr =
    "inproc://packets";

SimpleSwitch *SimpleSwitch_ParserErrorP4::test_switch = nullptr;

const std::string SimpleSwitch_ParserErrorP4::testdata_dir = TESTDATADIR;
const std::string SimpleSwitch_ParserErrorP4::test_json = "parser_error.json";

TEST_F(SimpleSwitch_ParserErrorP4, NoError) {
  static constexpr int port = 1;
  int recv_port;
  char recv_buffer[kMaxBufSize];
  const char pkt[] = {'\x00', '\x00', '\x00', '\x00'};
  packet_inject.send(port, pkt, sizeof(pkt));
  EXPECT_EQ(4u, receiver.read(recv_buffer, kMaxBufSize, &recv_port));
  EXPECT_EQ(port, recv_port);
  EXPECT_EQ(0, static_cast<int>(recv_buffer[3]));
}

TEST_F(SimpleSwitch_ParserErrorP4, PacketTooShort) {
  static constexpr int port = 1;
  int recv_port;
  char recv_buffer[kMaxBufSize];
  const char pkt[] = {'\x00', '\x00'};
  packet_inject.send(port, pkt, sizeof(pkt));
  // why 6? because the 2 bytes of pkt are never extracted because they are too
  // short and are therefore considered payload bytes. In ingress, we make the
  // header valid, therefore prepending 4 bytes to the packet.
  EXPECT_EQ(6u, receiver.read(recv_buffer, kMaxBufSize, &recv_port));
  EXPECT_EQ(port, recv_port);
  EXPECT_EQ(1, static_cast<int>(recv_buffer[3]));
}

TEST_F(SimpleSwitch_ParserErrorP4, CustomError) {
  static constexpr int port = 1;
  int recv_port;
  char recv_buffer[kMaxBufSize];
  const char pkt[] = {'\x00', '\x00', '\x00', '\xab'};
  packet_inject.send(port, pkt, sizeof(pkt));
  EXPECT_EQ(4u, receiver.read(recv_buffer, kMaxBufSize, &recv_port));
  EXPECT_EQ(port, recv_port);
  EXPECT_EQ(2, static_cast<int>(recv_buffer[3]));
}
