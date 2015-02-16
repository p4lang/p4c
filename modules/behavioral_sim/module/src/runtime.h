#ifndef _BM_RUNTIME_H_
#define _BM_RUNTIME_H_

#include <string>

#include "tables.h"

using std::string;

namespace BM {

MatchTable::ErrorCode
add_exact_match_entry(string &table_name, const char *key, ActionFn &action_fn,
		      entry_handle_t *handle);

MatchTable::ErrorCode
add_lpm_entry(string &table_name, const char *key, int prefix_length,
	      ActionFn &action_fn, entry_handle_t *handle);

MatchTable::ErrorCode
add_ternary_match_entry(string &table_name, const char *key, const char *mask,
			int priority, ActionFn &action_fn,
			entry_handle_t *handle);

}

#endif
