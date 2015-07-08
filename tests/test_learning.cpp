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

#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>

#include <cassert>

#include "bm_sim/learning.h"

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;

class MemoryAccessor : public LearnWriter {
public:
  enum class Status { CAN_READ, CAN_WRITE };
public:
  MemoryAccessor(size_t max_size)
    : max_size(max_size), status(Status::CAN_WRITE) {
    buffer_.reserve(max_size);
  }

  int send(const char *buffer, size_t len) const override {
    if(len > max_size) return -1;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_WRITE) {
      can_write.wait(lock);
    }
    buffer_.insert(buffer_.end(), buffer, buffer + len);
    status = Status::CAN_READ;
    can_read.notify_one();
    return 0;
  }

  int send_msgs(
      const std::initializer_list<TransportIface::MsgBuf> &msgs
  ) const override
  {
    size_t len = 0;
    for(const auto &msg : msgs) {
      len += msg.len;
    }
    if(len > max_size) return -1;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_WRITE) {
      can_write.wait(lock);
    }
    for(const auto &msg : msgs) {
      buffer_.insert(buffer_.end(), msg.buf, msg.buf + msg.len);
    }
    status = Status::CAN_READ;
    can_read.notify_one();
    return 0;
  }

  int read(char *dst, size_t len) const {
    len = (len > max_size) ? max_size : len;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_READ) {
      can_read.wait(lock);
    }
    std::copy(buffer_.begin(), buffer_.begin() + len, dst);
    buffer_.clear();
    status = Status::CAN_WRITE;
    can_write.notify_one();
    return 0;
  }

  Status check_status() {
    std::unique_lock<std::mutex> lock(mutex);
    return status;
  }

private:
  // dirty trick (mutable) to make sure that send() const is override
  mutable std::vector<char> buffer_;
  size_t max_size;
  mutable Status status;
  mutable std::mutex mutex;
  mutable std::condition_variable can_write;
  mutable std::condition_variable can_read;
};

// Google Test fixture for learning tests
class LearningTest : public ::testing::Test {
protected:
  typedef std::chrono::high_resolution_clock clock;

protected:
  PHVFactory phv_factory;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  std::shared_ptr<MemoryAccessor> learn_writer;
  char buffer[4096];

  // used exclusively for callback mode
  LearnEngine::msg_hdr_t cb_hdr;
  bool cb_written;
  mutable std::mutex cb_written_mutex;
  mutable std::condition_variable cb_written_cv;

  LearnEngine learn_engine;

  clock::time_point start;

  LearningTest()
    : testHeaderType("test_t", 0),
      learn_writer(new MemoryAccessor(4096)) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
    start = clock::now();
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }

  void learn_on_test1_f16(LearnEngine::list_id_t list_id,
			  size_t max_samples, unsigned timeout_ms) {
    learn_engine.list_create(list_id, max_samples, timeout_ms);
    learn_engine.list_set_learn_writer(list_id, learn_writer);
    learn_engine.list_push_back_field(list_id, testHeader1, 0); // test1.f16
    learn_engine.list_init(list_id);
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, 128, PacketBuffer(256));
  }

  void learn_cb_(LearnEngine::msg_hdr_t msg_hdr, size_t size,
		 std::unique_ptr<char []> data) {
    std::unique_lock<std::mutex> lock(cb_written_mutex);
    std::copy(&data[0], &data[size], buffer);
    cb_hdr = msg_hdr;
    cb_written = true;
    cb_written_cv.notify_one();    
  }

  static void learn_cb(LearnEngine::msg_hdr_t msg_hdr, size_t size,
		       std::unique_ptr<char []> data, void *cookie) {
    assert(size <= sizeof(buffer));
    ((LearningTest *) cookie)->learn_cb_(msg_hdr, size, std::move(data));
  }
};

TEST_F(LearningTest, OneSample) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 1; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;
  const char *data = buffer + sizeof(LearnEngine::msg_hdr_t);

  ASSERT_EQ(0, msg_hdr->switch_id);
  ASSERT_EQ(list_id, msg_hdr->list_id);
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);

  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);
}

TEST_F(LearningTest, OneSampleTimeout) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 100;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  clock::time_point end = clock::now();

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;
  const char *data = buffer + sizeof(LearnEngine::msg_hdr_t);

  ASSERT_EQ(0, msg_hdr->switch_id);
  ASSERT_EQ(list_id, msg_hdr->list_id);
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);

  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);

  // check that the timeout was on time :)
  unsigned int elapsed = duration_cast<milliseconds>(end - start).count();
  ASSERT_GT(elapsed, timeout_ms - 20u);
  ASSERT_LT(elapsed, timeout_ms + 20u);
}

TEST_F(LearningTest, NoTimeout) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);

  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
  sleep_for(milliseconds(1000));
  // if we still cannot read after 1s, that means that nothing was written
  // i.e. no timeout happened
  // 1s was chosen arbitrarily
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
}

