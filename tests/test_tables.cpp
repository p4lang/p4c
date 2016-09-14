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
#include <random>
#include <thread>
#include <future>
#include <limits>
#include <bm/bm_sim/tables.h>

using namespace bm;

using testing::Types;

using std::string;
using std::to_string;

using std::chrono::milliseconds;
using std::this_thread::sleep_until;

typedef MatchTableAbstract::ActionEntry ActionEntry;
typedef MatchUnitExact<ActionEntry> MUExact;
typedef MatchUnitLPM<ActionEntry> MULPM;
typedef MatchUnitTernary<ActionEntry> MUTernary;
typedef MatchUnitRange<ActionEntry> MURange;

namespace {

// used for hit / miss next node selection tests (NextNodeHitMiss)
struct DummyNode: public ControlFlowNode {
  DummyNode()
      : ControlFlowNode("", 0) { }

  const ControlFlowNode *operator()(Packet *pkt) const {
    return nullptr;
  }
};

template <typename MUType>
struct match_type_test {

};
template <>
struct match_type_test<MUExact> {
  static constexpr MatchKeyParam::Type type = MatchKeyParam::Type::EXACT;
};
template <>
struct match_type_test<MULPM> {
  static constexpr MatchKeyParam::Type type = MatchKeyParam::Type::LPM;
};
template <>
struct match_type_test<MUTernary> {
  static constexpr MatchKeyParam::Type type = MatchKeyParam::Type::TERNARY;
};
template <>
struct match_type_test<MURange> {
  static constexpr MatchKeyParam::Type type = MatchKeyParam::Type::RANGE;
};

LookupStructureFactory lookup_factory;

}  // namespace

template <typename MUType>
class TableSizeTwo : public ::testing::Test {
 protected:
  static constexpr size_t t_size = 2u;

  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  MatchKeyBuilder key_builder_w_valid;
  std::unique_ptr<MatchTable> table_w_valid;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  DummyNode node_miss_default{};

  TableSizeTwo()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16,
                                match_type_test<MUType>::type);

    key_builder_w_valid.push_back_field(testHeader1, 0, 16,
                                        match_type_test<MUType>::type);
    key_builder_w_valid.push_back_valid_header(testHeader2);

    std::unique_ptr<MUType> match_unit;

    // true enables counters
    match_unit = std::unique_ptr<MUType>(
        new MUType(t_size, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit), true)
    );
    table->set_next_node(0, nullptr);
    table->set_next_node_miss_default(&node_miss_default);

    match_unit = std::unique_ptr<MUType>(
        new MUType(t_size, key_builder_w_valid, &lookup_factory));
    table_w_valid = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit)));
    table_w_valid->set_next_node(0, nullptr);
    table_w_valid->set_next_node_miss_default(&node_miss_default);
  }

  MatchErrorCode add_entry(const std::string &key,
			   entry_handle_t *handle);

  MatchErrorCode add_entry_w_valid(const std::string &key,
				   bool valid,
				   entry_handle_t *handle);

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        length, PacketBuffer(length * 2), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    // return std::move(packet);
    // enable NVRO
    return packet;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

template<>
MatchErrorCode
TableSizeTwo<MUExact>::add_entry(const std::string &key,
				 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeTwo<MULPM>::add_entry(const std::string &key,
			       entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeTwo<MUTernary>::add_entry(const std::string &key,
				   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, key, std::move(mask));
  return table->add_entry(match_key, &action_fn, action_data, handle, priority);
}

template<>
MatchErrorCode
TableSizeTwo<MURange>::add_entry(const std::string &key,
                                 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  Data start(key.data(), key.size());
  Data end;
  // we create a range with 5 values
  end.add(start, Data(4));
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::RANGE, key, end.get_string());
  return table->add_entry(match_key, &action_fn, action_data, handle, priority);
}

template<>
MatchErrorCode
TableSizeTwo<MUExact>::add_entry_w_valid(const std::string &key,
					 bool valid,
					 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  // I'm putting the valid match in second position on purpose
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeTwo<MULPM>::add_entry_w_valid(const std::string &key,
				       bool valid,
				       entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeTwo<MUTernary>::add_entry_w_valid(const std::string &key,
					   bool valid,
					   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, key, std::move(mask));
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data,
				  handle, priority);
}

template<>
MatchErrorCode
TableSizeTwo<MURange>::add_entry_w_valid(const std::string &key,
					   bool valid,
					   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  Data start(key.data(), key.size());
  Data end;
  // we create a range with 5 values
  end.add(start, Data(4));
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::RANGE, key, end.get_string());
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data,
				  handle, priority);
}

typedef Types<MUExact,
	      MULPM,
	      MUTernary,
              MURange> TableTypes;

TYPED_TEST_CASE(TableSizeTwo, TableTypes);

TYPED_TEST(TableSizeTwo, AddEntry) {
  std::string key_1 = "\xaa\xaa";
  std::string key_2 = "\xbb\xbb";
  std::string key_3 = "\xcc\xcc";
  entry_handle_t handle;
  MatchErrorCode rc;

  rc = this->add_entry(key_1, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(1u, this->table->get_num_entries());

  rc = this->add_entry(key_1, &handle);
  ASSERT_EQ(MatchErrorCode::DUPLICATE_ENTRY, rc);
  ASSERT_EQ(1u, this->table->get_num_entries());

  rc = this->add_entry(key_2, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(2u, this->table->get_num_entries());

  rc = this->add_entry(key_3, &handle);
  ASSERT_EQ(MatchErrorCode::TABLE_FULL, rc);
  ASSERT_EQ(2u, this->table->get_num_entries());
}

TYPED_TEST(TableSizeTwo, IsValidHandle) {
  std::string key_ = "\xaa\xaa";
  entry_handle_t handle_1 = 0;
  MatchErrorCode rc;

  ASSERT_FALSE(this->table->is_valid_handle(handle_1));

  rc = this->add_entry(key_, &handle_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  ASSERT_TRUE(this->table->is_valid_handle(handle_1));
}

TYPED_TEST(TableSizeTwo, DeleteEntry) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(0u, this->table->get_num_entries());

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::INVALID_HANDLE, rc);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
}

TYPED_TEST(TableSizeTwo, DeleteEntryHandleUpdate) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle_1, handle_2;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = this->table->delete_entry(handle_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(0u, this->table->get_num_entries());

  rc = this->add_entry(key_, &handle_2);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  ASSERT_NE(handle_1, handle_2);

  rc = this->table->delete_entry(handle_1);
  ASSERT_EQ(MatchErrorCode::EXPIRED_HANDLE, rc);
}

TYPED_TEST(TableSizeTwo, LookupEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);

  // has to be large enough for range test
  f.set("0xcba");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
}

