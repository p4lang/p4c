#ifndef _BM_ENTRIES_H_
#define _BM_ENTRIES_H_

#include "actions.h"

struct MatchEntry
{
  char *key;
  ActionFn action_fn; /* includes action data */
};

struct ExactMatchEntry: MatchEntry
{

};

struct LongestPrefixMatchEntry : MatchEntry
{
  int prefix_length;
};

struct TernaryMatchEntry : MatchEntry
{
  char *mask;
  int priority;
};

#endif