TEST_F(LearningTest, OneSampleConstData) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 1;
  unsigned int timeout_ms = 500;
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 2];
  std::shared_ptr<MemoryAccessor> learn_writer(new
  MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, max_samples, timeout_ms);
  learn_engine.list_set_learn_writer(list_id, learn_writer);
  learn_engine.list_push_back_constant(list_id, "0xaba"); // 2 bytes
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();

  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;
  const char *data = buffer + sizeof(LearnEngine::msg_hdr_t);

  ASSERT_EQ(0, msg_hdr->switch_id);
  ASSERT_EQ(list_id, msg_hdr->list_id);
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);

  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);
}

TEST_F(LearningTest, TwoSampleTimeout) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 3; unsigned timeout_ms = 200;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  f.set("0xabb");
  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  clock::time_point end = clock::now();

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;
  const char *data = buffer + sizeof(LearnEngine::msg_hdr_t);

  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);
  ASSERT_EQ((char) 0xa, data[2]);
  ASSERT_EQ((char) 0xbb, data[3]);

  // check that the timeout was on time :)
  unsigned int elapsed = duration_cast<milliseconds>(end - start).count();
  ASSERT_GT(elapsed, timeout_ms - 20u);
  ASSERT_LT(elapsed, timeout_ms + 20u);
}

TEST_F(LearningTest, Filter) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);
  learn_engine.learn(list_id, pkt);
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  f.set("0xabb");
  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;
  const char *data = buffer + sizeof(LearnEngine::msg_hdr_t);

  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);
  ASSERT_EQ((char) 0xa, data[2]);
  ASSERT_EQ((char) 0xbb, data[3]);
}

TEST_F(LearningTest, FilterAck) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 1; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;
  const char *data = buffer + sizeof(LearnEngine::msg_hdr_t);

  learn_engine.learn(list_id, pkt);
  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(1u, msg_hdr->num_samples);
  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);

  learn_engine.learn(list_id, pkt); // cannot learn a second time
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  learn_engine.ack(list_id, 0, 0); // ack and learn again
  learn_engine.learn(list_id, pkt);
  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(1u, msg_hdr->buffer_id); // buffer id was incremented
  ASSERT_EQ(1u, msg_hdr->num_samples);
  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);
}

TEST_F(LearningTest, FilterAcks) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  f.set("0xaba");
  learn_engine.learn(list_id, pkt);

  f.set("0xabb");
  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  learn_engine.learn(list_id, pkt); // cannot learn a second time
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  learn_engine.ack(list_id, 0, {0, 1}); // ack both samples and learn again

  f.set("0xaba");
  learn_engine.learn(list_id, pkt);

  f.set("0xabb");
  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(1u, msg_hdr->buffer_id); // buffer id was incremented
  ASSERT_EQ(2u, msg_hdr->num_samples);
}

TEST_F(LearningTest, FilterAckBuffer) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2; unsigned timeout_ms = 0;
  learn_on_test1_f16(list_id, max_samples, timeout_ms);

  LearnEngine::msg_hdr_t *msg_hdr = (LearnEngine::msg_hdr_t *) buffer;

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  f.set("0xaba");
  learn_engine.learn(list_id, pkt);

  f.set("0xabb");
  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(0u, msg_hdr->buffer_id);
  ASSERT_EQ(2u, msg_hdr->num_samples);

  learn_engine.learn(list_id, pkt); // cannot learn a second time
  sleep_for(milliseconds(100));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());

  learn_engine.ack_buffer(list_id, 0); // ack whole buffer

  f.set("0xaba");
  learn_engine.learn(list_id, pkt);

  f.set("0xabb");
  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));
  ASSERT_EQ(1u, msg_hdr->buffer_id); // buffer id was incremented
  ASSERT_EQ(2u, msg_hdr->num_samples);
}

TEST_F(LearningTest, OneSampleCbMode) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 1; unsigned timeout_ms = 100;
  learn_engine.list_create(list_id, max_samples, timeout_ms);
  learn_engine.list_set_learn_cb(list_id, LearningTest::learn_cb, this);
  learn_engine.list_push_back_field(list_id, testHeader1, 0); // test1.f16
  learn_engine.list_init(list_id);

  cb_written = false;

  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);

  std::unique_lock<std::mutex> lock(cb_written_mutex);
  while(!cb_written) {
    cb_written_cv.wait(lock);
  }

  const char *data = buffer;

  ASSERT_EQ(0, cb_hdr.switch_id);
  ASSERT_EQ(list_id, cb_hdr.list_id);
  ASSERT_EQ(0u, cb_hdr.buffer_id);
  ASSERT_EQ(1u, cb_hdr.num_samples);

  ASSERT_EQ((char) 0xa, data[0]);
  ASSERT_EQ((char) 0xba, data[1]);
}
