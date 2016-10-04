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

#include <gtest/gtest.h>

#include <bm/bm_sim/ageing.h>
#include <bm/bm_sim/learning.h>
#include <bm/bm_sim/transport.h>
#include <bm/bm_apps/notifications.h>

#include <string>
#include <memory>
#include <future>
#include <thread>
#include <chrono>

class NotificationsTest : public ::testing::Test {
 protected:
  using NotificationsListener = bm_apps::NotificationsListener;
  using AgeingMsgInfo = NotificationsListener::AgeingMsgInfo;
  using LearnMsgInfo = NotificationsListener::LearnMsgInfo;

  NotificationsTest()
      : transport(bm::TransportIface::make_nanomsg(notifications_addr)),
        notifications(notifications_addr) { }

  void generate_ageing_notif(const AgeingMsgInfo &info) {
    char data[] = {'\xab', '\xcd'};
    using msg_hdr_t = bm::AgeingMonitorIface::msg_hdr_t;
    msg_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.sub_topic, "AGE|", 4);
    hdr.switch_id = info.switch_id;
    hdr.cxt_id = info.cxt_id;
    hdr.buffer_id = info.buffer_id;
    hdr.table_id = info.table_id;
    hdr.num_entries = info.num_entries;
    bm::TransportIface::MsgBuf buf_hdr =
        {reinterpret_cast<char *>(&hdr), sizeof(hdr)};
    bm::TransportIface::MsgBuf buf_data = {data, sizeof(data)};
    transport->send_msgs({buf_hdr, buf_data});
  }

  void generate_learn_notif(const LearnMsgInfo &info) {
    char data[] = {'\xab', '\xcd'};
    using msg_hdr_t = bm::LearnEngineIface::msg_hdr_t;
    msg_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.sub_topic, "LEA|", 4);
    hdr.switch_id = info.switch_id;
    hdr.cxt_id = info.cxt_id;
    hdr.list_id = info.list_id;
    hdr.buffer_id = info.buffer_id;
    hdr.num_samples = info.num_samples;
    bm::TransportIface::MsgBuf buf_hdr =
        {reinterpret_cast<char *>(&hdr), sizeof(hdr)};
    bm::TransportIface::MsgBuf buf_data = {data, sizeof(data)};
    transport->send_msgs({buf_hdr, buf_data});
  }

  bool check_data(const char *data) {
    const char expected_data[] = {'\xab', '\xcd'};
    return memcmp(data, expected_data, sizeof(expected_data)) == 0;
  }

  template <typename T>
  bool compare_(const T &a, const T &b) {
    return (a.switch_id == b.switch_id)
        && (a.cxt_id == b.cxt_id)
        && (a.buffer_id == b.buffer_id);
  }

  bool compare(const AgeingMsgInfo &a, const AgeingMsgInfo &b) {
    return compare_(a, b)
        && (a.table_id == b.table_id)
        && (a.num_entries == b.num_entries);
  }

  bool compare(const LearnMsgInfo &a, const LearnMsgInfo &b) {
    return compare_(a, b)
        && (a.list_id == b.list_id)
        && (a.num_samples == b.num_samples);
  }

  virtual void SetUp() {
    transport->open();
    notifications.start();
  }

  static const std::string notifications_addr;

  std::unique_ptr<bm::TransportIface> transport;
  NotificationsListener notifications;
};

const std::string NotificationsTest::notifications_addr =
    "inproc://notifications";

TEST_F(NotificationsTest, Ageing) {
  struct AgeingData {
    AgeingMsgInfo info;
    const char *data;
  };
  std::promise<AgeingData> promise;
  auto future = promise.get_future();
  // lambda does not capture, so can be used as function pointer
  auto cb = [](const AgeingMsgInfo &info, const char *data, void *cookie) {
    AgeingData adata = {info, data};
    static_cast<decltype(&promise)>(cookie)->set_value(adata);
  };
  notifications.register_ageing_cb(cb, static_cast<void *>(&promise));
  AgeingMsgInfo input_info = {0, 0, 1, 18, 5};
  generate_ageing_notif(input_info);
  future.wait();
  auto received = future.get();
  ASSERT_TRUE(compare(input_info, received.info));
  ASSERT_TRUE(check_data(received.data));
}

// I could try to unify this code by using a type-parametrized GTEST fixture,
// but it would required specializing a few member functions, and I don't think
// it is worth it for 2 notification types... I am not sure it would help with
// readability either.
TEST_F(NotificationsTest, Learn) {
  struct LearnData {
    LearnMsgInfo info;
    const char *data;
  };
  std::promise<LearnData> promise;
  auto future = promise.get_future();
  // lambda does not capture, so can be used as function pointer
  auto cb = [](const LearnMsgInfo &info, const char *data, void *cookie) {
    LearnData adata = {info, data};
    static_cast<decltype(&promise)>(cookie)->set_value(adata);
  };
  notifications.register_learn_cb(cb, static_cast<void *>(&promise));
  LearnMsgInfo input_info = {0, 0, 2, 12, 16};
  generate_learn_notif(input_info);
  future.wait();
  auto received = future.get();
  ASSERT_TRUE(compare(input_info, received.info));
  ASSERT_TRUE(check_data(received.data));
}