TYPED_TEST(TableSizeTwo, NextNodeHitMiss) {
  std::string key = "\x0a\xba";
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  // before even adding entry
  // miss, next node is default one (as per the P4 program)
  f.set("0xaba");
  ASSERT_EQ(&this->node_miss_default, this->table->apply_action(&pkt));

  rc = this->add_entry(key, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  // we call apply_action, even though no action has been set

  // base case
  // hit, next node set to nullptr
  f.set("0xaba");
  ASSERT_EQ(nullptr, this->table->apply_action(&pkt));
  // miss, next node is default one (as per the P4 program)
  f.set("0xcba");
  ASSERT_EQ(&this->node_miss_default, this->table->apply_action(&pkt));

  // need to remove entry because the node pointer is inserted in the entry when
  // the entry is added
  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  DummyNode dummy_node_hit, dummy_node_miss;
  this->table->set_next_node_hit(&dummy_node_hit);
  this->table->set_next_node_miss(&dummy_node_miss);

  rc = this->add_entry(key, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  ASSERT_EQ(&dummy_node_hit, this->table->apply_action(&pkt));
  f.set("0xcba");
  ASSERT_EQ(&dummy_node_miss, this->table->apply_action(&pkt));

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  ASSERT_EQ(&dummy_node_miss, this->table->apply_action(&pkt));
}

TYPED_TEST(TableSizeTwo, SetDefaultAction) {
  std::string key = "\x0a\xba";
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  // before even adding entry
  // miss, next node is default one (as per the P4 program)
  f.set("0xaba");
  ASSERT_EQ(&this->node_miss_default, this->table->apply_action(&pkt));

  MatchTable::Entry de;

  rc = this->table->get_default_entry(&de);
  ASSERT_EQ(MatchErrorCode::NO_DEFAULT_ENTRY, rc);

  rc = this->table->set_default_action(&this->action_fn, ActionData());
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(nullptr, this->table->apply_action(&pkt));

  rc = this->table->get_default_entry(&de);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(&this->action_fn, de.action_fn);
}

TYPED_TEST(TableSizeTwo, ConstDefaultActionFn) {
  std::string key = "\x0a\xba";
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  ActionFn bad_action_fn("bad_action", 1);
  this->table->set_next_node(1, nullptr);

  this->table->set_const_default_action_fn(&this->action_fn);

  // try to set the default action function to something new, should fail
  rc = this->table->set_default_action(&bad_action_fn, ActionData());
  ASSERT_EQ(MatchErrorCode::DEFAULT_ACTION_IS_CONST, rc);

  // no default entry, so next node is default one
  ASSERT_EQ(&this->node_miss_default, this->table->apply_action(&pkt));

  // setting the default entry succeeds if the action function matches the const
  // specification
  rc = this->table->set_default_action(&this->action_fn, ActionData());
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  ASSERT_EQ(nullptr, this->table->apply_action(&pkt));
}

TYPED_TEST(TableSizeTwo, ConstDefaultEntry) {
  std::string key = "\x0a\xba";
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  ActionFn bad_action_fn("bad_action", 1);
  this->table->set_next_node(1, nullptr);

  auto change_default_entry = [this, &bad_action_fn](
      MatchErrorCode expected_rc_action_data,
      MatchErrorCode expected_rc_action_fn) {
    MatchErrorCode rc;

    rc = this->table->set_default_action(&this->action_fn, ActionData());
    ASSERT_EQ(expected_rc_action_data, rc);

    rc = this->table->set_default_action(&bad_action_fn, ActionData());
    ASSERT_EQ(expected_rc_action_fn, rc);
  };

  // false means non-const
  this->table->set_default_entry(&this->action_fn, ActionData(), false);

  // both changing action data and action fn should succeed
  change_default_entry(MatchErrorCode::SUCCESS, MatchErrorCode::SUCCESS);

  this->table->set_const_default_action_fn(&this->action_fn);

  // can change the action data, not the action function
  change_default_entry(MatchErrorCode::SUCCESS,
                       MatchErrorCode::DEFAULT_ACTION_IS_CONST);

  // true means const
  this->table->set_default_entry(&this->action_fn, ActionData(), true);

  // no change possible
  change_default_entry(MatchErrorCode::DEFAULT_ENTRY_IS_CONST,
                       MatchErrorCode::DEFAULT_ENTRY_IS_CONST);

  ASSERT_EQ(nullptr, this->table->apply_action(&pkt));
}

TYPED_TEST(TableSizeTwo, ModifyEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  const ActionEntry &entry_1 = this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_EQ(0, entry_1.action_fn.action_data_size());

  // in theory I could modify the entry directly using the pointer, but I need
  // to exercise my modify_entry function

  // we modify the entry, this time using some action data
  ActionData new_action_data;
  new_action_data.push_back_action_data(0xaba);
  this->table->modify_entry(handle, &(this->action_fn),
			    std::move(new_action_data));

  const ActionEntry &entry_2 = this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_NE(0, entry_2.action_fn.action_data_size());
  ASSERT_EQ((unsigned) 0xaba,
            entry_2.action_fn.get_action_data_at(0).get_uint());
}

TYPED_TEST(TableSizeTwo, Counters) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  entry_handle_t bad_handle = 999u;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  uint64_t counter_bytes = 0;
  uint64_t counter_packets = 0;

  rc = this->table->query_counters(bad_handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::INVALID_HANDLE);

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(0u, counter_bytes);
  ASSERT_EQ(0u, counter_packets);

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  // there is no action specified for the entry, potential for a bug :(
  this->table->apply_action(&pkt);

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);

  f.set("0xcba");
  this->table->apply_action(&pkt);

  // counters should not have been incremented
  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);
}

TYPED_TEST(TableSizeTwo, Meters) {
  typedef std::chrono::high_resolution_clock clock;

  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  entry_handle_t bad_handle = 999u;
  MatchErrorCode rc;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  const Field &f_c = pkt.get_phv()->get_field(this->testHeader2, 0);

  const Meter::color_t GREEN = 0;
  const Meter::color_t RED = 1;

  // 1 rate, same size as table
  MeterArray meters("meter_test", 0, Meter::MeterType::PACKETS, 1,
                    this->t_size);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(1u, this->table->get_num_entries());

  std::vector<Meter::rate_config_t> rates = {};
  rc = this->table->set_meter_rates(handle, rates);
  ASSERT_EQ(MatchErrorCode::METERS_DISABLED, rc);
  this->table->set_direct_meters(&meters, this->testHeader2, 0);

  std::vector<Meter::color_t> output;
  output.reserve(32);

  Meter::reset_global_clock();

  clock::time_point next_stop = clock::now();

  // meter rates not configured yet, all packets should be 'GREEN'

  for (size_t i = 0; i < 20; i++) {
    this->table->apply_action(&pkt);
    Meter::color_t color = f_c.get_int();
    output.push_back(color);
    next_stop += milliseconds(100);
    sleep_until(next_stop);
  }

  std::vector<Meter::color_t> expected(20, GREEN);
  ASSERT_EQ(expected, output);

  rc = this->table->set_meter_rates(handle, rates);
  ASSERT_EQ(MatchErrorCode::ERROR, rc);  // because rates vector empty

  // 1 packet per second, burst size of 2 packets
  Meter::rate_config_t one_rate = {0.000001, 2};
  rates.push_back(one_rate);
  rc = this->table->set_meter_rates(handle, rates);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  Meter::reset_global_clock();

  output.clear();

  std::vector<int> int_ms = {10, 10, 1100, 0};

  for (int ms : int_ms) {
    this->table->apply_action(&pkt);
    Meter::color_t color = f_c.get_int();
    output.push_back(color);
    next_stop += milliseconds(ms);
    sleep_until(next_stop);
  }

  expected = {GREEN, GREEN, RED, GREEN};
  ASSERT_EQ(expected, output);

  rc = this->table->set_meter_rates(bad_handle, rates);
  ASSERT_EQ(MatchErrorCode::INVALID_HANDLE, rc);
}

TYPED_TEST(TableSizeTwo, Valid) {
  std::string key_ = "\x0a\xba";
  bool valid = true;
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  rc = this->add_entry_w_valid(key_, valid, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table_w_valid->get_num_entries());

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");
  Header &h2 = pkt.get_phv()->get_header(this->testHeader2);
  EXPECT_TRUE(h2.is_valid());

  h2.mark_invalid();

  this->table_w_valid->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);

  h2.mark_valid();

  this->table_w_valid->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}

TYPED_TEST(TableSizeTwo, ResetState) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);

  this->table->reset_state();
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::INVALID_HANDLE, rc);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}

TYPED_TEST(TableSizeTwo, HandleIterator) {
  entry_handle_t handle_1, handle_2;
  MatchErrorCode rc;

  rc = this->add_entry("\x0a\xba", &handle_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = this->add_entry("\x12\x34", &handle_2);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  auto it = this->table->handles_begin();
  ASSERT_EQ(handle_1, *it);
  ASSERT_EQ(handle_1, *it++);
  ASSERT_EQ(handle_2, *it);
  ASSERT_EQ(this->table->handles_end(), ++it);
}

TYPED_TEST(TableSizeTwo, GetEntries) {
  MatchErrorCode rc;
  std::vector<entry_handle_t> handles;
  std::vector<std::string> keys;

  size_t num_entries = 2;
  for (size_t e = 0; e < num_entries; e++) {
    keys.emplace_back(2, static_cast<char>(e + 1));
    handles.push_back(0);
    rc = this->add_entry(keys.back(), &handles.back());
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  }

  const auto entries = this->table->get_entries();

  ASSERT_EQ(num_entries, entries.size());

  for (size_t i = 0; i < num_entries; i++) {
    const auto &e = entries[i];
    ASSERT_EQ(handles[i], e.handle);
    ASSERT_EQ(keys[i], e.match_key[0].key);
    ASSERT_EQ(&this->action_fn, e.action_fn);
    ASSERT_EQ(0u, e.action_data.size());
  }
}


class TableIndirect : public ::testing::Test {
 protected:
  typedef MatchTableIndirect::mbr_hdl_t mbr_hdl_t;

 protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTableIndirect> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  DummyNode node_miss_default{};

  static const size_t table_size = 128;

  TableIndirect()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::EXACT);

    // with counters, without ageing
    table = MatchTableIndirect::create("exact", "test_table", 0,
                                       table_size, key_builder,
                                       &lookup_factory,
                                       true, false);
    table->set_next_node(0, nullptr);
    table->set_next_node_miss_default(&node_miss_default);
  }

  MatchErrorCode add_entry(const std::string &key,
			   mbr_hdl_t mbr,
			   entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
    return table->add_entry(match_key, mbr, handle);
  }

  MatchErrorCode add_member(unsigned int data, mbr_hdl_t *mbr) {
    ActionData action_data;
    action_data.push_back_action_data(data);
    return table->add_member(&action_fn, std::move(action_data), mbr);
  }

  MatchErrorCode modify_member(mbr_hdl_t mbr, unsigned int data) {
    ActionData action_data;
    action_data.push_back_action_data(data);
    return table->modify_member(mbr, &action_fn, std::move(action_data));
  }

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        length, PacketBuffer(length * 2), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    // return std::move(packet);
    // enable NVRO
    return packet;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

