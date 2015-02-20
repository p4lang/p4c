#ifndef _BM_TABLES_H_
#define _BM_TABLES_H_

#include <unordered_map>
#include <string>

#include "entries.h"
#include "packet.h"
#include "phv.h"
#include "actions.h"
#include "bytecontainer.h"
#include "handle_mgr.h"
#include "lpm_trie.h"

using std::vector;
using std::unordered_map;
using std::pair;
using std::string;

typedef uintptr_t entry_handle_t;

struct MatchKeyBuilder
{
  vector< pair<header_id_t, int> > fields;

  void add_field(header_id_t header, int field_offset) {
    fields.push_back( pair<header_id_t, int>(header, field_offset) );
  }

  void operator()(const PHV &phv, ByteContainer &key) const
  {
    for(vector< pair<header_id_t, int> >::const_iterator it = fields.begin();
	it != fields.end();
	it++) {
      const Field &field = phv.get_field((*it).first, (*it).second);
      key.append(field.get_bytes());
    }
  }
};

class MatchTable
{
public:
  enum ErrorCode {
    SUCCESS = 0,
    TABLE_FULL,
    INVALID_HANDLE,
    ERROR
  };
public:
  MatchTable(const string &name, size_t size, size_t nbytes_key,
	     const MatchKeyBuilder &match_key_builder)
    : name(name), size(size), num_entries(0),
      nbytes_key(nbytes_key),
      match_key_builder(match_key_builder) {
    auto table_it = tables_map.find(name);
    assert(table_it == tables_map.end()); // make sure the table name is not taken
    tables_map[name] = this;
  }

  virtual ~MatchTable() {
    tables_map.erase(name);
  }
  
  void apply(const Packet &pkt, PHV *phv);
  
  virtual const MatchEntry *lookup(const ByteContainer &key) const = 0;
  virtual ErrorCode delete_entry(entry_handle_t handle);

  size_t get_num_entries() const {return num_entries;}

  size_t get_nbytes_key() const {return nbytes_key;}

protected:
  string name;

  size_t size;
  size_t num_entries;
  size_t nbytes_key;
  HandleMgr handles;
  MatchKeyBuilder match_key_builder;

  void build_key(const PHV &phv, ByteContainer &key) {
    match_key_builder(phv, key);
  }
  
  ErrorCode get_and_set_handle(entry_handle_t *handle);
  ErrorCode unset_handle(entry_handle_t handle);

  bool valid_handle(entry_handle_t handle) const;

private:
  static unordered_map<string, MatchTable *> tables_map;

public:
  static MatchTable *get_table(const string &name);
};

class ExactMatchTable : public MatchTable
{
public:
  ExactMatchTable(const string &name, int size, int nbytes_key,
		  const MatchKeyBuilder &match_key_builder)
    : MatchTable(name, size, nbytes_key, match_key_builder) {
    entries = vector<ExactMatchEntry>(size);
    entries_map.reserve(size);
  }

  const ExactMatchEntry *lookup(const ByteContainer &key) const;
  // ErrorCode add_entry(const ExactMatchEntry &entry, entry_handle_t *handle);
  ErrorCode add_entry(ExactMatchEntry &&entry, entry_handle_t *handle);
  ErrorCode delete_entry(entry_handle_t handle);

private:
  vector<ExactMatchEntry> entries;
  unordered_map<ByteContainer, entry_handle_t, ByteContainerKeyHash> entries_map;
};

class LongestPrefixMatchTable : public MatchTable
{
public:
  LongestPrefixMatchTable(const string &name, int size, int nbytes_key, 
			  const MatchKeyBuilder &match_key_builder)
    : MatchTable(name, size, nbytes_key, match_key_builder),
    entries_trie(nbytes_key) {
    entries = vector<LongestPrefixMatchEntry>(size);
  }
  
  const LongestPrefixMatchEntry *lookup(const ByteContainer &key) const;
  // ErrorCode add_entry(const LongestPrefixMatchEntry &entry,
  // 		      entry_handle_t *handle);
  ErrorCode add_entry(LongestPrefixMatchEntry &&entry,
		      entry_handle_t *handle);
  ErrorCode delete_entry(entry_handle_t handle);

private:
  vector<LongestPrefixMatchEntry> entries;
  LPMTrie entries_trie;
};


class TernaryMatchTable : public MatchTable
{
public:
  TernaryMatchTable(const string &name, int size, int nbytes_key,
		    const MatchKeyBuilder &match_key_builder)
    : MatchTable(name, size, nbytes_key, match_key_builder) {
    entries = vector<TernaryMatchEntry>(size);
  }

  const TernaryMatchEntry *lookup(const ByteContainer &key) const;
  // ErrorCode add_entry(const TernaryMatchEntry &entry, entry_handle_t *handle);
  ErrorCode add_entry(TernaryMatchEntry &&entry, entry_handle_t *handle);
  ErrorCode delete_entry(entry_handle_t handle);

private:
  vector<TernaryMatchEntry> entries;
};

#endif
