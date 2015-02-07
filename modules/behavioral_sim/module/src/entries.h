#ifndef _BM_ENTRIES_H_
#define _BM_ENTRIES_H_

#include <memory>

#include "actions.h"

// using std::unique_ptr;
// using std::move;
using std::string;

struct MatchEntry
{
  string key;
  ActionFn action_fn; // includes action data

  MatchEntry() {}

  MatchEntry(const string &key, ActionFn action_fn)
    : key(key), action_fn(action_fn) {}
};

struct ExactMatchEntry: MatchEntry
{
  ExactMatchEntry()
    : MatchEntry() {}

  ExactMatchEntry(const string &key, ActionFn action_fn)
    : MatchEntry(key, action_fn) {}
};

struct LongestPrefixMatchEntry : MatchEntry
{
  int prefix_length;

  LongestPrefixMatchEntry()
    : MatchEntry() {}

  LongestPrefixMatchEntry(const string &key, ActionFn action_fn,
			  int prefix_length)
    : MatchEntry(key, action_fn), prefix_length(prefix_length) {}
};

struct TernaryMatchEntry : MatchEntry
{
  string mask;
  int priority;

  TernaryMatchEntry()
    : MatchEntry() {}

  TernaryMatchEntry(const string &key, ActionFn action_fn,
		    const string &mask, int priority)
    : MatchEntry(key, action_fn), mask(mask), priority(priority) {}
};

#endif