TEST_F(TableIndirect, AddMember) {
  MatchErrorCode rc;
  mbr_hdl_t mbr;
  size_t num_mbrs = 128;

  for(unsigned int i = 0; i < num_mbrs; i++) {
    rc = add_member(i, &mbr);
    ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
    // this is implementation specific, is it a good idea ?
    ASSERT_EQ(i, mbr);
    ASSERT_EQ(i + 1, table->get_num_members());
  }
}

TEST_F(TableIndirect, AddEntry) {
  MatchErrorCode rc;
  mbr_hdl_t mbr_1;
  mbr_hdl_t mbr_2 = 999u;
  entry_handle_t handle;
  std::string key("\x00\x00");
  unsigned int data = 666u;

  rc = add_member(data, &mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_entry(key, mbr_2, &handle);
  ASSERT_EQ(MatchErrorCode::INVALID_MBR_HANDLE, rc);
  ASSERT_EQ(0u, table->get_num_entries());

  static_assert(table_size < 255, "");

  for(unsigned int i = 0; i < table_size; i++) {
    key = std::string(2, (char) i);
    rc = add_entry(key, mbr_1, &handle);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
    rc = add_entry(key, mbr_1, &handle);
    ASSERT_EQ(MatchErrorCode::DUPLICATE_ENTRY, rc);
    // this is implementation specific, is it a good idea ?
    ASSERT_EQ(i, handle);
    ASSERT_EQ(i + 1, table->get_num_entries());
  }

  rc = add_entry("\xff\xff", mbr_1, &handle);
  ASSERT_EQ(MatchErrorCode::TABLE_FULL, rc);
}

TEST_F(TableIndirect, DeleteMember) {
  std::string key("\x0a\xba");
  MatchErrorCode rc;
  entry_handle_t handle;
  mbr_hdl_t mbr;

  rc = add_member(0xaba, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  // this is implementation specific, is it a good idea ?
  ASSERT_EQ(0u, mbr);
  ASSERT_EQ(1u, table->get_num_members());

  rc = table->delete_member(mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(0u, table->get_num_members());

  rc = add_member(0xabb, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  // this is implementation specific, is it a good idea ?
  ASSERT_EQ(0u, mbr);
  ASSERT_EQ(1u, table->get_num_members());

  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_member(mbr);
  ASSERT_EQ(rc, MatchErrorCode::MBR_STILL_USED);

  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_member(mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
}

TEST_F(TableIndirect, ModifyEntry) {
  MatchErrorCode rc;
  mbr_hdl_t mbr_1, mbr_2;
  mbr_hdl_t mbr_3 = 999u;
  entry_handle_t handle;
  std::string key = "\x0a\xba";
  unsigned int data = 666u;

  rc = add_member(data, &mbr_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_member(data, &mbr_2);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_entry(key, mbr_1, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = table->modify_entry(handle, mbr_2);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = table->modify_entry(handle, mbr_3);
  ASSERT_EQ(rc, MatchErrorCode::INVALID_MBR_HANDLE);
}

TEST_F(TableIndirect, LookupEntry) {
  std::string key = "\x0a\xba";
  std::string nkey = "\x0a\xbb";
  unsigned int data = 666u;
  mbr_hdl_t mbr;
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = get_pkt(64);
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  rc = add_member(data, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  const ActionEntry &entry = table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_EQ(data, entry.action_fn.get_action_data_at(0).get_uint());

  f.set("0xabb");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);

  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
}

// this test can probbaly be improved quite a lot by set a default member
// properly
TEST_F(TableIndirect, NextNodeHitMiss) {
  std::string key = "\x0a\xba";
  unsigned int data = 666u;
  mbr_hdl_t mbr;
  entry_handle_t handle;
  bool hit;

  Packet pkt = get_pkt(64);
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  auto add_member_and_entry = [this, &mbr, &handle](const std::string &key,
                                                    unsigned int data) {
    MatchErrorCode rc;
    rc = add_member(data, &mbr);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
    rc = add_entry(key, mbr, &handle);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  };

  auto delete_entry_and_member = [this, &mbr, &handle]() {
    MatchErrorCode rc;
    rc = table->delete_entry(handle);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
    rc = table->delete_member(mbr);
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  };

  // before adding member
  // miss, next node is default one (as per the P4 program)
  f.set("0xaba");
  ASSERT_EQ(&node_miss_default, table->apply_action(&pkt));

  add_member_and_entry(key, data);

  // base case
  // hit, next node set to nullptr
  f.set("0xaba");
  ASSERT_EQ(nullptr, table->apply_action(&pkt));
  // miss, next node is default one (as per the P4 program)
  f.set("0xabb");
  ASSERT_EQ(&node_miss_default, table->apply_action(&pkt));

  // need to remove entry because the node pointer is inserted in the entry when
  // the entry is added
  delete_entry_and_member();

  DummyNode dummy_node_hit, dummy_node_miss;
  table->set_next_node_hit(&dummy_node_hit);
  table->set_next_node_miss(&dummy_node_miss);

  add_member_and_entry(key, data);

  f.set("0xaba");
  ASSERT_EQ(&dummy_node_hit, table->apply_action(&pkt));
  f.set("0xabb");
  ASSERT_EQ(&dummy_node_miss, table->apply_action(&pkt));

  delete_entry_and_member();
}

TEST_F(TableIndirect, ModifyMember) {
  MatchErrorCode rc;
  mbr_hdl_t mbr;
  unsigned int data_1 = 0xaba;
  unsigned int data_2 = 0xabb;
  std::string key = "\x0a\xba";
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = get_pkt(64);
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  // I don't have a way of inspecting a member (yet?), so I perform a lookup
  // instead
  rc = add_member(data_1, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  const ActionEntry &entry_1 = table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_EQ(data_1, entry_1.action_fn.get_action_data_at(0).get_uint());

  rc = modify_member(mbr, data_2);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  const ActionEntry &entry_2 = table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_EQ(data_2, entry_2.action_fn.get_action_data_at(0).get_uint());
}

TEST_F(TableIndirect, ResetState) {
  std::string key = "\x0a\xba";
  std::string nkey = "\x0a\xbb";
  unsigned int data = 666u;
  mbr_hdl_t mbr;
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = get_pkt(64);
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);
  f.set("0xaba");

  rc = add_member(data, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);

  table->reset_state();
  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::INVALID_HANDLE, rc);
  rc = table->delete_member(mbr);
  ASSERT_EQ(MatchErrorCode::INVALID_MBR_HANDLE, rc);

  rc = add_member(data, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}

TEST_F(TableIndirect, GetEntries) {
  MatchErrorCode rc;
  std::vector<mbr_hdl_t> mbrs;
  std::vector<entry_handle_t> handles;
  std::vector<std::string> keys;
  std::vector<unsigned int> datas;

  size_t num_mbrs = 16;
  for (size_t m = 0; m < num_mbrs; m++) {
    mbrs.push_back(0);
    datas.push_back(m);
    rc = add_member(datas.back(), &mbrs.back());
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  }

  size_t num_entries = 64;
  for (size_t e = 0; e < num_entries; e++) {
    keys.emplace_back(2, static_cast<char>(e + 1));
    handles.push_back(0);
    rc = add_entry(keys.back(), mbrs[e % num_mbrs], &handles.back());
    ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  }

  const auto entries = table->get_entries();
  const auto members = table->get_members();

  ASSERT_EQ(num_entries, entries.size());
  ASSERT_EQ(num_mbrs, members.size());

  for (size_t i = 0; i < num_entries; i++) {
    const auto &e = entries[i];
    ASSERT_EQ(handles[i], e.handle);
    ASSERT_EQ(keys[i], e.match_key[0].key);
    ASSERT_EQ(mbrs[i % num_mbrs], e.mbr);
    ASSERT_EQ(-1, e.priority);
  }

  for (size_t i = 0; i < num_mbrs; i++) {
    const auto &m = members[i];
    ASSERT_EQ(mbrs[i], m.mbr);
    ASSERT_EQ(&action_fn, m.action_fn);
    ASSERT_EQ(1u, m.action_data.size());
    ASSERT_EQ(datas[i], m.action_data.action_data[0].get<unsigned int>());
  }
}


class TableIndirectWS : public ::testing::Test {
 protected:
  typedef MatchTableIndirect::mbr_hdl_t mbr_hdl_t;
  typedef MatchTableIndirectWS::grp_hdl_t grp_hdl_t;

 protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTableIndirectWS> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  // I am not seeding this on purpose, at least for now
  std::mt19937 gen;
  std::uniform_int_distribution<unsigned int> dis;

  static const size_t table_size = 128;

  TableIndirectWS()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::EXACT);

    // with counters, without ageing
    table = MatchTableIndirectWS::create("exact", "test_table", 0,
                                         table_size, key_builder,
                                         &lookup_factory,
                                         true, false);
    table->set_next_node(0, nullptr);

    BufBuilder builder;

    builder.push_back_field(testHeader1, 1); // h1.f48
    builder.push_back_field(testHeader2, 0); // h2.f16
    builder.push_back_field(testHeader2, 1); // h2.f48

    std::unique_ptr<Calculation> calc(new Calculation(builder, "xxh64"));

    table->set_hash(std::move(calc));
  }

  MatchErrorCode add_entry(const std::string &key,
  			   mbr_hdl_t mbr,
  			   entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
    return table->add_entry(match_key, mbr, handle);
  }

  MatchErrorCode add_entry_ws(const std::string &key,
			      grp_hdl_t grp,
			      entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
    return table->add_entry_ws(match_key, grp, handle);
  }

  MatchErrorCode add_member(unsigned int data, mbr_hdl_t *mbr) {
    ActionData action_data;
    action_data.push_back_action_data(data);
    return table->add_member(&action_fn, std::move(action_data), mbr);
  }

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        length, PacketBuffer(length * 2), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    // return std::move(packet);
    // enable NVRO
    return packet;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

TEST_F(TableIndirectWS, Group) {
  MatchErrorCode rc;
  grp_hdl_t grp;
  mbr_hdl_t mbr_1;
  mbr_hdl_t mbr_bad = 999u;
  mbr_hdl_t grp_bad = 999u;
  size_t num_mbrs;
  unsigned int data = 666u;

  ASSERT_EQ(0u, table->get_num_groups());

  rc = table->create_group(&grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(1u, table->get_num_groups());

  rc = table->get_num_members_in_group(grp, &num_mbrs);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(0u, num_mbrs);

  rc = add_member(data, &mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->add_member_to_group(mbr_1, grp_bad);
  ASSERT_EQ(MatchErrorCode::INVALID_GRP_HANDLE, rc);

  rc = table->add_member_to_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->add_member_to_group(mbr_bad, grp);
  ASSERT_EQ(MatchErrorCode::INVALID_MBR_HANDLE, rc);

  rc = table->get_num_members_in_group(grp, &num_mbrs);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(1u, num_mbrs);

  rc = table->add_member_to_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::MBR_ALREADY_IN_GRP, rc);

  rc = table->get_num_members_in_group(grp, &num_mbrs);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(1u, num_mbrs);

  rc = table->delete_member(mbr_1);
  ASSERT_EQ(MatchErrorCode::MBR_STILL_USED, rc);

  rc = table->remove_member_from_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->get_num_members_in_group(grp, &num_mbrs);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(0u, num_mbrs);

  rc = table->remove_member_from_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::MBR_NOT_IN_GRP, rc);

  rc = table->delete_member(mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_group(grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  ASSERT_EQ(0u, table->get_num_groups());

  rc = table->delete_group(grp);
  ASSERT_EQ(MatchErrorCode::INVALID_GRP_HANDLE, rc);
}

TEST_F(TableIndirectWS, EntryWS) {
  MatchErrorCode rc;
  grp_hdl_t grp;
  mbr_hdl_t mbr_1;
  mbr_hdl_t mbr_bad = 999u;
  mbr_hdl_t grp_bad = 999u;
  entry_handle_t handle;
  std::string key = "\x0a\xba";
  unsigned int data = 666u;

  rc = table->create_group(&grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_entry_ws(key, grp, &handle);
  ASSERT_EQ(MatchErrorCode::EMPTY_GRP, rc);

  rc = add_member(data, &mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->add_member_to_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_entry_ws(key, grp, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_group(grp);
  ASSERT_EQ(MatchErrorCode::GRP_STILL_USED, rc);

  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_group(grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
}

TEST_F(TableIndirectWS, LookupEntryWS) {
  MatchErrorCode rc;
  grp_hdl_t grp;
  mbr_hdl_t mbr_1, mbr_2;
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;
  std::string key = "\x0a\xba";
  unsigned int data_1 = 666u;
  unsigned int data_2 = 777u;

  rc = table->create_group(&grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_member(data_1, &mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = table->add_member_to_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_entry_ws(key, grp, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  Packet pkt = get_pkt(64);
  Field &h1_f16 = pkt.get_phv()->get_field(testHeader1, 0);
  Field &h1_f48 = pkt.get_phv()->get_field(testHeader1, 1);
  Field &h2_f16 = pkt.get_phv()->get_field(testHeader2, 0);
  Field &h2_f48 = pkt.get_phv()->get_field(testHeader2, 1);

  h1_f16.set("0xaba");

  // do a few lookups, should always resolve to mbr_1
  for(int i = 0; i < 16; i++) {
    h1_f48.set(dis(gen));
    h2_f16.set(dis(gen));
    h2_f48.set(dis(gen));
    const ActionEntry &entry = table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_TRUE(hit);
    ASSERT_EQ(data_1, entry.action_fn.get_action_data_at(0).get_uint());
  }

  // add a second member
  rc = add_member(data_2, &mbr_2);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = table->add_member_to_group(mbr_2, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  // do a few lookups, should resolve to 1 or 2 with equal probability
  size_t hits_1 = 0;
  size_t hits_2 = 0;
  for(int i = 0; i < 2000; i++) {
    h1_f48.set(dis(gen));
    h2_f16.set(dis(gen));
    h2_f48.set(dis(gen));
    const ActionEntry &entry = table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_TRUE(hit);
    unsigned int data = entry.action_fn.get_action_data_at(0).get_uint();
    if (data == data_1) hits_1++;
    else if (data == data_2) hits_2++;
    else FAIL();
  }

  ASSERT_NEAR(hits_1, hits_2, 100);

  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_group(grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_member(mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = table->delete_member(mbr_2);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
}

TEST_F(TableIndirectWS, GetEntries) {
  MatchErrorCode rc;
  grp_hdl_t grp;
  mbr_hdl_t mbr_1, mbr_2;
  entry_handle_t handle_1, handle_2;
  std::string key_1 = "\x0a\xba";
  std::string key_2 = "\x0a\xbb";
  unsigned int data_1 = 666u;
  unsigned int data_2 = 777u;

  rc = table->create_group(&grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_member(data_1, &mbr_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = add_member(data_2, &mbr_2);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = table->add_member_to_group(mbr_1, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = table->add_member_to_group(mbr_2, grp);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = add_entry_ws(key_1, grp, &handle_1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  rc = add_entry(key_2, mbr_1, &handle_2);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  const auto entries = table->get_entries();
  const auto groups = table->get_groups();
  const auto members = table->get_members();

  ASSERT_EQ(2u, entries.size());
  ASSERT_EQ(1u, groups.size());
  ASSERT_EQ(2u, members.size());

  const auto &e1 = entries[0];
  ASSERT_EQ(handle_1, e1.handle);
  ASSERT_EQ(key_1, e1.match_key[0].key);
  ASSERT_EQ(std::numeric_limits<mbr_hdl_t>::max(), e1.mbr);
  ASSERT_EQ(grp, e1.grp);
  ASSERT_EQ(-1, e1.priority);

  const auto &g1 = groups[0];
  ASSERT_EQ(grp, g1.grp);
  ASSERT_EQ(2u, g1.mbr_handles.size());
  ASSERT_EQ(mbr_1, g1.mbr_handles[0]);
  ASSERT_EQ(mbr_2, g1.mbr_handles[1]);

  const auto &m1 = members[0];
  ASSERT_EQ(mbr_1, m1.mbr);
  ASSERT_EQ(&action_fn, m1.action_fn);

  const auto &m2 = members[1];
  ASSERT_EQ(mbr_2, m2.mbr);

  const auto &e2 = entries[1];
  ASSERT_EQ(handle_2, e2.handle);
  ASSERT_EQ(key_2, e2.match_key[0].key);
  ASSERT_EQ(mbr_1, e2.mbr);
  ASSERT_EQ(std::numeric_limits<grp_hdl_t>::max(), e2.grp);
  ASSERT_EQ(-1, e2.priority);
}


template <typename MUType>
class TableBigMask : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  ByteContainer mask_2{"0x0000ff0000ff"};
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  void make_key_builder();

  TableBigMask()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    make_key_builder();

    std::unique_ptr<MUType> match_unit;

    // false: no counters
    match_unit = std::unique_ptr<MUType>(
        new MUType(2, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  MatchErrorCode add_entry(const std::string &key_1,
                           const std::string &key_2,
			   entry_handle_t *handle);

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        length, PacketBuffer(length * 2), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    // return std::move(packet);
    // enable NVRO
    return packet;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

template<>
MatchErrorCode
TableBigMask<MUExact>::add_entry(const std::string &key_1,
                                 const std::string &key_2,
				 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key_1);
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key_2);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
void
TableBigMask<MUExact>::make_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::EXACT);
  key_builder.push_back_field(testHeader2, 1, 48, mask_2,
                              MatchKeyParam::Type::EXACT);
}

template<>
MatchErrorCode
TableBigMask<MULPM>::add_entry(const std::string &key_1,
                               const std::string &key_2,
			       entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key_1, 16);
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key_2);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
void
TableBigMask<MULPM>::make_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::LPM);
  key_builder.push_back_field(testHeader2, 1, 48, mask_2,
                              MatchKeyParam::Type::EXACT);
}

template<>
MatchErrorCode
TableBigMask<MUTernary>::add_entry(const std::string &key_1,
                                   const std::string &key_2,
				   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask_1 = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY,
                         key_1, std::move(mask_1));
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key_2);
  return table->add_entry(match_key, &action_fn, action_data, handle, priority);
}

template<>
void
TableBigMask<MUTernary>::make_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::TERNARY);
  key_builder.push_back_field(testHeader2, 1, 48, mask_2,
                              MatchKeyParam::Type::EXACT);
}

template<>
MatchErrorCode
TableBigMask<MURange>::add_entry(const std::string &key_1,
                                   const std::string &key_2,
				   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  Data start_1(key_1.data(), key_1.size());
  Data end_1;
  // we create a range with 5 values
  end_1.add(start_1, Data(4));
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::RANGE, key_1, end_1.get_string());
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key_2);
  return table->add_entry(match_key, &action_fn, action_data, handle, priority);
}

template<>
void
TableBigMask<MURange>::make_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::RANGE);
  key_builder.push_back_field(testHeader2, 1, 48, mask_2,
                              MatchKeyParam::Type::EXACT);
}

TYPED_TEST_CASE(TableBigMask, TableTypes);

TYPED_TEST(TableBigMask, HitMiss) {
  const std::string key_1 = "\x11\x22";
  const std::string key_2 = "\x33\x44\x55\x66\x77\x88";

  entry_handle_t handle_1;
  entry_handle_t lookup_handle;
  bool hit;
  MatchErrorCode rc;

  rc = this->add_entry(key_1, key_2, &handle_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  Packet pkt = this->get_pkt(64);
  Field &f_1 = pkt.get_phv()->get_field(this->testHeader1, 0);
  Field &f_2 = pkt.get_phv()->get_field(this->testHeader2, 1);

  f_1.set(key_1.c_str(), key_1.size());
  f_2.set(key_2.c_str(), key_2.size());
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);

  const std::string key_2_hit = "\xaa\x44\x55\x66\x77\x88";
  f_2.set(key_2_hit.c_str(), key_2_hit.size());
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);

  const std::string key_2_miss = "\x33\x44\x55\x66\x77\xaa";
  f_2.set(key_2_miss.c_str(), key_2_miss.size());
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
}

class MatchKeyBuilderTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1}, testHeader3{2};

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  MatchKeyBuilderTest()
      : testHeaderType("test_t", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f17", 17);
    // the code requires headers to be byte aligned, thus f7
    testHeaderType.push_back_field("f7", 7);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
    phv_factory.push_back_header("test3", testHeader3, testHeaderType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  Packet get_pkt() const {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        128, PacketBuffer(256), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    packet.get_phv()->get_header(testHeader3).mark_valid();
    return packet;
  }

  // virtual void TearDown() { }
};

class MatchKeyBuilderTest1 : public MatchKeyBuilderTest {
 protected:
  virtual void SetUp() {
    MatchKeyBuilderTest::SetUp();

    key_builder.push_back_field(testHeader1, 0, 16,
                                MatchKeyParam::Type::EXACT);  // h1.f16
    key_builder.push_back_field(testHeader2, 2, 17,
                                MatchKeyParam::Type::LPM);  // h2.f17
    key_builder.push_back_field(testHeader2, 0, 16,
                                MatchKeyParam::Type::TERNARY);  // h2.f16
    key_builder.push_back_valid_header(testHeader3);

    key_builder.build();
  }

  // virtual void TearDown() { }

  Packet gen_pkt() const {
    Packet pkt = get_pkt();
    PHV *phv = pkt.get_phv();
    Field &h1_f16 = phv->get_field(testHeader1, 0);
    Field &h2_f17 = phv->get_field(testHeader2, 2);
    Field &h2_f16 = phv->get_field(testHeader2, 0);

    h1_f16.set("0xabcd");
    h2_f17.set("0x10044");
    h2_f16.set("0x7001");

    return pkt;
  }
};

TEST_F(MatchKeyBuilderTest1, KeyToString) {
  Packet pkt = gen_pkt();
  PHV *phv = pkt.get_phv();

  ByteContainer key;
  key_builder(*phv, &key);

  std::string s = key_builder.key_to_string(key, " ");
  ASSERT_EQ("abcd 010044 7001 01", s);
}

TEST_F(MatchKeyBuilderTest1, KeyToFields) {
  Packet pkt = gen_pkt();
  PHV *phv = pkt.get_phv();

  ByteContainer key;
  key_builder(*phv, &key);

  const auto v = key_builder.key_to_fields(key);
  std::vector<std::string> expected = {"\xab\xcd",
                                       // necessary because of null character
                                       std::string("\x01\x00\x44", 3),
                                       "\x70\x01", "\x01"};
  ASSERT_EQ(expected, v);
}


// added after exposing some hidden nasty bugs
class AdvancedTest : public ::testing::Test {
 protected:
  static constexpr size_t t_size = 128u;

  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1}, testHeader3{2};
  ActionFn action_fn;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  AdvancedTest()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f17", 17);
    testHeaderType.push_back_field("f7", 7);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
    phv_factory.push_back_header("test3", testHeader3, testHeaderType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  Packet get_pkt() const {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        128, PacketBuffer(256), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    packet.get_phv()->get_header(testHeader3).mark_valid();
    return packet;
  }

  // virtual void TearDown() { }
};

// was added after I discovered a bug in LPM match (when a table contains both a
// LPM match field and a valid match field)
class AdvancedLPMTest : public AdvancedTest {
 protected:
  AdvancedLPMTest()
      : AdvancedTest() {
    key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::LPM);
    key_builder.push_back_field(testHeader2, 0, 16, MatchKeyParam::Type::EXACT);
    key_builder.push_back_valid_header(testHeader3);

    std::unique_ptr<MULPM> match_unit;
    match_unit = std::unique_ptr<MULPM>(
        new MULPM(t_size, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    AdvancedTest::SetUp();
  }

  MatchErrorCode add_entry(entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    //10.00/12
    match_key.emplace_back(MatchKeyParam::Type::LPM,
                           std::string("\x0a\x00", 2), 12);
    match_key.emplace_back(MatchKeyParam::Type::EXACT,
                           std::string("\xab\xcd", 2));
    match_key.emplace_back(MatchKeyParam::Type::VALID, std::string("\x01", 1));
    return table->add_entry(match_key, &action_fn, ActionData(), handle);
  }

  Packet gen_pkt(const std::string &h1_f16_v,
                 const std::string &h2_f16_v) const {
    Packet packet = get_pkt();
    PHV *phv = packet.get_phv();
    Field &h1_f16 = phv->get_field(testHeader1, 0);
    Field &h2_f16 = phv->get_field(testHeader2, 0);

    h1_f16.set(h1_f16_v);
    h2_f16.set(h2_f16_v);

    return packet;
  }
};

// TODO(antonin): use value-parametrized test to cover every case?
TEST_F(AdvancedLPMTest, Lookup1) {
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;

  ASSERT_EQ(MatchErrorCode::SUCCESS, add_entry(&handle));

  Packet pkt = gen_pkt("0x0a00", "0xabcd");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}

TEST_F(AdvancedLPMTest, Lookup2) {
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;

  ASSERT_EQ(MatchErrorCode::SUCCESS, add_entry(&handle));

  Packet pkt = gen_pkt("0x0a0d", "0xabcd");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}

TEST_F(AdvancedLPMTest, Lookup3) {
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;

  ASSERT_EQ(MatchErrorCode::SUCCESS, add_entry(&handle));

  Packet pkt = gen_pkt("0x0ad0", "0xabcd");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
}


class AdvancedTernaryTest : public AdvancedTest {
 protected:
  AdvancedTernaryTest()
      : AdvancedTest() {
    key_builder.push_back_field(testHeader1, 0, 16,
                                MatchKeyParam::Type::TERNARY);
    key_builder.push_back_field(testHeader2, 0, 16, MatchKeyParam::Type::EXACT);
    key_builder.push_back_valid_header(testHeader3);

    std::unique_ptr<MUTernary> match_unit;
    match_unit = std::unique_ptr<MUTernary>(
        new MUTernary(t_size, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    AdvancedTest::SetUp();
  }

  MatchErrorCode add_entry(entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    //10.00/12
    match_key.emplace_back(MatchKeyParam::Type::TERNARY,
                           std::string("\x12\x34", 2),
                           std::string("\x0f\xf0", 2));
    match_key.emplace_back(MatchKeyParam::Type::EXACT,
                           std::string("\xab\xcd", 2));
    match_key.emplace_back(MatchKeyParam::Type::VALID, std::string("\x01", 1));
    return table->add_entry(match_key, &action_fn, ActionData(), handle);
  }

  Packet gen_pkt(const std::string &h1_f16_v,
                 const std::string &h2_f16_v) const {
    Packet packet = get_pkt();
    PHV *phv = packet.get_phv();
    Field &h1_f16 = phv->get_field(testHeader1, 0);
    Field &h2_f16 = phv->get_field(testHeader2, 0);

    h1_f16.set(h1_f16_v);
    h2_f16.set(h2_f16_v);

    return packet;
  }
};

TEST_F(AdvancedTernaryTest, Lookup1) {
  entry_handle_t handle;
  entry_handle_t lookup_handle;
  bool hit;

  ASSERT_EQ(MatchErrorCode::SUCCESS, add_entry(&handle));

  Packet pkt = gen_pkt("0x5235", "0xabcd");
  // Packet pkt = gen_pkt("0xabcd", "0x5235");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}


class HiddenValidFieldTest : public AdvancedTest {
 protected:
  HiddenValidFieldTest()
      : AdvancedTest() {
    int offset = testHeaderType.get_hidden_offset(HeaderType::HiddenF::VALID);
    key_builder.push_back_field(testHeader1, offset, 1,
                                MatchKeyParam::Type::TERNARY);
    key_builder.push_back_field(testHeader2, 0, 16, MatchKeyParam::Type::EXACT);

    std::unique_ptr<MUTernary> match_unit;
    match_unit = std::unique_ptr<MUTernary>(
        new MUTernary(t_size, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    AdvancedTest::SetUp();
  }

  MatchErrorCode add_entry(const std::string &exact_v,
                           bool is_valid, bool is_masked,
                           entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    //10.00/12
    std::string valid_key = is_valid ?
        std::string("\x01", 1) : std::string("\x00", 1);
    std::string valid_mask = is_masked ?
        std::string("\x01", 1) : std::string("\x00", 1);
    match_key.emplace_back(MatchKeyParam::Type::TERNARY,
                           std::move(valid_key), std::move(valid_mask));
    match_key.emplace_back(MatchKeyParam::Type::EXACT, exact_v);
    return table->add_entry(match_key, &action_fn, ActionData(), handle);
  }

  Packet gen_pkt(const std::string &exact_v, bool is_valid) const {
    Packet packet = Packet::make_new(
        128, PacketBuffer(256), phv_source.get());
    PHV *phv = packet.get_phv();
    if (is_valid) phv->get_header(testHeader1).mark_valid();
    phv->get_header(testHeader2).mark_valid();
    Field &h2_f16 = phv->get_field(testHeader2, 0);
    h2_f16.set(exact_v.data(), exact_v.size());
    return packet;
  }
};

TEST_F(HiddenValidFieldTest, Lookup) {
  entry_handle_t h1, h2;
  entry_handle_t lookup_handle;
  bool hit;
  const std::string exact_v_1("\xab\x15", 2);
  const std::string exact_v_2("\xab\xcd", 2);

  // needs header1 to be valid
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            add_entry(exact_v_1, true, true, &h1));
  // do not care about header1's validity
  ASSERT_EQ(MatchErrorCode::SUCCESS,
            add_entry(exact_v_2, true, false, &h2));

  {
    auto pkt = gen_pkt(exact_v_1, false);
    table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_FALSE(hit);
  }
  {
    auto pkt = gen_pkt(exact_v_2, false);
    table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_TRUE(hit); ASSERT_EQ(h2, lookup_handle);
  }
  {
    auto pkt = gen_pkt(exact_v_1, true);
    table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_TRUE(hit); ASSERT_EQ(h1, lookup_handle);
  }
  {
    auto pkt = gen_pkt(exact_v_2, true);
    table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_TRUE(hit); ASSERT_EQ(h2, lookup_handle);
  }
  {
    auto pkt = gen_pkt(exact_v_1, true);
    pkt.get_phv()->get_header(testHeader1).mark_invalid();
    table->lookup(pkt, &hit, &lookup_handle);
    ASSERT_FALSE(hit);
  }
}


template <typename MUType>
class TableEntryDebug : public ::testing::Test {
 protected:
  static constexpr size_t t_size = 128u;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1}, testHeader3{2};
  ActionFn action_fn;
  unsigned int action_data_v = 0xaba;
  ActionData action_data;
  int priority = 12;

  TableEntryDebug()
      : testHeaderType("test_t", 0), action_fn("actionA", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f17", 17);
    testHeaderType.push_back_field("f7", 7);

    action_data.push_back_action_data(action_data_v);

    std::unique_ptr<MUType> match_unit;

    set_key_builder();

    // true enables counters
    match_unit = std::unique_ptr<MUType>(
        new MUType(t_size, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  std::vector<MatchKeyParam> gen_match_key(const char * key1="\x12\x34",
                                           const char * key2="\xab\xcd") const;

  std::string gen_entry_string() const;

  MatchErrorCode add_entries() {
    entry_handle_t handle;
    MatchErrorCode rc1 =
    table->add_entry(gen_match_key("\x12\x34","\xab\xcd"), &action_fn,
                     action_data, &handle, priority);
    if (rc1 != MatchErrorCode::SUCCESS) return rc1;

    MatchErrorCode rc2 =
    table->add_entry(gen_match_key("\x34\x56","\xcd\xef"), &action_fn,
                     action_data, &handle, priority);
    if (rc2 != MatchErrorCode::SUCCESS) return rc2;

    MatchErrorCode rc3 =
    table->add_entry(gen_match_key("\x56\x78","\xfe\xdc"), &action_fn,
                     action_data, &handle, priority);
    if (rc3 != MatchErrorCode::SUCCESS) return rc3;

    MatchErrorCode rc4 =
    table->add_entry(gen_match_key("\x78\x90","\xdc\xba"), &action_fn,
                     action_data, &handle, priority);
    return rc4;

  }

  MatchErrorCode add_entry(entry_handle_t *handle) {
    return table->add_entry(gen_match_key(), &action_fn, action_data, handle,
                            priority);
  }

 private:
  void set_key_builder();
};

template <>
void TableEntryDebug<MUExact>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16,
                              MatchKeyParam::Type::EXACT, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 16,
                              MatchKeyParam::Type::EXACT, "h2.f0");
  key_builder.push_back_valid_header(testHeader3, "h3");
}

template <>
void TableEntryDebug<MULPM>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16,
                              MatchKeyParam::Type::LPM, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 16,
                              MatchKeyParam::Type::EXACT, "h2.f0");
  key_builder.push_back_valid_header(testHeader3, "h3");
}

template <>
void TableEntryDebug<MUTernary>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16,
                              MatchKeyParam::Type::LPM, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 16,
                              MatchKeyParam::Type::TERNARY, "h2.f0");
  key_builder.push_back_valid_header(testHeader3, "h3");
}

template <>
void TableEntryDebug<MURange>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 16,
                              MatchKeyParam::Type::LPM, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 16,
                              MatchKeyParam::Type::RANGE, "h2.f0");
  key_builder.push_back_valid_header(testHeader3, "h3");
}

template <>
std::vector<MatchKeyParam>
TableEntryDebug<MUExact>::gen_match_key(const char *k1, const char *k2) const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT,
                         std::string(k1, 2));
  match_key.emplace_back(MatchKeyParam::Type::EXACT,
                         std::string(k2, 2));
  match_key.emplace_back(MatchKeyParam::Type::VALID, std::string("\x01", 1));
  return match_key;
}

template <>
std::vector<MatchKeyParam>
TableEntryDebug<MULPM>::gen_match_key(const char *k1, const char *k2) const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM,
                         std::string(k1, 2), 12);
  match_key.emplace_back(MatchKeyParam::Type::EXACT,
                         std::string(k2, 2));
  match_key.emplace_back(MatchKeyParam::Type::VALID, std::string("\x01", 1));
  return match_key;
}

template <>
std::vector<MatchKeyParam>
TableEntryDebug<MUTernary>::gen_match_key(const char *k1, const char *k2) const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM,
                         std::string(k1, 2), 12);
  match_key.emplace_back(MatchKeyParam::Type::TERNARY,
                         std::string(k2, 2),
                         std::string("\x0f\xf0", 2));
  match_key.emplace_back(MatchKeyParam::Type::VALID, std::string("\x01", 1));
  return match_key;
}

template <>
std::vector<MatchKeyParam>
TableEntryDebug<MURange>::gen_match_key(const char *k1, const char *k2) const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM,
                         std::string(k1, 2), 12);
  Data start(k2, 2);
  Data end;
  // we create a range with 5 values
  end.add(start, Data(4));
  match_key.emplace_back(MatchKeyParam::Type::RANGE,
                         std::string(k2, 2), end.get_string());
  match_key.emplace_back(MatchKeyParam::Type::VALID, std::string("\x01", 1));
  return match_key;
}

