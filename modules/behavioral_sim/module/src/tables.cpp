#include "tables.h"

using std::vector;
using std::copy;

int MatchTable::get_and_set_handle(int *handle)
{
  Word_t jhandle = 0;
  int Rc_int;
  J1FE(Rc_int, handles_used, jhandle); // Judy1FirstEmpty()
  J1S(Rc_int, handles_used, jhandle); // Judy1Set()

  *handle = jhandle;

  return Rc_int;
}

int MatchTable::unset_handle(int handle)
{
  int Rc_int;
  J1U(Rc_int, handles_used, handle); // Judy1Unset()

  return Rc_int;
}

int MatchTable::delete_entry(int handle)
{
  return unset_handle(handle);
}

void MatchTable::apply(const PHV &phv)
{
  ByteContainer lookup_key;
  build_key(phv, lookup_key);
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
  return entry_it->second;
}

int ExactMatchTable::add_entry(const ByteContainer &key, const ActionFn &action_fn,
			       int *handle)
{
  int status = get_and_set_handle(handle);
  if(!status) return status;
  
  entries[*handle] = ExactMatchEntry(key, action_fn);
  // the key is copied a second time, but it should not incur a significant cost
  entries_map[key] = &entries[*handle];

  return 0;
}

int ExactMatchTable::delete_entry(int handle)
{
  MatchTable::delete_entry(handle);
  /* TODO */
  return 0;
}

const LongestPrefixMatchEntry *LongestPrefixMatchTable::lookup(const ByteContainer &key) const
{
  /* TODO */
  LongestPrefixMatchEntry *entry = NULL;
  return entry;
}

int LongestPrefixMatchTable::add_entry(const char *key, int prefix_length,
				       const ActionFn &action_fn,
				       int *handle)
{
  /* TODO */
  return get_and_set_handle(handle);
}

int LongestPrefixMatchTable::delete_entry(int handle)
{
  MatchTable::delete_entry(handle);
  /* TODO */
  return 0;
}

const TernaryMatchEntry *TernaryMatchTable::lookup(const ByteContainer &key) const
{
  /* TODO */
  TernaryMatchEntry *entry = NULL;
  return entry;
}

int TernaryMatchTable::add_entry(const char *key, const char *mask,
				 int priority,
				 const ActionFn &action_fn,
				 int *handle)
{
  /* TODO */
  return get_and_set_handle(handle);
}

int TernaryMatchTable::delete_entry(int handle)
{
  MatchTable::delete_entry(handle);
  /* TODO */
  return 0;
}
