#include <gtest/gtest.h>

#include "runtime.h"

int pull_test_runtime() { return 0; }

/* These tests are succint on purpose, they are just aimed at testing the
   runtime APIs exposed in runtime.h. For more in-depth testing, see the test
   files for each subunit (e.g. test_tables.cpp). */

using testing::Types;

using std::string;
using std::to_string;

template <typename TableType>
class RuntimeTables : public ::testing::Test {
protected:
  MatchKeyBuilder key_builder;
  string table_name;
  TableType table;

  RuntimeTables() 
    : key_builder(), table_name("test_table"),
      table(table_name, 1, 2, key_builder) { }

  MatchTable::ErrorCode add_entry(const char *key, entry_handle_t *handle);

  virtual void SetUp() {}

  // virtual void TearDown() {}
};

template<>
MatchTable::ErrorCode
RuntimeTables<ExactMatchTable>::add_entry(const char *key,
					  entry_handle_t *handle) {
  ActionFn action_fn;
  return BM::add_exact_match_entry(table_name, key, action_fn, handle);
}

template<>
MatchTable::ErrorCode
RuntimeTables<LongestPrefixMatchTable>::add_entry(const char *key,
						  entry_handle_t *handle) {
  ActionFn action_fn;
  int prefix_length = 16;
  return BM::add_lpm_entry(table_name, key, prefix_length, action_fn, handle);
}

template<>
MatchTable::ErrorCode
RuntimeTables<TernaryMatchTable>::add_entry(const char *key,
					    entry_handle_t *handle) {
  char mask[2] = {(char) 0xff, (char) 0xff};
  ActionFn action_fn;
  int priority = 1;
  return BM::add_ternary_match_entry(table_name, key, mask, priority,
				     action_fn, handle);
}

typedef Types<ExactMatchTable,
	      LongestPrefixMatchTable,
	      TernaryMatchTable> TableTypes;

TYPED_TEST_CASE(RuntimeTables, TableTypes);

TYPED_TEST(RuntimeTables, AddEntry) {
  char key[2] = {(char) 0x0a, (char) 0xba};
  entry_handle_t handle_1, handle_2;
  MatchTable::ErrorCode rc;

  rc = this->add_entry(key, &handle_1);
  ASSERT_EQ(rc, MatchTable::SUCCESS);
  ASSERT_EQ(1u, this->table.get_num_entries());

  rc = this->add_entry(key, &handle_2);
  ASSERT_EQ(MatchTable::TABLE_FULL, rc);
  ASSERT_EQ(1u, this->table.get_num_entries());
}