// not very resistant...
template <>
std::string TableEntryDebug<MUExact>::gen_entry_string() const {
  return std::string(
      "Dumping entry 0\n"
      "Match key:\n"
      "* h1.f0               : EXACT     1234\n"
      "* h2.f0               : EXACT     abcd\n"
      "* h3                  : VALID     01\n"
      "Action entry: actionA - aba,\n");
}

template <>
std::string TableEntryDebug<MULPM>::gen_entry_string() const {
  return std::string(
      "Dumping entry 0\n"
      "Match key:\n"
      "* h1.f0               : LPM       1234/12\n"
      "* h2.f0               : EXACT     abcd\n"
      "* h3                  : VALID     01\n"
      "Action entry: actionA - aba,\n");
}

template <>
std::string TableEntryDebug<MUTernary>::gen_entry_string() const {
  return std::string(
      "Dumping entry 0\n"
      "Match key:\n"
      "* h1.f0               : LPM       1230/12\n"
      "* h2.f0               : TERNARY   0bc0 &&& 0ff0\n"
      "* h3                  : VALID     01\n"
      "Priority: 12\n"
      "Action entry: actionA - aba,\n");
}

template <>
std::string TableEntryDebug<MURange>::gen_entry_string() const {
  return std::string(
      "Dumping entry 0\n"
      "Match key:\n"
      "* h1.f0               : LPM       1230/12\n"
      "* h2.f0               : RANGE     abcd -> abd1\n"
      "* h3                  : VALID     01\n"
      "Priority: 12\n"
      "Action entry: actionA - aba,\n");
}

