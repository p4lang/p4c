#include <gtest/gtest.h>

#include <memory>
#include "bm_sim/tables.h"

using testing::Types;

using std::string;
using std::to_string;

template <typename TableType>
class TableSizeOne : public ::testing::Test {
protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  // using a pointer because table is not movable (mutex member)
  // would not need that if key_builder could be set after the table was created
  // whould I consider doing that ?
  // TODO
  std::unique_ptr<TableType> table;

  MatchKeyBuilder key_builder_w_valid;
  std::unique_ptr<TableType> table_w_valid;

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

    // true enables counters
    table = std::unique_ptr<TableType>(
        new TableType("test_table", 0, 1, 2, key_builder, true)
    );
    table->set_next_node(0, nullptr);

    table_w_valid = std::unique_ptr<TableType>(
        new TableType("test_table_w_valid", 0, 1, 2, key_builder_w_valid, true)
    );
    table_w_valid->set_next_node(0, nullptr);
  }

  MatchTable::ErrorCode add_entry(const std::string &key,
				  entry_handle_t *handle);

  MatchTable::ErrorCode add_entry_w_valid(const std::string &key,
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
MatchTable::ErrorCode
TableSizeOne<ExactMatchTable>::add_entry(const std::string &key,
					 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  return table->add_entry(match_key, action_fn, action_data, handle);
}

template<>
MatchTable::ErrorCode
TableSizeOne<LongestPrefixMatchTable>::add_entry(const std::string &key,
						 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  return table->add_entry(match_key, action_fn, action_data, handle);
}

template<>
MatchTable::ErrorCode
TableSizeOne<TernaryMatchTable>::add_entry(const std::string &key,
					   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, key, std::move(mask));
  return table->add_entry(match_key, action_fn, action_data, handle, priority);
}

template<>
MatchTable::ErrorCode
TableSizeOne<ExactMatchTable>::add_entry_w_valid(const std::string &key,
						 bool valid,
						 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  // I'm putting the valid match in second position on purpose
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, action_fn, action_data, handle);
}

template<>
MatchTable::ErrorCode
TableSizeOne<LongestPrefixMatchTable>::add_entry_w_valid(const std::string &key,
							 bool valid,
							 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, action_fn, action_data, handle);
}

template<>
MatchTable::ErrorCode
TableSizeOne<TernaryMatchTable>::add_entry_w_valid(const std::string &key,
						   bool valid,
						   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, key, std::move(mask));
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, action_fn, action_data, handle, priority);
}

typedef Types<ExactMatchTable,
	      LongestPrefixMatchTable,
	      TernaryMatchTable> TableTypes;

TYPED_TEST_CASE(TableSizeOne, TableTypes);

TYPED_TEST(TableSizeOne, AddEntry) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle_1, handle_2;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key_, &handle_1);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  rc = this->add_entry(key_, &handle_2);
  ASSERT_EQ(MatchTable::TABLE_FULL, rc);
  ASSERT_EQ(1u, this->table->get_num_entries());
}

TYPED_TEST(TableSizeOne, DeleteEntry) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);
  ASSERT_EQ(0u, this->table->get_num_entries());

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchTable::INVALID_HANDLE, rc);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);
}

TYPED_TEST(TableSizeOne, LookupEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  ASSERT_NE(nullptr, this->table->lookup(key));

  std::string nkey_ = "\x0a\xbb";
  ByteContainer nkey("0x0abb");

  ASSERT_EQ(nullptr, this->table->lookup(nkey));

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  ASSERT_EQ(nullptr, this->table->lookup(key));
}

TYPED_TEST(TableSizeOne, ModifyEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  const MatchEntry *entry = this->table->lookup(key);
  ASSERT_NE(nullptr, entry);
  ASSERT_EQ(0, entry->action_entry.action_data_size());

  // in theory I could modify the entry directly using the pointer, but I need
  // to exercise my modify_entry function

  // we define a different action entry (with some action data) and modify the
  // entry using this action entry
  ActionFnEntry new_action_entry;
  new_action_entry.push_back_action_data(0xaba);
  this->table->modify_entry(handle, new_action_entry, nullptr);

  entry = this->table->lookup(key);
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(0, entry->action_entry.action_data_size());
  ASSERT_EQ((unsigned) 0xaba, entry->action_entry.get_action_data(0).get_uint());
}

TYPED_TEST(TableSizeOne, Counters) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  uint64_t counter_bytes = 0;
  uint64_t counter_packets = 0;

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(0u, counter_bytes);
  ASSERT_EQ(0u, counter_packets);

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  // there is no action specified for the entry, potential for a bug :(
  (*(this->table))(&pkt);

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);

  f.set("0xabb");
  (*(this->table))(&pkt);

  // counters should not have been incremented
  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);
}

TYPED_TEST(TableSizeOne, Valid) {
  std::string key_ = "\x0a\xba";
  bool valid = true;
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry_w_valid(key_, valid, &handle);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(1u, this->table_w_valid->get_num_entries());

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");
  Header &h2 = pkt.get_phv()->get_header(this->testHeader2);
  ASSERT_FALSE(h2.is_valid());

  ByteContainer lookup_key;
  this->key_builder_w_valid(*(pkt.get_phv()), lookup_key);
  ASSERT_EQ(ByteContainer("0x000aba"), lookup_key);

  ASSERT_EQ(nullptr, this->table_w_valid->lookup(lookup_key));

  h2.mark_valid();
  lookup_key.clear();
  this->key_builder_w_valid(*(pkt.get_phv()), lookup_key);
  ASSERT_EQ(ByteContainer("0x010aba"), lookup_key);

  ASSERT_NE(nullptr, this->table_w_valid->lookup(lookup_key));
}
