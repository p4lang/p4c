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

#include <bm/bm_sim/learning.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/phv_source.h>
#include <bm/bm_sim/packet.h>

#include <algorithm>  // for std::copy
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>  // for std::pair
#include <vector>

#include <cassert>

#include "utils.h"

using namespace bm;

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;

// Google Test fixture for learning tests
class LearningTest : public ::testing::Test {
 protected:
  using clock = std::chrono::high_resolution_clock;

 protected:
  PHVFactory phv_factory;

  const device_id_t device_id = 0;
  const cxt_id_t cxt_id = 0;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  std::shared_ptr<MemoryAccessor> learn_writer;
  char buffer[4096];

  // used exclusively for callback mode
  LearnEngineIface::msg_hdr_t cb_hdr;
  bool cb_written;
  mutable std::mutex cb_written_mutex;
  mutable std::condition_variable cb_written_cv;

  std::unique_ptr<LearnEngineIface> learn_engine;

  clock::time_point start;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  LearningTest()
      : testHeaderType("test_t", 0),
        learn_writer(new MemoryAccessor(4096)),
        learn_engine(LearnEngineIface::make()),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    start = clock::now();
  }

  // virtual void TearDown() { }

  void learn_on_test1_f16(LearnEngineIface::list_id_t list_id,
                          size_t max_samples, unsigned timeout_ms) {
    learn_engine->list_create(list_id, "test_digest", max_samples, timeout_ms);
    learn_engine->list_set_learn_writer(list_id, learn_writer);
    learn_engine->list_push_back_field(list_id, testHeader1, 0);  // test1.f16
    learn_engine->list_init(list_id);
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet::make_new(128, PacketBuffer(256), phv_source.get());
  }

  void learn_cb_(LearnEngineIface::msg_hdr_t msg_hdr, size_t size,
                 std::unique_ptr<char[]> data) {
    std::unique_lock<std::mutex> lock(cb_written_mutex);
    std::copy(&data[0], &data[size], buffer);
    cb_hdr = msg_hdr;
    cb_written = true;
    cb_written_cv.notify_one();
  }

  static void learn_cb(LearnEngineIface::msg_hdr_t msg_hdr, size_t size,
                       std::unique_ptr<char[]> data, void *cookie) {
    assert(size <= sizeof(buffer));
    auto instance = static_cast<LearningTest *>(cookie);
    instance->learn_cb_(msg_hdr, size, std::move(data));
  }
};

TEST_F(LearningTest, OneSample) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 1; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
  const char *data = buffer + sizeof(LearnEngineIface::msg_hdr_t);

  ASSERT_EQ(0, memcmp("LEA|", msg_hdr->sub_topic, sizeof(msg_hdr->sub_topic)));
  ASSERT_EQ(device_id, msg_hdr->switch_id);
  ASSERT_EQ(cxt_id, msg_hdr->cxt_id);
  ASSERT_EQ(list_id, msg_hdr->list_id);
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);

  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');
}

TEST_F(LearningTest, OneSampleTimeout) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  clock::time_point end = clock::now();

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
  const char *data = buffer + sizeof(LearnEngineIface::msg_hdr_t);

  ASSERT_EQ(0, memcmp("LEA|", msg_hdr->sub_topic, sizeof(msg_hdr->sub_topic)));
  ASSERT_EQ(device_id, msg_hdr->switch_id);
  ASSERT_EQ(cxt_id, msg_hdr->cxt_id);
  ASSERT_EQ(list_id, msg_hdr->list_id);
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);

  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');

  // check that the timeout was on time :)
  unsigned int elapsed = duration_cast<milliseconds>(end - start).count();
  ASSERT_GT(elapsed, timeout_ms - 20u);
  ASSERT_LT(elapsed, timeout_ms + 20u);
}

TEST_F(LearningTest, NoTimeout) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine->learn(list_id, pkt);

  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
  sleep_for(milliseconds(1000));
  // if we still cannot read after 1s, that means that nothing was written
  // i.e. no timeout happened
  // 1s was chosen arbitrarily
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
}

TEST_F(LearningTest, OneSampleConstData) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 1;
  unsigned int timeout_ms = 500;
  char buffer[sizeof(LearnEngineIface::msg_hdr_t) + 2];
  std::shared_ptr<MemoryAccessor> learn_writer(new
  MemoryAccessor(sizeof(buffer)));
  learn_engine->list_create(list_id, "test_digest", max_samples, timeout_ms);
  learn_engine->list_set_learn_writer(list_id, learn_writer);
  learn_engine->list_push_back_constant(list_id, "0xaba");  // 2 bytes
  learn_engine->list_init(list_id);

  Packet pkt = get_pkt();

  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
  const char *data = buffer + sizeof(LearnEngineIface::msg_hdr_t);

  ASSERT_EQ(0, memcmp("LEA|", msg_hdr->sub_topic, sizeof(msg_hdr->sub_topic)));
  ASSERT_EQ(device_id, msg_hdr->switch_id);
  ASSERT_EQ(cxt_id, msg_hdr->cxt_id);
  ASSERT_EQ(list_id, msg_hdr->list_id);
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);

  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');
}