TYPED_TEST_CASE(TableEntryDebug, TableTypes);

namespace {

// I initially overloaded the == operator for MatchKeyParam, but it was
// polluting the bm namespace

bool cmp_LPM(const std::string &k1, const std::string &k2, int pref) {
  if (k1.size() != k2.size()) return false;
  if (k1.compare(0, pref / 8, k2, 0, pref / 8) != 0) return false;
  if (pref % 8 != 0) {
    char mask = static_cast<char>(0xff) << (8 - (pref % 8));
    if ((k1[pref / 8] & mask) != (k2[pref / 8] & mask)) return false;
  }
  return true;
}

bool cmp_ternary(const std::string &k1, const std::string &k2,
                 const std::string &mask) {
  if (k1.size() != k2.size()) return false;
  for (size_t i = 0; i < k1.size(); i++) {
    if ((k1[i] & mask[i]) != (k2[i] & mask[i])) return false;
  }
  return true;
}

bool cmp_match_params(const MatchKeyParam &p1, const MatchKeyParam &p2) {
  if (p1.type != p2.type) return false;
  switch (p1.type) {
    case MatchKeyParam::Type::EXACT:
    case MatchKeyParam::Type::VALID:
      return (p1.key == p2.key);
    case MatchKeyParam::Type::LPM:
      return (p1.prefix_length == p2.prefix_length &&
              cmp_LPM(p1.key, p2.key, p1.prefix_length));
    case MatchKeyParam::Type::TERNARY:
      return (p1.mask == p2.mask &&
              cmp_ternary(p1.key, p2.key, p1.mask));
    case MatchKeyParam::Type::RANGE:
      return (p1.key == p2.key && p1.mask == p2.mask);
  }
  return true;
}

bool cmp_match_keys(const std::vector<MatchKeyParam> &k1,
                    const std::vector<MatchKeyParam> &k2) {
  if (k1.size() != k2.size()) return false;
  for (size_t i = 0; i < k1.size(); i++) {
    if (!cmp_match_params(k1[i], k2[i])) return false;
  }
  return true;
}

}  // namespace

