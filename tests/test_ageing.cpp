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

#include <bm/bm_sim/ageing.h>

#include "utils.h"

using namespace bm;

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;

// Google Test fixture for learning tests
class AgeingTest : public ::testing::Test {
protected:
  typedef std::chrono::high_resolution_clock clock;

protected:
  PHVFactory phv_factory;

  const int device_id = 0;
  const int cxt_id = 0;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::shared_ptr<MemoryAccessor> ageing_writer;
  char buffer[4096];

  std::unique_ptr<AgeingMonitor> ageing_monitor;

  clock::time_point start;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  AgeingTest()
      : testHeaderType("test_t", 0),
        action_fn("actionA", 0),
        ageing_writer(new MemoryAccessor(4096)),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::EXACT);

    typedef MatchTableAbstract::ActionEntry ActionEntry;
    typedef MatchUnitExact<ActionEntry> MUExact;

    LookupStructureFactory factory;

    std::unique_ptr<MUExact> match_unit(new MUExact(1, key_builder, &factory));

    // counters disabled, ageing enabled
    table = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit), false, true)
    );
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    start = clock::now();
  }

  // virtual void TearDown() { }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(128, PacketBuffer(256), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    // return std::move(packet);
    // enable NVRO
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
        new AgeingMonitor(device_id, cxt_id, ageing_writer, sweep_int));
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
