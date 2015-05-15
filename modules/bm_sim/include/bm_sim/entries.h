#ifndef _BM_ENTRIES_H_
#define _BM_ENTRIES_H_

#include <memory>

#include "actions.h"
#include "bytecontainer.h"
#include "control_flow.h"

class ControlFlowNode;

struct MatchEntry
{
  ByteContainer key{};
  ActionFnEntry action_entry{}; // includes action data
  const ControlFlowNode *next_table{nullptr};

  MatchEntry() {}

  MatchEntry(ByteContainer key,
	     ActionFnEntry action_entry,
	     const ControlFlowNode *next_table)
  : key(std::move(key)), action_entry(std::move(action_entry)),
    next_table(next_table) {}

  MatchEntry(const MatchEntry &other) = delete;
  MatchEntry &operator=(const MatchEntry &other) = delete;

  MatchEntry(MatchEntry &&other) = default;
  MatchEntry &operator=(MatchEntry &&other) = default;
};

struct ExactMatchEntry: MatchEntry
{
  ExactMatchEntry()
    : MatchEntry() {}

  ExactMatchEntry(ByteContainer key,
		  ActionFnEntry action_entry,
		  const ControlFlowNode *next_table)
    : MatchEntry(std::move(key), std::move(action_entry), next_table) {}
};

struct LongestPrefixMatchEntry : MatchEntry
{
  int prefix_length{0};

  LongestPrefixMatchEntry()
    : MatchEntry() {}

  LongestPrefixMatchEntry(ByteContainer key,
			  ActionFnEntry action_entry,
			  unsigned int prefix_length,
			  const ControlFlowNode *next_table)
    : MatchEntry(std::move(key), std::move(action_entry), next_table),
    prefix_length(prefix_length) {
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
  ByteContainer mask{};
  int priority{0};

  TernaryMatchEntry()
    : MatchEntry() {}

  TernaryMatchEntry(ByteContainer key, ActionFnEntry action_entry,
		    ByteContainer mask, int priority,
		    const ControlFlowNode *next_table)
    : MatchEntry(std::move(key), std::move(action_entry), next_table),
    mask(std::move(mask)), priority(priority) {
    for(unsigned byte_index = 0; byte_index < key.size(); byte_index++) {
      this->key[byte_index] &= mask[byte_index];
    }
  }
};

#endif