TYPED_TEST(TableEntryDebug, GetEntry) {
  entry_handle_t handle;
  ASSERT_EQ(MatchErrorCode::SUCCESS, this->add_entry(&handle));

  MatchTable::Entry entry;
  this->table->get_entry(handle, &entry);
  ASSERT_TRUE(cmp_match_keys(this->gen_match_key(), entry.match_key));
  ASSERT_EQ(this->action_data.get(0), entry.action_data.get(0));
}

TYPED_TEST(TableEntryDebug, DumpEntry) {
  entry_handle_t handle;
  entry_handle_t bad_handle = 99;
  ASSERT_EQ(MatchErrorCode::SUCCESS, this->add_entry(&handle));

  std::stringstream os;
  this->table->dump_entry(&os, handle);
  // std::cout << os.str();
  ASSERT_EQ(this->gen_entry_string(), os.str());

  ASSERT_EQ(this->gen_entry_string(), this->table->dump_entry_string(handle));

  ASSERT_NE(MatchErrorCode::SUCCESS, this->table->dump_entry(&os, bad_handle));

  ASSERT_EQ("", this->table->dump_entry_string(bad_handle));
}


// added after a deadlock was found because of shared mutex acquired multiple
// times by MatchTable::dump_entry_string()
class TableDeadlock : public ::testing::Test {
 protected:
  static constexpr size_t t_size = 128u;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0};
  ActionFn action_fn;
  unsigned int action_data_v = 0xaba;
  ActionData action_data;

  PHVFactory phv_factory;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  TableDeadlock()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);

    action_data.push_back_action_data(action_data_v);

    key_builder.push_back_field(testHeader1, 0, 16,
                                MatchKeyParam::Type::EXACT, "h1.f0");
    LookupStructureFactory factory;
    std::unique_ptr<MUExact> match_unit(new MUExact(t_size, key_builder, &factory));
    table = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit), false)
    );
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }
};