TEST_F(LearningTest, TwoSampleTimeout) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 3; unsigned timeout_ms = 200;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine->learn(list_id, pkt);
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  f.set("0xabb");
  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  clock::time_point end = clock::now();

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
  const char *data = buffer + sizeof(LearnEngineIface::msg_hdr_t);

  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');
  EXPECT_EQ(data[2], '\x0a');
  EXPECT_EQ(data[3], '\xbb');

  // check that the timeout was on time :)
  unsigned int elapsed = duration_cast<milliseconds>(end - start).count();
  ASSERT_GT(elapsed, timeout_ms - 20u);
  ASSERT_LT(elapsed, timeout_ms + 20u);
}

TEST_F(LearningTest, Filter) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine->learn(list_id, pkt);
  learn_engine->learn(list_id, pkt);
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  f.set("0xabb");
  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
  const char *data = buffer + sizeof(LearnEngineIface::msg_hdr_t);

  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');
  EXPECT_EQ(data[2], '\x0a');
  EXPECT_EQ(data[3], '\xbb');
}

TEST_F(LearningTest, FilterAck) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 1; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
  const char *data = buffer + sizeof(LearnEngineIface::msg_hdr_t);

  learn_engine->learn(list_id, pkt);
  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);
  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');

  learn_engine->learn(list_id, pkt);  // cannot learn a second time
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  // ack and learn again
  ASSERT_EQ(LearnEngineIface::SUCCESS, learn_engine->ack(list_id, 0, 0));
  learn_engine->learn(list_id, pkt);
  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(1u, msg_hdr->buffer_id);  // buffer id was incremented
  ASSERT_EQ(1u, msg_hdr->num_samples);
  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');
}

TEST_F(LearningTest, FilterAcks) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  f.set("0xaba");
  learn_engine->learn(list_id, pkt);

  f.set("0xabb");
  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  learn_engine->learn(list_id, pkt);  // cannot learn a second time
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  // ack both samples and learn again
  ASSERT_EQ(LearnEngineIface::SUCCESS, learn_engine->ack(list_id, 0, {0, 1}));

  f.set("0xaba");
  learn_engine->learn(list_id, pkt);

  f.set("0xabb");
  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(1u, msg_hdr->buffer_id);  // buffer id was incremented
  ASSERT_EQ(2u, msg_hdr->num_samples);
}

TEST_F(LearningTest, FilterAckBuffer) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  f.set("0xaba");
  learn_engine->learn(list_id, pkt);

  f.set("0xabb");
  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  learn_engine->learn(list_id, pkt);  // cannot learn a second time
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  // ack whole buffer
  ASSERT_EQ(LearnEngineIface::SUCCESS, learn_engine->ack_buffer(list_id, 0));

  f.set("0xaba");
  learn_engine->learn(list_id, pkt);

  f.set("0xabb");
  learn_engine->learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(1u, msg_hdr->buffer_id);  // buffer id was incremented
  ASSERT_EQ(2u, msg_hdr->num_samples);
}

TEST_F(LearningTest, TimeoutChange) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  auto learn_and_ack = [this, list_id, &pkt](unsigned int tm) {
    clock::time_point s = clock::now();
    learn_engine->learn(list_id, pkt);
    learn_writer->read(buffer, sizeof(buffer));
    clock::time_point e = clock::now();
    unsigned int d = duration_cast<milliseconds>(e - s).count();
    ASSERT_GT(d, tm - 20u);
    ASSERT_LT(d, tm + 20u);
    auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
    // ack to be able to learn on the same packet
    ASSERT_EQ(LearnEngineIface::SUCCESS,
              learn_engine->ack_buffer(list_id, msg_hdr->buffer_id));
  };

  auto change_timeout = [this, list_id](unsigned int tm) {
    ASSERT_EQ(LearnEngineIface::SUCCESS,
              learn_engine->list_set_timeout(list_id, tm));
  };

  learn_and_ack(timeout_ms);

  timeout_ms = 200;
  change_timeout(timeout_ms);
  learn_and_ack(timeout_ms);

  timeout_ms = 100;
  change_timeout(timeout_ms);
  learn_and_ack(timeout_ms);
}

TEST_F(LearningTest, TimeoutChangeBadList) {
  LearnEngineIface::list_id_t list_id = 1;
  LearnEngineIface::list_id_t bad_list_id = 2;
  ASSERT_NE(list_id, bad_list_id);
  size_t max_samples = 2; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);
  ASSERT_EQ(LearnEngineIface::INVALID_LIST_ID,
            learn_engine->list_set_timeout(bad_list_id, timeout_ms));
}

