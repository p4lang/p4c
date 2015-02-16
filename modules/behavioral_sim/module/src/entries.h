#ifndef _BM_ENTRIES_H_
#define _BM_ENTRIES_H_

#include <memory>

#include "actions.h"
#include "bytecontainer.h"

struct MatchEntry
{
  ByteContainer key;
  ActionFn action_fn; // includes action data

  MatchEntry() {}

  MatchEntry(const ByteContainer &key, ActionFn &action_fn)
    : key(key), action_fn(action_fn) {}
};

struct ExactMatchEntry: MatchEntry
{
  ExactMatchEntry()
    : MatchEntry() {}

  ExactMatchEntry(const ByteContainer &key, ActionFn &action_fn)
    : MatchEntry(key, action_fn) {}
};

struct LongestPrefixMatchEntry : MatchEntry
{
  int prefix_length;

  LongestPrefixMatchEntry()
    : MatchEntry() {}

  LongestPrefixMatchEntry(const ByteContainer &key, ActionFn &action_fn,
			  unsigned prefix_length)
    : MatchEntry(key, action_fn), prefix_length(prefix_length) {
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

  TernaryMatchEntry(const ByteContainer &key, ActionFn &action_fn,
		    const ByteContainer &mask, int priority)
    : MatchEntry(key, action_fn), mask(mask), priority(priority) {
    for(unsigned byte_index = 0; byte_index < key.size(); byte_index++) {
      this->key[byte_index] &= mask[byte_index];
    }
  }
};

#endif