extern bool WITH_VALGRIND; // defined in main.cpp

// used to fail because of deadlock in dump_entry_string (when called from
// apply_action); could only be observed when bmv2 was compiled with
// -DBMLOG_DEBUG_ON
TEST_F(TableDeadlock, DumpEntryString) {
  auto modify_loop = [this](size_t iters, entry_handle_t handle) {
    for (size_t i = 0; i < iters; i++) {
      MatchErrorCode rc = table->modify_entry(handle, &action_fn, action_data);
      ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
    }
  };

  auto apply_loop = [this](size_t iters) {
    Packet packet = Packet::make_new(phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_field(testHeader1, 0).set("0xabcd");
    for (size_t i = 0; i < iters; i++) {
      table->apply_action(&packet);
    }
  };

  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT,
                         std::string("\xab\xcd", 2));
  entry_handle_t handle;
  MatchErrorCode rc;
  rc = table->add_entry(match_key, &action_fn, action_data, &handle, 1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  const size_t iterations = WITH_VALGRIND ? 100u : 100000u;

  std::thread t1(modify_loop, iterations, handle);
  std::thread t2(apply_loop, iterations);

  auto timeout = std::chrono::seconds(30);

  auto future = std::async(std::launch::async, &std::thread::join, &t2);
  if (future.wait_for(timeout) == std::future_status::timeout) {
    // thread has not terminated before timeout
    ADD_FAILURE() << "Timeout expired\n";
    // TODO(antonin): do something cleaner
    std::terminate();
  } else {
    t1.join();
  }
}


// We recently added automatic masking of the first byte of each match
// param. For example, if a match field is 14 bit wide and we receive value
// 0xffff from the client (instead of the correct 0x3fff), we will automatically
// do the conversion. Before that change, we would not perform any checks and
// simply use the user-provided value, which would cause unexpected dataplane
// behavior. But is this silent conversion better than returning an error to the
// client?
template <typename MUType>
class TableBadInputKey : public ::testing::Test {
 protected:
  static constexpr size_t t_size = 128u;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1}, testHeader3{2};
  ActionFn action_fn;
  ActionData action_data;
  int priority = 12;

  PHVFactory phv_factory;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  TableBadInputKey()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    // non-aligned, to expose potential issues
    testHeaderType.push_back_field("f14", 14);
    // for header byte alignment
    testHeaderType.push_back_field("f2", 2);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
    phv_factory.push_back_header("test3", testHeader3, testHeaderType);

    std::unique_ptr<MUType> match_unit;

    set_key_builder();

    match_unit = std::unique_ptr<MUType>(
        new MUType(t_size, key_builder, &lookup_factory));
    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  std::vector<MatchKeyParam> gen_match_key() const;

  MatchErrorCode add_entry(entry_handle_t *handle) {
    return table->add_entry(gen_match_key(), &action_fn, action_data, handle,
                            priority);
  }

  Packet get_pkt() const {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(
        128, PacketBuffer(256), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    packet.get_phv()->get_header(testHeader3).mark_valid();
    return packet;
  }

  Packet gen_pkt(const std::string &h1_f14_v = "0x3fff",
                 const std::string &h2_f14_v = "0x3fff",
                 const std::string &h3_f14_v = "0x3fff") const {
    Packet packet = get_pkt();
    PHV *phv = packet.get_phv();
    Field &h1_f14 = phv->get_field(testHeader1, 0);
    Field &h2_f14 = phv->get_field(testHeader2, 0);
    Field &h3_f14 = phv->get_field(testHeader3, 0);

    h1_f14.set(h1_f14_v);
    h2_f14.set(h2_f14_v);
    h3_f14.set(h3_f14_v);

    return packet;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

 private:
  void set_key_builder();
};

template <>
void TableBadInputKey<MUExact>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 14,
                              MatchKeyParam::Type::EXACT, "h1.f0");
}

