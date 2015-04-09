#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>

#include <cassert>

#include "bm_sim/learning.h"

// Google Test fixture for learning tests
class LearningTest : public ::testing::Test {
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
private:
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
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      lock.lock();
    }
    std::copy(buffer, buffer + len, buffer_.begin());
    status = Status::CAN_READ;
    return 0;
  }

  int read(char *dst, size_t len) const {
    len = (len > max_size) ? max_size : len;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_READ) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      lock.lock();
    }
    std::copy(buffer_.begin(), buffer_.begin() + len, dst);
    status = Status::CAN_WRITE;
    return 0;
  }

private:
  // dirty trick (mutable) to make sure that send() is override
  mutable std::vector<char> buffer_;
  size_t max_size;
  mutable Status status;
  mutable std::mutex mutex;
};

TEST_F(LearningTest, OneSample) {
  LearnEngine::list_id_t list_id = 0;
  char buffer[2];
  std::shared_ptr<MemoryAccessor> learn_writer(new MemoryAccessor(sizeof(buffer)));
  learn_engine.list_create(list_id, learn_writer, 1, 10000);
  learn_engine.list_push_back_constant(list_id, "0xaba"); // 2 bytes
  learn_engine.list_init(list_id);

  Packet pkt = get_pkt();

  learn_engine.learn(list_id, pkt);

  learn_writer->read(buffer, sizeof(buffer));

  ASSERT_EQ((char) 0xa, buffer[0]);
  ASSERT_EQ((char) 0xba, buffer[1]);
}
