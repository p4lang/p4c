#ifndef _BM_TABLES_H_
#define _BM_TABLES_H_

#include <Judy.h>

#include "entries.h"
#include "phv.h"
#include "actions.h"


struct MatchKeyBuilder
{
  void operator()(const PHV &phv, char *key) const
  {
    // builds a key
  }
};


class MatchTable
{
protected:
  int size;
  int nbytes_key;
  Pvoid_t handles_used; // Judy array of used entries, for handles
  
  MatchKeyBuilder match_key_builder;
  void build_key(const PHV &phv, char *key) { match_key_builder(phv, key); }
  
  int get_and_set_handle(int *handle);
  int unset_handle(int handle);
  
public:
  MatchTable(int size, int nbytes_key,
	     const MatchKeyBuilder &match_key_builder)
    : size(size),
      nbytes_key(nbytes_key),
      match_key_builder(match_key_builder) { }
  
  ~MatchTable() {
    
  }
  
  void apply(const PHV &phv);
  
  virtual MatchEntry *lookup(const char *key) = 0;
  virtual int delete_entry(int handle);
};

class ExactMatchTable : public MatchTable
{

public:
  ExactMatchTable(int size, int nbytes_key,
		  const MatchKeyBuilder &match_key_builder)
    : MatchTable(size, nbytes_key, match_key_builder) {
    
  }
  
  ExactMatchEntry *lookup(const char *key);
  int add_entry(const char *key, const ActionFn &action_fn,
		int *handle);
  int delete_entry(int handle);
};

class LongestPrefixMatchTable : public MatchTable
{
public:
  LongestPrefixMatchTable(int size, int nbytes_key, 
			  const MatchKeyBuilder &match_key_builder)
    : MatchTable(size, nbytes_key, match_key_builder) {

  }
  
  LongestPrefixMatchEntry *lookup(const char *key);
  int add_entry(const char *key, int prefix_length, const ActionFn &action_fn,
		int *handle);
  int delete_entry(int handle);
};


class TernaryMatchTable : public MatchTable
{
public:
  TernaryMatchTable(int size, int nbytes_key,
		    const MatchKeyBuilder &match_key_builder)
    : MatchTable(size, nbytes_key, match_key_builder) {

  }

  TernaryMatchEntry *lookup(const char *key);
  int add_entry(const char *key, const char *mask, int priority,
		const ActionFn &action_fn,
		int *handle);
  int delete_entry(int handle);
};

#endif
