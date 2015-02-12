#include "tables.h"
#include <iostream>

using std::vector;
using std::copy;

MatchTable::ErrorCode MatchTable::get_and_set_handle(entry_handle_t *handle)
{
  if(num_entries >= size) { // table is full
    return TABLE_FULL;
  }

  if(handles.get_handle(handle)) return ERROR;

  num_entries++;
  return SUCCESS;
}

MatchTable::ErrorCode MatchTable::unset_handle(entry_handle_t handle)
{
  if(handles.release_handle(handle)) return INVALID_HANDLE;

  num_entries--;
  return SUCCESS;
}

bool MatchTable::valid_handle(entry_handle_t handle) const {
  return handles.valid_handle(handle);
}

MatchTable::ErrorCode MatchTable::delete_entry(entry_handle_t handle)
{
  return unset_handle(handle);
}

void MatchTable::apply(const Packet &pkt, PHV *phv)
{
  ByteContainer lookup_key;
  build_key(*phv, lookup_key);
  // char lookup_key[nbytes_key];
  // build_key(phv, lookup_key);
  // MatchEntry *entry = lookup(lookup_key);
  // if(!entry) return; /* TODO : default action */
  // entry->action_fn(phv);
}


const ExactMatchEntry *ExactMatchTable::lookup(const ByteContainer &key) const
{
  auto entry_it = entries_map.find(key);
  if(entry_it == entries_map.end()) return nullptr;
  return &entries[entry_it->second];
}

MatchTable::ErrorCode ExactMatchTable::add_entry(const ByteContainer &key,
						 const ActionFn &action_fn,
						 entry_handle_t *handle)
{
  ErrorCode status = get_and_set_handle(handle);
  if(status != SUCCESS) return status;
  
  entries[*handle] = ExactMatchEntry(key, action_fn);
  // the key is copied a second time, but it should not incur a significant cost
  entries_map[key] = *handle;

  return SUCCESS;
}

MatchTable::ErrorCode ExactMatchTable::delete_entry(entry_handle_t handle)
{
  if(!valid_handle(handle)) return INVALID_HANDLE;
  ByteContainer &key = entries[handle].key;
  entries_map.erase(key);
  return MatchTable::delete_entry(handle);
}

const LongestPrefixMatchEntry *LongestPrefixMatchTable::lookup(const ByteContainer &key) const
{
  entry_handle_t handle;
  if(entries_trie.lookup(key, &handle)) {
    return &entries[handle];
  }
  return nullptr;
}

MatchTable::ErrorCode LongestPrefixMatchTable::add_entry(const ByteContainer &key,
							 int prefix_length,
							 const ActionFn &action_fn,
							 entry_handle_t *handle)
{
  assert(prefix_length >= 0);
  ErrorCode status = get_and_set_handle(handle);
  if(status != SUCCESS) return status;
  
  entries[*handle] = LongestPrefixMatchEntry(key, action_fn, prefix_length);
  entries_trie.insert_prefix(entries[*handle].key, prefix_length, *handle);

  return SUCCESS;
}

MatchTable::ErrorCode LongestPrefixMatchTable::delete_entry(entry_handle_t handle)
{
  if(!valid_handle(handle)) return INVALID_HANDLE;
  LongestPrefixMatchEntry &entry = entries[handle];
  assert(entries_trie.delete_prefix(entry.key, entry.prefix_length));
  return MatchTable::delete_entry(handle);
}

const TernaryMatchEntry *TernaryMatchTable::lookup(const ByteContainer &key) const
{
  int max_priority = 0;
  bool match;

  const TernaryMatchEntry *entry;
  const TernaryMatchEntry *max_entry = nullptr;

  for(auto it = handles.begin(); it != handles.end(); ++it) {
    entry = &entries[*it];

    if(entry->priority <= max_priority) continue;
    
    match = true;
    for(int byte_index = 0; byte_index < nbytes_key; byte_index++) {
      if(entry->key[byte_index] != (key[byte_index] & entry->mask[byte_index])) {
	match = false;
	break;
      }
    }

    if(match) {
      max_priority = entry->priority;
      max_entry = entry;
    }
  }

  return max_entry;
}

MatchTable::ErrorCode TernaryMatchTable::add_entry(const ByteContainer &key,
						   const ByteContainer &mask,
						   int priority,
						   const ActionFn &action_fn,
						   entry_handle_t *handle)
{
  ErrorCode status = get_and_set_handle(handle);
  if(status != SUCCESS) return status;
  
  entries[*handle] = TernaryMatchEntry(key, action_fn, mask, priority);

  return SUCCESS;
}

MatchTable::ErrorCode TernaryMatchTable::delete_entry(entry_handle_t handle)
{
  if(!valid_handle(handle)) return INVALID_HANDLE;
  return MatchTable::delete_entry(handle);
}