template <>
void TableBadInputKey<MULPM>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 14,
                              MatchKeyParam::Type::LPM, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 14,
                              MatchKeyParam::Type::EXACT, "h2.f0");
}

template <>
void TableBadInputKey<MUTernary>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 14,
                              MatchKeyParam::Type::LPM, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 14,
                              MatchKeyParam::Type::TERNARY, "h2.f0");
  key_builder.push_back_field(testHeader3, 0, 14,
                              MatchKeyParam::Type::EXACT, "h3.f0");
}

template <>
void TableBadInputKey<MURange>::set_key_builder() {
  key_builder.push_back_field(testHeader1, 0, 14,
                              MatchKeyParam::Type::LPM, "h1.f0");
  key_builder.push_back_field(testHeader2, 0, 14,
                              MatchKeyParam::Type::RANGE, "h2.f0");
  key_builder.push_back_field(testHeader3, 0, 14,
                              MatchKeyParam::Type::EXACT, "h3.f0");
}

template <>
std::vector<MatchKeyParam>
TableBadInputKey<MUExact>::gen_match_key() const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\xff\xff"));
  return match_key;
}

template <>
std::vector<MatchKeyParam>
TableBadInputKey<MULPM>::gen_match_key() const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM, std::string("\xff\xff"), 12);
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\xff\xff"));
  return match_key;
}

template <>
std::vector<MatchKeyParam>
TableBadInputKey<MUTernary>::gen_match_key() const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM, std::string("\xff\xff"), 12);
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, std::string("\xff\xff"),
                         std::string("\xff\xff"));
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\xff\xff"));
  return match_key;
}

template <>
std::vector<MatchKeyParam>
TableBadInputKey<MURange>::gen_match_key() const {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::LPM, std::string("\xff\xff"), 12);
  match_key.emplace_back(MatchKeyParam::Type::RANGE, std::string("\xff\x01"),
                         std::string("\xff\xff"));
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\xff\xff"));
  return match_key;
}

TYPED_TEST_CASE(TableBadInputKey, TableTypes);

TYPED_TEST(TableBadInputKey, NonByteAligned) {
  entry_handle_t handle, lookup_handle;
  bool hit;
  ASSERT_EQ(MatchErrorCode::SUCCESS, this->add_entry(&handle));
  auto pkt = this->gen_pkt();
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}


class TableRangeMatch : public ::testing::Test {
 protected:
  static constexpr size_t t_size = 1024u;

  PHVFactory phv_factory;

  std::unique_ptr<MatchTable> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  TableRangeMatch()
      : testHeaderType("test_t", 0), action_fn("actionA", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    Packet packet = Packet::make_new(128, PacketBuffer(256), phv_source.get());
    packet.get_phv()->get_header(testHeader1).mark_valid();
    packet.get_phv()->get_header(testHeader2).mark_valid();
    return packet;
  }

  void create_table(const MatchKeyBuilder &key_builder) {
    std::unique_ptr<MURange> match_unit(
        new MURange(t_size, key_builder, &lookup_factory));

    table = std::unique_ptr<MatchTable>(
        new MatchTable("test_table", 0, std::move(match_unit), false));
    table->set_next_node(0, nullptr);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

TEST_F(TableRangeMatch, OneRange) {
  {
    MatchKeyBuilder key_builder;
    key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::RANGE);
    create_table(key_builder);
  }

  MatchErrorCode rc;
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  unsigned int start = 0xab;
  unsigned int end = 0x041d;
  std::string start_s("\x00\xab", 2);
  std::string end_s("\x04\x1d", 2);
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::RANGE, start_s, end_s);
  entry_handle_t handle;
  rc = table->add_entry(match_key, &action_fn, std::move(action_data),
                        &handle, priority);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  auto check_one = [this](unsigned int v, bool expected) {
    bool hit;
    entry_handle_t h;
    Packet pkt = get_pkt();
    pkt.get_phv()->get_field(testHeader1, 0).set(v);
    table->lookup(pkt, &hit, &h);
    ASSERT_EQ(expected, hit);
  };

  for (unsigned int v = start; v < end; v++) check_one(v, true);
  for (unsigned int v = 0; v < start; v++) check_one(v, false);
  for (unsigned int v = end + 1; v < 0xffff; v++) check_one(v, false);
}

TEST_F(TableRangeMatch, TwoRanges) {
  {
    MatchKeyBuilder key_builder;
    key_builder.push_back_field(testHeader1, 0, 16, MatchKeyParam::Type::RANGE);
    key_builder.push_back_field(testHeader2, 0, 16, MatchKeyParam::Type::RANGE);
    create_table(key_builder);
  }

  MatchErrorCode rc;
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  unsigned int start_1 = 0xab, start_2 = 0x8888;
  unsigned int end_1 = 0x041d, end_2 = 0x8889;
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::RANGE,
                         std::string("\x00\xab", 2),
                         std::string("\x04\x1d", 2));
  match_key.emplace_back(MatchKeyParam::Type::RANGE,
                         std::string("\x88\x88", 2),
                         std::string("\x88\x89", 2));
  entry_handle_t handle;
  rc = table->add_entry(match_key, &action_fn, std::move(action_data),
                        &handle, priority);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  auto check_one = [this](unsigned int v1, unsigned int v2, bool expected) {
    bool hit;
    entry_handle_t h;
    Packet pkt = get_pkt();
    pkt.get_phv()->get_field(testHeader1, 0).set(v1);
    pkt.get_phv()->get_field(testHeader2, 0).set(v2);
    table->lookup(pkt, &hit, &h);
    ASSERT_EQ(expected, hit);
  };

  check_one(start_1, start_2, true);
  check_one(1, start_2, false);
  check_one(start_1, 1, false);
  check_one(1, 1, false);
  // TODO(antonin): more checks?
}