TEST_F(LearningTest, MaxSamplesChange) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  auto check_recv = [this, list_id](size_t s) {
    learn_writer->read(buffer, sizeof(buffer));
    auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);
    ASSERT_EQ(s, msg_hdr->num_samples);
    // ack to be able to learn on the same packet
    ASSERT_EQ(LearnEngineIface::SUCCESS,
              learn_engine->ack_buffer(list_id, msg_hdr->buffer_id));
  };

  auto check_not_ready = [this](size_t d = 0u) {
    sleep_for(milliseconds(d));
    ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
  };

  auto do_send = [this, list_id](size_t s, size_t start) {
    Packet pkt = get_pkt();
    Field &f = pkt.get_phv()->get_field(testHeader1, 0);
    for (size_t c = start; c < start + s; c++) {
      f.set(std::to_string(c));
      learn_engine->learn(list_id, pkt);
    }
  };

  auto change_max_samples = [this, list_id](size_t s) {
    ASSERT_EQ(LearnEngineIface::SUCCESS,
              learn_engine->list_set_max_samples(list_id, s));
  };

  check_not_ready();
  do_send(1, 0);
  check_not_ready(300);
  do_send(1, 1);
  check_recv(2);

  check_not_ready(300);
  change_max_samples(1);
  check_not_ready(300);
  do_send(1, 0);
  check_recv(1);

  change_max_samples(2);
  do_send(1, 0);
  check_not_ready(300);
  // we're changing the buffer size to 1, because there is already 1 sample
  // waiting, the learn notification should be ready right away
  change_max_samples(1);
  check_recv(1);
}

TEST_F(LearningTest, MaxSamplesChangeBadList) {
  LearnEngineIface::list_id_t list_id = 1;
  LearnEngineIface::list_id_t bad_list_id = 2;
  ASSERT_NE(list_id, bad_list_id);
  size_t max_samples = 2; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);
  ASSERT_EQ(LearnEngineIface::INVALID_LIST_ID,
            learn_engine->list_set_max_samples(bad_list_id, max_samples));
}

TEST_F(LearningTest, OneSampleCbMode) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 1; unsigned timeout_ms = 100;
  learn_engine->list_create(list_id, "test_digest", max_samples, timeout_ms);
  learn_engine->list_set_learn_cb(list_id, LearningTest::learn_cb, this);
  learn_engine->list_push_back_field(list_id, testHeader1, 0);  // test1.f16
  learn_engine->list_init(list_id);

  cb_written = false;

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine->learn(list_id, pkt);

  std::unique_lock<std::mutex> lock(cb_written_mutex);
  while (!cb_written) {
    cb_written_cv.wait(lock);
  }

  const char *data = buffer;

  ASSERT_EQ(device_id, cb_hdr.switch_id);
  ASSERT_EQ(cxt_id, cb_hdr.cxt_id);
  ASSERT_EQ(list_id, cb_hdr.list_id);
  ASSERT_EQ(0u, cb_hdr.buffer_id);
  ASSERT_EQ(1u, cb_hdr.num_samples);

  EXPECT_EQ(data[0], '\x0a');
  EXPECT_EQ(data[1], '\xba');
}

TEST_F(LearningTest, FilterStress) {
  LearnEngineIface::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  auto msg_hdr = reinterpret_cast<LearnEngineIface::msg_hdr_t *>(buffer);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  uint16_t v;
  v = 0;
  std::vector<std::pair<LearnEngineIface::buffer_id_t, int> > samples_to_ack;
  for (int i = 0; i < 1000; i++) {
    f.set(v++);
    learn_engine->learn(list_id, pkt);
    f.set(v++);
    learn_engine->learn(list_id, pkt);
    learn_writer->read(buffer, sizeof(buffer));
    ASSERT_EQ(msg_hdr->num_samples, 2u);
    // we will ack the first sample but not the second one
    samples_to_ack.emplace_back(
        static_cast<LearnEngineIface::buffer_id_t>(msg_hdr->buffer_id), 0);
  }

  for (const auto &p : samples_to_ack) {
    ASSERT_EQ(learn_engine->ack(list_id, p.first, p.second),
              LearnEngineIface::SUCCESS);
  }

  ASSERT_EQ(learn_engine->list_set_max_samples(list_id, 1),
            LearnEngineIface::SUCCESS);
  v = 0;
  for (int i = 0; i < 1000; i++) {
    f.set(v++);
    learn_engine->learn(list_id, pkt);
    f.set(v++);
    learn_engine->learn(list_id, pkt);
    learn_writer->read(buffer, sizeof(buffer));
    ASSERT_EQ(msg_hdr->num_samples, 1u);
    const auto data = reinterpret_cast<unsigned char *>(buffer)
        + sizeof(LearnEngineIface::msg_hdr_t);
    auto learned_v =
        static_cast<uint16_t>(data[0]) << 8 | static_cast<uint16_t>(data[1]);
    EXPECT_EQ(learned_v, v - 2);
  }
}
