#include "runtime.h"

namespace BM {

/* This seems like a good use of dynamic_cast<>, I do not see any nicer
   alternative */

MatchTable::ErrorCode
add_exact_match_entry(string &table_name, const char *key, ActionFn &action_fn,
		      entry_handle_t *handle) {
  auto *table =
    dynamic_cast<ExactMatchTable *>(MatchTable::get_table(table_name));
  size_t nbytes_key = table->get_nbytes_key();
  return table->add_entry(
      ExactMatchEntry(ByteContainer(key, nbytes_key), action_fn),
      handle
  );
}

MatchTable::ErrorCode
add_lpm_entry(string &table_name, const char *key, int prefix_length,
	      ActionFn &action_fn, entry_handle_t *handle) {
  auto *table =
    dynamic_cast<LongestPrefixMatchTable *>(MatchTable::get_table(table_name));
  size_t nbytes_key = table->get_nbytes_key();
  return table->add_entry(
      LongestPrefixMatchEntry(ByteContainer(key, nbytes_key), 
			      action_fn, prefix_length),
      handle
  );
}

MatchTable::ErrorCode
add_ternary_match_entry(string &table_name, const char *key, const char *mask,
			int priority, ActionFn &action_fn,
			entry_handle_t *handle) {
  auto *table =
    dynamic_cast<TernaryMatchTable *>(MatchTable::get_table(table_name));
  size_t nbytes_key = table->get_nbytes_key();
  return table->add_entry(
      TernaryMatchEntry(ByteContainer(key, nbytes_key),
			action_fn,
			ByteContainer(mask, nbytes_key),
			priority),
      handle
  );
}

}
