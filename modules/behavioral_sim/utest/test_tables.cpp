#include <gtest/gtest.h>

#include "tables.h"

int pull_test_tables() { return 0; }

using testing::Types;

template <typename TableType>
class TableSizeOne : public ::testing::Test {
protected:
  MatchKeyBuilder key_builder;
  TableType table;
  
  TableSizeOne()
    : key_builder(), table(1, 2, key_builder) {}


  virtual void SetUp() {}

  // virtual void TearDown() {}
};

// typedef TableSizeOne<ExactMatchTable> ExactTableSizeOne

typedef Types<ExactMatchTable, TernaryMatchTable> TableTypes;

TYPED_TEST_CASE(TableSizeOne, TableTypes);

TYPED_TEST(TableSizeOne, AddEntry) {
  ByteContainer key(2);
  ActionFn action_fn;
  entry_handle_t handle_1, handle_2;
  MatchTable::ErrorCode rc;

  rc = this->table.add_entry(key, action_fn, &handle_1);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(1u, this->table.get_num_entries());

  rc = this->table.add_entry(key, action_fn, &handle_2);
  ASSERT_EQ(MatchTable::TABLE_FULL, rc);
  ASSERT_EQ(1u, this->table.get_num_entries());
}

TYPED_TEST(TableSizeOne, DeleteEntry) {
  ByteContainer key(2);
  ActionFn action_fn;
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->table.add_entry(key, action_fn, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  rc = this->table.delete_entry(handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);
  ASSERT_EQ(0u, this->table.get_num_entries());

  rc = this->table.delete_entry(handle);
  ASSERT_EQ(MatchTable::INVALID_HANDLE, rc);

  rc = this->table.add_entry(key, action_fn, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);
}

TYPED_TEST(TableSizeOne, LookupEntry) {
  char key_[2] = {(char) 0x0a, (char) 0xba};
  ByteContainer key(key_, sizeof(key_));
  ActionFn action_fn;
  entry_handle_t handle;
  MatchTable::ErrorCode rc;

  rc = this->table.add_entry(key, action_fn, &handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  ASSERT_NE(nullptr, this->table.lookup(key));

  char nkey_[2] = {(char) 0x0a, (char) 0xbb};
  ByteContainer nkey(nkey_, sizeof(nkey_));

  ASSERT_EQ(nullptr, this->table.lookup(nkey));

  rc = this->table.delete_entry(handle);
  ASSERT_EQ(MatchTable::SUCCESS, rc);

  ASSERT_EQ(nullptr, this->table.lookup(key));
}
