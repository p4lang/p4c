#include "tables.h"

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
  char lookup_key[nbytes_key];
  build_key(phv, lookup_key);
  MatchEntry *entry = lookup(lookup_key);
  if(!entry) return; /* TODO : default action */
  entry->action_fn(phv);
}


ExactMatchEntry *ExactMatchTable::lookup(const char *key)
{
  /* TODO */
  ExactMatchEntry *entry = NULL;
  return entry;
}

int ExactMatchTable::add_entry(const char *key, const ActionFn &action_fn,
			       int *handle)
{
  /* TODO */
  return get_and_set_handle(handle);
}

int ExactMatchTable::delete_entry(int handle)
{
  MatchTable::delete_entry(handle);
  /* TODO */
  return 0;
}

LongestPrefixMatchEntry *LongestPrefixMatchTable::lookup(const char *key)
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

TernaryMatchEntry *TernaryMatchTable::lookup(const char *key)
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
