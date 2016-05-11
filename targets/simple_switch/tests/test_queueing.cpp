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
#include <algorithm>  // for std::is_sorted

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

void
read_packet_field(char *dst, const char *src, size_t s) {
  for (size_t i = 0; i < s; i++)
    dst[i] = src[s - 1 - i];
}

}  // namespace

class SimpleSwitch_QueueingP4 : public ::testing::Test {
 protected:
  static constexpr size_t kQueueingHdrSize = (48u + 24u + 32u + 24u) / 8u;

  static constexpr int device_id{0};

  SimpleSwitch_QueueingP4()
      : packet_inject(packet_in_addr) { }

  // Per-test-case set-up.
  // We make the switch a shared resource for all tests. This is mainly because
  // the simple_switch target detaches threads
  static void SetUpTestCase() {
    // bm::Logger::set_logger_console();

    test_switch = new SimpleSwitch(8);  // 8 ports

    // load JSON
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

    // default actions for all tables
    test_switch->mt_set_default_action(0, "t_ingress", "_drop", ActionData());
    test_switch->mt_set_default_action(0, "t_egress",
                                       "copy_queueing_data", ActionData());
  }

  virtual void TearDown() {
    // kind of experimental, so reserved for testing
    test_switch->reset_state();
  }

  void get_queueing_data_from_pkt(const char *pkt,
                                  uint64_t *enq_timestamp,
                                  uint32_t *enq_qdepth,
                                  uint32_t *deq_timedelta,
                                  uint32_t *deq_qdepth) const {
    const char *queueing_hdr = pkt + 2;  // 2 is size of hdr1
    char *enq_timestamp_ = reinterpret_cast<char *>(enq_timestamp);
    char *enq_qdepth_ = reinterpret_cast<char *>(enq_qdepth);
    char *deq_timedelta_ = reinterpret_cast<char *>(deq_timedelta);
    char *deq_qdepth_ = reinterpret_cast<char *>(deq_qdepth);

    *enq_timestamp = 0u;
    read_packet_field(enq_timestamp_, queueing_hdr, 6);
    *enq_qdepth = 0u;
    read_packet_field(enq_qdepth_, queueing_hdr + 6, 3);
    *deq_timedelta = 0u;
    read_packet_field(deq_timedelta_, queueing_hdr + 9, 4);
    *deq_qdepth = 0u;
    read_packet_field(deq_qdepth_, queueing_hdr + 13, 3);
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

const std::string SimpleSwitch_QueueingP4::packet_in_addr =
    "inproc://packets";

SimpleSwitch *SimpleSwitch_QueueingP4::test_switch = nullptr;

const std::string SimpleSwitch_QueueingP4::testdata_dir = TESTDATADIR;
const std::string SimpleSwitch_QueueingP4::test_json =
    "queueing.json";

constexpr size_t SimpleSwitch_QueueingP4::kQueueingHdrSize;

TEST_F(SimpleSwitch_QueueingP4, QueueingBasic) {
  static constexpr int port_in = 1;
  static constexpr int port_out = 2;
  static constexpr size_t kPktSizeIn = 256u;
  static constexpr size_t kPktSizeOut = 256u + kQueueingHdrSize;

  ActionData action_data;
  action_data.push_back_action_data(port_out);
  test_switch->mt_set_default_action(0, "t_ingress", "set_port",
                                     std::move(action_data));

  char pkt[kPktSizeIn];
  packet_inject.send(port_in, pkt, sizeof(pkt));
  char recv_buffer[kPktSizeOut];
  int recv_port = -1;
  size_t recv_size = receiver.read(recv_buffer, sizeof(recv_buffer),
                                   &recv_port);
  ASSERT_EQ(port_out, recv_port);
  ASSERT_EQ(sizeof(recv_buffer), recv_size);

  uint64_t enq_timestamp;
  uint32_t enq_qdepth;
  uint32_t deq_timedelta;
  uint32_t deq_qdepth;
  get_queueing_data_from_pkt(recv_buffer, &enq_timestamp, &enq_qdepth,
                             &deq_timedelta, &deq_qdepth);
  // std::cout << enq_timestamp << " " << enq_qdepth << " "
  //           << deq_timedelta << " " << deq_qdepth << "\n";
  ASSERT_EQ(0u, enq_qdepth);
  ASSERT_EQ(0u, deq_qdepth);
  // no reliable tests I can do for these
  (void) enq_timestamp;
  (void) deq_timedelta;
}

TEST_F(SimpleSwitch_QueueingP4, QueueingAdvanced) {
  static constexpr int port_in = 1;
  static constexpr int port_out = 2;
  static constexpr size_t kPktSizeIn = 256u;
  static constexpr size_t kPktSizeOut = 256u + kQueueingHdrSize;

  ActionData action_data;
  action_data.push_back_action_data(port_out);
  test_switch->mt_set_default_action(0, "t_ingress", "set_port",
                                     std::move(action_data));

  char pkt[kPktSizeIn];
  char recv_buffer[kPktSizeOut];
  int recv_port = -1;

  // setting egress queue rate to 1 pps
  test_switch->set_all_egress_queue_rates(1u);

  const size_t nb_packets = 16u;
  for (size_t i = 0; i < nb_packets; i++) {
    packet_inject.send(port_in, pkt, sizeof(pkt));
  }

  std::vector<uint64_t> enq_timestamps(nb_packets);
  std::vector<uint32_t> enq_qdepths(nb_packets);
  std::vector<uint32_t> deq_timedeltas(nb_packets);
  std::vector<uint32_t> deq_qdepths(nb_packets);

  for (size_t i = 0; i < nb_packets; i++) {
    size_t recv_size = receiver.read(recv_buffer, sizeof(recv_buffer),
                                     &recv_port);
    ASSERT_EQ(port_out, recv_port);
    ASSERT_EQ(sizeof(recv_buffer), recv_size);

    get_queueing_data_from_pkt(recv_buffer, &enq_timestamps[i], &enq_qdepths[i],
                               &deq_timedeltas[i], &deq_qdepths[i]);
  }

  const uint64_t one_sec_in_usecs = 1000000;

  ASSERT_TRUE(std::is_sorted(enq_timestamps.begin(), enq_timestamps.end()));

  // there should not be too much time between the first enqueue and the last
  ASSERT_GT(one_sec_in_usecs, enq_timestamps.back() - enq_timestamps.front());

  // when first packet enqueued, queue is empty
  ASSERT_EQ(0, enq_qdepths.front());
  // when last packet is enqueued, almost all packets are in the queue still
  ASSERT_LT(nb_packets - 2, enq_qdepths.back());

  ASSERT_TRUE(std::is_sorted(deq_timedeltas.begin(), deq_timedeltas.end()));
  // first packet is dequeued quickly
  ASSERT_GT(2 * one_sec_in_usecs, deq_timedeltas.front());
  // last packet spends a long time in the queue
  ASSERT_LT((nb_packets - 2) * one_sec_in_usecs, deq_timedeltas.back());

  // when first packet is dequeued, almost all packets already in queue
  ASSERT_LT(nb_packets - 2, deq_qdepths.front());
  // when last packet leaves queue, it is empty
  ASSERT_EQ(0, deq_qdepths.back());
}
