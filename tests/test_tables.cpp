#include <gtest/gtest.h>

#include <memory>
#include "bm_sim/tables.h"

using testing::Types;

using std::string;
using std::to_string;

typedef MatchTableAbstract::ActionEntry ActionEntry;
typedef MatchUnitExact<ActionEntry> MUExact;
typedef MatchUnitLPM<ActionEntry> MULPM;
typedef MatchUnitTernary<ActionEntry> MUTernary;

template <typename MUType>
class TableSizeOne : public ::testing::Test {
protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  MatchKeyBuilder key_builder_w_valid;
  std::unique_ptr<MatchTable> table_w_valid;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  TableSizeOne() 
    : testHeaderType("test_t", 0), action_fn("actionA", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0);

    key_builder_w_valid.push_back_field(testHeader1, 0);
    key_builder_w_valid.push_back_valid_header(testHeader2);

    std::unique_ptr<MUType> match_unit;
    std::unique_ptr<MatchTableAbstract> match_table;

    // true enables counters
    match_unit = std::unique_ptr<MUType>(new MUType(1, 2, key_builder));
    table = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit), true)
    );
    table->set_next_node(0, nullptr);

    match_unit = std::unique_ptr<MUType>(new MUType(1, 2, key_builder_w_valid));
    table_w_valid = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit))
    );
    table_w_valid->set_next_node(0, nullptr);
  }

  MatchErrorCode add_entry(const std::string &key,
			   entry_handle_t *handle);

  MatchErrorCode add_entry_w_valid(const std::string &key,
				   bool valid,
				   entry_handle_t *handle);

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, length, PacketBuffer(length * 2));
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

template<>
MatchErrorCode
TableSizeOne<MUExact>::add_entry(const std::string &key,
				 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeOne<MULPM>::add_entry(const std::string &key,
			       entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeOne<MUTernary>::add_entry(const std::string &key,
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
TableSizeOne<MUExact>::add_entry_w_valid(const std::string &key,
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
TableSizeOne<MULPM>::add_entry_w_valid(const std::string &key,
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
TableSizeOne<MUTernary>::add_entry_w_valid(const std::string &key,
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

typedef Types<MUExact,
	      MULPM,
	      MUTernary> TableTypes;

TYPED_TEST_CASE(TableSizeOne, TableTypes);

TYPED_TEST(TableSizeOne, AddEntry) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle_1, handle_2;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  rc = this->add_entry(key_, &handle_2);
  ASSERT_EQ(MatchErrorCode::TABLE_FULL, rc);
  ASSERT_EQ(1u, this->table->get_num_entries());
}

TYPED_TEST(TableSizeOne, DeleteEntry) {
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

TYPED_TEST(TableSizeOne, LookupEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  const ActionEntry *action_entry;
  entry_handle_t lookup_handle;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_NE(nullptr, action_entry);

  f.set("0xabb");
  this->table->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_EQ(nullptr, action_entry);

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_EQ(nullptr, action_entry);
}

TYPED_TEST(TableSizeOne, ModifyEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  const ActionEntry *action_entry;
  entry_handle_t lookup_handle;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_NE(nullptr, action_entry);
  ASSERT_EQ(0, action_entry->action_fn.action_data_size());

  // in theory I could modify the entry directly using the pointer, but I need
  // to exercise my modify_entry function

  // we modify the entry, this time using some action data
  ActionData new_action_data;
  new_action_data.push_back_action_data(0xaba);
  this->table->modify_entry(handle, &(this->action_fn),
			    std::move(new_action_data));

  this->table->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_NE(nullptr, action_entry);
  ASSERT_NE(0, action_entry->action_fn.action_data_size());
  ASSERT_EQ((unsigned) 0xaba, action_entry->action_fn.get_action_data(0).get_uint());
}

TYPED_TEST(TableSizeOne, Counters) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  uint64_t counter_bytes = 0;
  uint64_t counter_packets = 0;

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(0u, counter_bytes);
  ASSERT_EQ(0u, counter_packets);

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  // there is no action specified for the entry, potential for a bug :(
  this->table->lookup_action(pkt);

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);

  f.set("0xabb");
  this->table->lookup_action(pkt);

  // counters should not have been incremented
  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);
}

TYPED_TEST(TableSizeOne, Valid) {
  std::string key_ = "\x0a\xba";
  bool valid = true;
  entry_handle_t handle;
  MatchErrorCode rc;
  const ActionEntry *action_entry;
  entry_handle_t lookup_handle;

  rc = this->add_entry_w_valid(key_, valid, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table_w_valid->get_num_entries());

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");
  Header &h2 = pkt.get_phv()->get_header(this->testHeader2);
  ASSERT_FALSE(h2.is_valid());

  this->table_w_valid->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_EQ(nullptr, action_entry);

  h2.mark_valid();

  this->table_w_valid->lookup(pkt, &action_entry, &lookup_handle);
  ASSERT_NE(nullptr, action_entry);
}
