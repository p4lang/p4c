#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>

#include <cassert>

#include "bm_sim/learning.h"

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;

// Google Test fixture for learning tests
class LearningTest : public ::testing::Test {
protected:
  typedef std::chrono::high_resolution_clock clock;

protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  LearnEngine learn_engine;

  LearningTest()
    : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }

  // std::unique_ptr<Packet> get_pkt() {
  //   return std::unique_ptr<Packet>(new Packet(0, 0, 0, PacketBuffer(256)));
  // }

  Packet get_pkt() {
    return Packet(0, 0, 0, PacketBuffer(256));
  }
};

class MemoryAccessor : public LearnWriter {
public:
  enum class Status { CAN_READ, CAN_WRITE };
public:
  MemoryAccessor(size_t max_size)
    : max_size(max_size), status(Status::CAN_WRITE) {
    buffer_.reserve(max_size);
  }

  // I could use conditional variables, but I'm feeling lazy :P

  int send(const char *buffer, size_t len) const override {
    if(len > max_size) return -1;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_WRITE) {
      lock.unlock();
      sleep_for(milliseconds(50));
      lock.lock();
    }
    buffer_.insert(buffer_.end(), buffer, buffer + len);
    status = Status::CAN_READ;
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
      lock.unlock();
      sleep_for(milliseconds(50));
      lock.lock();
    }
    for(const auto &msg : msgs) {
      buffer_.insert(buffer_.end(), msg.buf, msg.buf + msg.len);
    }
    status = Status::CAN_READ;
    return 0;
  }

  int read(char *dst, size_t len) const {
    len = (len > max_size) ? max_size : len;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_READ) {
      lock.unlock();
      sleep_for(milliseconds(50));
      lock.lock();
    }
    std::copy(buffer_.begin(), buffer_.begin() + len, dst);
    buffer_.clear();
    status = Status::CAN_WRITE;
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
};

TEST_F(LearningTest, OneSample) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 1;
  unsigned int timeout_ms = 500;
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 2];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, max_samples, timeout_ms);
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

TEST_F(LearningTest, OneSampleTimeout) {
  clock::time_point start = clock::now();

  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2;
  unsigned int timeout_ms = 500;
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 2];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, max_samples, timeout_ms);
  learn_engine.list_push_back_constant(list_id, "0xaba"); // 2 bytes
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();

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
  ASSERT_GT(elapsed, timeout_ms - 50u);
  ASSERT_LT(elapsed, timeout_ms + 50u);
}

TEST_F(LearningTest, NoTimeout) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2;
  unsigned int timeout_ms = 0; // no timeout
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 2];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, max_samples, timeout_ms);
  learn_engine.list_push_back_constant(list_id, "0xaba"); // 2 bytes
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();

  learn_engine.learn(list_id, pkt);

  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
  sleep_for(milliseconds(1000));
  // if we still cannot read after 1s, that means that nothing was written
  // i.e. no timeout happened
  // 1s was chosen arbitrarily
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, learn_writer->check_status());
}

TEST_F(LearningTest, OneSampleField) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 1;
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 2];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, max_samples, 0);
  learn_engine.list_push_back_field(list_id, testHeader1, 0); // test1.f16
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  phv->get_field(testHeader1, 0).set("0xaba");

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
  clock::time_point start = clock::now();

  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 3;
  unsigned int timeout_ms = 500;
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 4];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, max_samples, timeout_ms);
  learn_engine.list_push_back_field(list_id, testHeader1, 0); // test1.f16
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  Field &f = phv->get_field(testHeader1, 0);
  f.set("0xaba");

  learn_engine.learn(list_id, pkt);
  sleep_for(milliseconds(250));
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
  ASSERT_GT(elapsed, timeout_ms - 50u);
  ASSERT_LT(elapsed, timeout_ms + 50u);
}

TEST_F(LearningTest, Filter) {
  LearnEngine::list_id_t list_id = 1;
  size_t max_samples = 2;
  unsigned int timeout_ms = 0;
  char buffer[sizeof(LearnEngine::msg_hdr_t) + 4];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, max_samples, timeout_ms);
  learn_engine.list_push_back_field(list_id, testHeader1, 0); // test1.f16
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  Field &f = phv->get_field(testHeader1, 0);
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
