#include <gtest/gtest.h>

#include "tables.h"

int pull_test_tables() { return 0; }

using testing::Types;

using std::string;
using std::to_string;

template <typename TableType>
class TableSizeOne : public ::testing::Test {
protected:
  static int table_cnt;
  string table_name;
  MatchKeyBuilder key_builder;
  TableType table;

  // a bit dirty, but we need to generate a unique name for the table
  TableSizeOne() 
    : table_name(typeid(TableType).name() + to_string(++table_cnt)),
      key_builder(), table(table_name, 1, 2, key_builder) { }

  MatchTable::ErrorCode add_entry(const ByteContainer &key,
				  entry_handle_t *handle);

  virtual void SetUp() {}

  // virtual void TearDown() {}
};

template<typename TableType>
int TableSizeOne<TableType>::table_cnt = 0;

template<>
MatchTable::ErrorCode
TableSizeOne<ExactMatchTable>::add_entry(const ByteContainer &key,
					 entry_handle_t *handle) {
  ActionFn action_fn;
  return table.add_entry(key, action_fn, handle);
}

template<>
MatchTable::ErrorCode
TableSizeOne<LongestPrefixMatchTable>::add_entry(const ByteContainer &key,
						 entry_handle_t *handle) {
  ActionFn action_fn;
  int prefix_length = 16;
  return table.add_entry(key, prefix_length, action_fn, handle);
}

template<>
MatchTable::ErrorCode
TableSizeOne<TernaryMatchTable>::add_entry(const ByteContainer &key,
					   entry_handle_t *handle) {
  char mask_[2] = {(char) 0xff, (char) 0xff};
  ByteContainer mask(mask_, sizeof(mask_));
  ActionFn action_fn;
  int priority = 1;
  return table.add_entry(key, mask, priority, action_fn, handle);
}

typedef Types<ExactMatchTable,
	      LongestPrefixMatchTable,
	      TernaryMatchTable> TableTypes;

TYPED_TEST_CASE(TableSizeOne, TableTypes);

TYPED_TEST(TableSizeOne, AddEntry) {
  ByteContainer key(2);
  entry_handle_t handle_1, handle_2;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key, &handle_1);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(1u, this->table.get_num_entries());

  rc = this->add_entry(key, &handle_2);
  ASSERT_EQ(MatchTable::TABLE_FULL, rc);
  ASSERT_EQ(1u, this->table.get_num_entries());
}

TYPED_TEST(TableSizeOne, DeleteEntry) {
  ByteContainer key(2);
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  rc = this->table.delete_entry(handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);
  ASSERT_EQ(0u, this->table.get_num_entries());

  rc = this->table.delete_entry(handle);
  ASSERT_EQ(MatchTable::INVALID_HANDLE, rc);

  rc = this->add_entry(key, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);
}

TYPED_TEST(TableSizeOne, LookupEntry) {
  char key_[2] = {(char) 0x0a, (char) 0xba};
  ByteContainer key(key_, sizeof(key_));
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  ASSERT_NE(nullptr, this->table.lookup(key));

  char nkey_[2] = {(char) 0x0a, (char) 0xbb};
  ByteContainer nkey(nkey_, sizeof(nkey_));

  ASSERT_EQ(nullptr, this->table.lookup(nkey));

  rc = this->table.delete_entry(handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  ASSERT_EQ(nullptr, this->table.lookup(key));
}
