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

#include "bm_sim/ageing.h"

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;

/* Exactly the same as the MemoryAccessor for the learning tests, I need to
   unify LearnWriter and AgeingWriter soon */

class MemoryAccessor : public AgeingWriter {
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
class AgeingTest : public ::testing::Test {
protected:
  typedef std::chrono::high_resolution_clock clock;

protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::shared_ptr<MemoryAccessor> ageing_writer;
  char buffer[4096];

  std::unique_ptr<AgeingMonitor> ageing_monitor;

  clock::time_point start;

  AgeingTest()
    : testHeaderType("test_t", 0),
      action_fn("actionA", 0),
      ageing_writer(new MemoryAccessor(4096)) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16);

    typedef MatchTableAbstract::ActionEntry ActionEntry;
    typedef MatchUnitExact<ActionEntry> MUExact;

    std::unique_ptr<MUExact> match_unit(new MUExact(1, key_builder));

    // counters disabled, ageing enabled
    table = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit), false, true)
    );
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
    start = clock::now();
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    Packet packet(0, 0, 0, 128, PacketBuffer(256));
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    return packet;
  }

  MatchErrorCode add_entry(const std::string &key, entry_handle_t *handle,
			   unsigned int ttl_ms) {
    ActionData action_data;
    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
    MatchErrorCode rc;
    rc = table->add_entry(match_key, &action_fn, action_data, handle);
    if(rc != MatchErrorCode::SUCCESS) return rc;
    return table->set_entry_ttl(*handle, ttl_ms);
  }

  MatchErrorCode delete_entry(entry_handle_t handle) {
    return table->delete_entry(handle);
  }

  bool send_pkt(const std::string &key, entry_handle_t *handle) {
    Packet pkt = get_pkt();
    Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
    f.set(key);
    bool hit;
    // lookup is enough to update timestamp (done in match unit),
    // no need for apply_action
    table->lookup(pkt, &hit, handle);
    return hit;
  }

  void init_monitor(unsigned int sweep_int) {
    ageing_monitor = std::unique_ptr<AgeingMonitor>(
      new AgeingMonitor(ageing_writer, sweep_int)
    );
    ageing_monitor->add_table(table.get());
  }
};

TEST_F(AgeingTest, OneNotification) {
  std::string key_("\x0a\xba");
  std::string key("0x0aba");
  entry_handle_t handle_1;
  entry_handle_t lookup_handle;
  unsigned int sweep_int = 200u; // use it as timeout too
  init_monitor(sweep_int);
  ASSERT_EQ(MatchErrorCode::SUCCESS, add_entry(key_, &handle_1, sweep_int));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, ageing_writer->check_status());
  sleep_for(milliseconds(150));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, ageing_writer->check_status());
  
  auto tp1 = clock::now();
  ASSERT_TRUE(send_pkt(key, &lookup_handle));
  ageing_writer->read(buffer, sizeof(buffer));
  auto tp2 = clock::now();

  unsigned int elapsed = duration_cast<milliseconds>(tp2 - tp1).count();
  ASSERT_GT(elapsed, sweep_int - 20u);
  ASSERT_LT(elapsed, 2 * sweep_int + 20u);

  // delete and make sure we do not get a new notification
  ASSERT_EQ(MatchErrorCode::SUCCESS, delete_entry(handle_1));

  sleep_for(milliseconds(2 * sweep_int + 100u));
  ASSERT_NE(MemoryAccessor::Status::CAN_READ, ageing_writer->check_status());
}

TEST_F(AgeingTest, NoDuplicate) {
  std::string key_("\x0a\xba");
  std::string key("0x0aba");
  entry_handle_t handle_1;
  entry_handle_t lookup_handle;
  unsigned int sweep_int = 200u;
  init_monitor(sweep_int);
  auto tp1 = clock::now();
  ASSERT_EQ(MatchErrorCode::SUCCESS, add_entry(key_, &handle_1, sweep_int));
  ageing_writer->read(buffer, sizeof(buffer));
  auto tp2 = clock::now();

  unsigned int elapsed = duration_cast<milliseconds>(tp2 - tp1).count();
  ASSERT_GT(elapsed, sweep_int - 20u);
  ASSERT_LT(elapsed, 2 * sweep_int + 20u);

  auto tp3 = clock::now();
  ageing_writer->read(buffer, sizeof(buffer));
  auto tp4 = clock::now();

  // we make sure that the next sweep does not generate a message

  elapsed = duration_cast<milliseconds>(tp4 - tp3).count();
  ASSERT_GT(elapsed, (unsigned int) (sweep_int * 1.5));
}
