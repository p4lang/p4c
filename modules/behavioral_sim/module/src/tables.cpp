#include "tables.h"

using std::vector;
using std::copy;

int MatchTable::get_and_set_handle(entry_handle_t *handle)
{
  Word_t jhandle = 0;
  int Rc_int;
  J1FE(Rc_int, handles_used, jhandle); // Judy1FirstEmpty()
  J1S(Rc_int, handles_used, jhandle); // Judy1Set()

  *handle = jhandle;

  return Rc_int;
}

int MatchTable::unset_handle(entry_handle_t handle)
{
  int Rc_int;
  J1U(Rc_int, handles_used, handle); // Judy1Unset()

  return Rc_int;
}

int MatchTable::delete_entry(entry_handle_t handle)
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
  return &entries[entry_it->second];
}

int ExactMatchTable::add_entry(const ByteContainer &key, const ActionFn &action_fn,
			       entry_handle_t *handle)
{
  int status = get_and_set_handle(handle);
  if(!status) return status;
  
  entries[*handle] = ExactMatchEntry(key, action_fn);
  // the key is copied a second time, but it should not incur a significant cost
  entries_map[key] = *handle;

  return 0;
}

int ExactMatchTable::delete_entry(entry_handle_t handle)
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

int LongestPrefixMatchTable::add_entry(const ByteContainer &key,
				       int prefix_length,
				       const ActionFn &action_fn,
				       entry_handle_t *handle)
{
  /* TODO */
  return get_and_set_handle(handle);
}

int LongestPrefixMatchTable::delete_entry(entry_handle_t handle)
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

int TernaryMatchTable::add_entry(const ByteContainer &key,
				 const ByteContainer &mask,
				 int priority,
				 const ActionFn &action_fn,
				 entry_handle_t *handle)
{
  /* TODO */
  return get_and_set_handle(handle);
}

int TernaryMatchTable::delete_entry(entry_handle_t handle)
{
  MatchTable::delete_entry(handle);
  /* TODO */
  return 0;
}
