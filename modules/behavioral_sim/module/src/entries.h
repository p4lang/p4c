#ifndef _BM_ENTRIES_H_
#define _BM_ENTRIES_H_

#include <memory>

#include "actions.h"
#include "bytecontainer.h"
#include "tables.h"

class MatchTable;

struct MatchEntry
{
  ByteContainer key;
  ActionFnEntry action_entry; // includes action data
  const MatchTable *next_table;

  MatchEntry() {}

  MatchEntry(const ByteContainer &key,
	     ActionFnEntry &action_entry,
	     const MatchTable *next_table)
    : key(key), action_entry(action_entry), next_table(next_table) {}
};

struct ExactMatchEntry: MatchEntry
{
  ExactMatchEntry()
    : MatchEntry() {}

  ExactMatchEntry(const ByteContainer &key,
		  ActionFnEntry &action_entry,
		  const MatchTable *next_table)
    : MatchEntry(key, action_entry, next_table) {}
};

struct LongestPrefixMatchEntry : MatchEntry
{
  int prefix_length;

  LongestPrefixMatchEntry()
    : MatchEntry() {}

  LongestPrefixMatchEntry(const ByteContainer &key, ActionFnEntry &action_entry,
			  unsigned prefix_length, const MatchTable *next_table)
    : MatchEntry(key, action_entry, next_table), prefix_length(prefix_length) {
    unsigned byte_index = prefix_length / 8;
    unsigned mod = prefix_length % 8;
    if(mod > 0) {
      byte_index++;
      this->key[byte_index] &= ~(0xFF >> mod);
    }
    for(; byte_index < key.size(); byte_index++) {
      this->key[byte_index] = 0;
    }
  }
};

struct TernaryMatchEntry : MatchEntry
{
  ByteContainer mask;
  int priority;

  TernaryMatchEntry()
    : MatchEntry() {}

  TernaryMatchEntry(const ByteContainer &key, ActionFnEntry &action_entry,
		    const ByteContainer &mask, int priority,
		    const MatchTable *next_table)
    : MatchEntry(key, action_entry, next_table), mask(mask), priority(priority) {
    for(unsigned byte_index = 0; byte_index < key.size(); byte_index++) {
      this->key[byte_index] &= mask[byte_index];
    }
  }
};

#endif
