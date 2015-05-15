#ifndef _BM_TABLES_H_
#define _BM_TABLES_H_

#include <unordered_map>
#include <string>
#include <atomic>

#include "entries.h"
#include "packet.h"
#include "phv.h"
#include "actions.h"
#include "bytecontainer.h"
#include "handle_mgr.h"
#include "lpm_trie.h"
#include "control_flow.h"
#include "named_p4object.h"

// shared_mutex will only be available in C++-14, so for now I'm using boost
#include <boost/thread/shared_mutex.hpp>

typedef uintptr_t entry_handle_t;

// using string and not ByteBontainer for efficiency
struct MatchKeyParam {
  enum class Type {
    EXACT,
    LPM,
    TERNARY,
    VALID
  };

  MatchKeyParam(const Type &type, std::string key)
    : type(type), key(std::move(key)) { }

  MatchKeyParam(const Type &type, std::string key, std::string mask)
    : type(type), key(std::move(key)), mask(std::move(mask)) { }

  MatchKeyParam(const Type &type, std::string key, int prefix_length)
    : type(type), key(std::move(key)), prefix_length(prefix_length) { }

  Type type;
  std::string key;
  std::string mask{}; // optional
  int prefix_length{0}; // optional
};

struct MatchKeyBuilder
{
  std::vector<header_id_t> valid_headers{};
  std::vector<std::pair<header_id_t, int> > fields{};

  void push_back_field(header_id_t header, int field_offset) {
    fields.push_back(std::pair<header_id_t, int>(header, field_offset));
  }

  void push_back_valid_header(header_id_t header) {
    valid_headers.push_back(header);
  }

  void operator()(const PHV &phv, ByteContainer &key) const
  {
    for(const auto &h : valid_headers) {
      key.push_back(phv.get_header(h).is_valid() ? '\x01' : '\x00');
    }
    for(const auto &p : fields) {
      const Field &field = phv.get_field(p.first, p.second);
      key.append(field.get_bytes());
    }
  }
};

class MatchTable : public ControlFlowNode, public NamedP4Object
{
public:
  enum ErrorCode {
    SUCCESS = 0,
    TABLE_FULL,
    INVALID_HANDLE,
    ERROR,
    COUNTERS_DISABLED
  };
  
  typedef struct {
    std::atomic<std::uint_fast64_t> bytes{0};
    std::atomic<std::uint_fast64_t> packets{0};
  } Counter;
  typedef uint64_t counter_value_t;

public:
  MatchTable(const std::string &name, p4object_id_t id,
	     size_t size, size_t nbytes_key,
	     const MatchKeyBuilder &match_key_builder,
	     bool with_counters = false)
    : NamedP4Object(name, id),
      size(size),
      nbytes_key(nbytes_key),
      match_key_builder(match_key_builder),
      with_counters(with_counters),
      counters(size) { }

  virtual ~MatchTable() {}
  
  // return pointer to next control flow node
  const ControlFlowNode *operator()(Packet *pkt) const override;
  
  virtual const MatchEntry *lookup(const ByteContainer &key) const = 0;
  virtual ErrorCode delete_entry(entry_handle_t handle);
  virtual ErrorCode modify_entry(entry_handle_t handle,
				 const ActionFnEntry &action_entry,
				 const ControlFlowNode *next_table) = 0;
  virtual ErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			      const ActionFn &action_fn,
			      const ActionData &action_data,
			      entry_handle_t *handle,
			      int priority = -1) = 0;

  virtual entry_handle_t get_entry_handle(const MatchEntry &entry) const = 0;

  size_t get_num_entries() const {return num_entries;}

  size_t get_nbytes_key() const {return nbytes_key;}

  void set_next_node(p4object_id_t action_id, const ControlFlowNode *next_node) {
    next_nodes[action_id] = next_node;
  }

  const ControlFlowNode *get_next_node(p4object_id_t action_id) const {
    return next_nodes.at(action_id);
  }

  ErrorCode set_default_action(const ActionFnEntry &action_entry,
			       const ControlFlowNode *next_node) {
    default_action_entry = action_entry;
    default_next_node = next_node;
    return SUCCESS;
  }

  void update_counters(entry_handle_t handle, const Packet &pkt) const {
    assert(with_counters);
    Counter &c = counters[handle];
    c.bytes += pkt.get_ingress_length();
    c.packets += 1;
  }

  ErrorCode query_counters(entry_handle_t handle,
			   counter_value_t *bytes,
			   counter_value_t *packets) const {
    if(!with_counters) return COUNTERS_DISABLED;
    Counter &c = counters[handle];
    *bytes = c.bytes;
    *packets = c.packets;
    return SUCCESS;
  }

  ErrorCode reset_counters() const {
    if(!with_counters) return COUNTERS_DISABLED;
    // could take a while, but do not block anyone else
    // alternative would be to do a fill and use a lock
    for(Counter &c : counters) {
      c.bytes = 0;
      c.packets = 0;
    }
    return SUCCESS;
  }

  MatchTable(const MatchTable &other) = delete;
  MatchTable &operator=(const MatchTable &other) = delete;

  MatchTable(MatchTable &&other) /*noexcept*/ = default;
  MatchTable &operator=(MatchTable &&other) /*noexcept*/ = default;

protected:
  size_t size{0};
  size_t num_entries{0};
  size_t nbytes_key;
  HandleMgr handles{};
  MatchKeyBuilder match_key_builder;
  // indexed by the action id
  std::unordered_map<p4object_id_t, const ControlFlowNode *> next_nodes{};
  ActionFnEntry default_action_entry{};
  const ControlFlowNode *default_next_node{nullptr};
  std::atomic_bool with_counters{false};
  // is it really legit for me to use mutable for counters
  // do counters really have a different status than entries
  // or is it just a hortcut for now?
  // TODO
  mutable std::vector<Counter> counters{};

  mutable boost::shared_mutex t_mutex{};

  void build_key(const PHV &phv, ByteContainer &key) const {
    match_key_builder(phv, key);
  }
  
  ErrorCode get_and_set_handle(entry_handle_t *handle);
  ErrorCode unset_handle(entry_handle_t handle);

  bool valid_handle(entry_handle_t handle) const;
};

class ExactMatchTable : public MatchTable
{
public:
  ExactMatchTable(const std::string &name, p4object_id_t id,
		  int size, int nbytes_key,
		  const MatchKeyBuilder &match_key_builder,
		  bool with_counters = false)
    : MatchTable(name, id, size, nbytes_key, match_key_builder, with_counters) {
    entries = std::vector<ExactMatchEntry>(size);
    entries_map.reserve(size);
  }

  const ExactMatchEntry *lookup(const ByteContainer &key) const;
  // ErrorCode add_entry(const ExactMatchEntry &entry, entry_handle_t *handle);
  ErrorCode add_entry(ExactMatchEntry &&entry, entry_handle_t *handle);
  ErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
		      const ActionFn &action_fn,
		      const ActionData &action_data,
		      entry_handle_t *handle,
		      int priority = -1) override;
  ErrorCode delete_entry(entry_handle_t handle);
  ErrorCode modify_entry(entry_handle_t handle,
			 const ActionFnEntry &action_entry,
			 const ControlFlowNode *next_table);

  entry_handle_t get_entry_handle(const MatchEntry &entry) const {
    return ((char *) &entry - (char *) entries.data()) / sizeof(entry);
  }

private:
  std::vector<ExactMatchEntry> entries{};
  std::unordered_map<ByteContainer, entry_handle_t, ByteContainerKeyHash> entries_map{};
};

class LongestPrefixMatchTable : public MatchTable
{
public:
  LongestPrefixMatchTable(const std::string &name, p4object_id_t id,
			  int size, int nbytes_key, 
			  const MatchKeyBuilder &match_key_builder,
			  bool with_counters = false)
    : MatchTable(name, id, size, nbytes_key, match_key_builder, with_counters),
      entries_trie(nbytes_key) {
    entries = std::vector<LongestPrefixMatchEntry>(size);
  }
  
  const LongestPrefixMatchEntry *lookup(const ByteContainer &key) const;
  // ErrorCode add_entry(const LongestPrefixMatchEntry &entry,
  // 		      entry_handle_t *handle);
  ErrorCode add_entry(LongestPrefixMatchEntry &&entry,
		      entry_handle_t *handle);
  ErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
		      const ActionFn &action_fn,
		      const ActionData &action_data,
		      entry_handle_t *handle,
		      int priority = -1) override;
  ErrorCode delete_entry(entry_handle_t handle);
  ErrorCode modify_entry(entry_handle_t handle,
			 const ActionFnEntry &action_entry,
			 const ControlFlowNode *next_table);

  entry_handle_t get_entry_handle(const MatchEntry &entry) const {
    return ((char *) &entry - (char *) entries.data()) / sizeof(entry);
  }

private:
  std::vector<LongestPrefixMatchEntry> entries{};
  LPMTrie entries_trie;
};


class TernaryMatchTable : public MatchTable
{
public:
  TernaryMatchTable(const std::string &name, p4object_id_t id,
		    int size, int nbytes_key,
		    const MatchKeyBuilder &match_key_builder,
		    bool with_counters = false)
    : MatchTable(name, id, size, nbytes_key, match_key_builder, with_counters) {
    entries = std::vector<TernaryMatchEntry>(size);
  }

  const TernaryMatchEntry *lookup(const ByteContainer &key) const;
  // ErrorCode add_entry(const TernaryMatchEntry &entry, entry_handle_t *handle);
  ErrorCode add_entry(TernaryMatchEntry &&entry, entry_handle_t *handle);
  ErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
		      const ActionFn &action_fn,
		      const ActionData &action_data,
		      entry_handle_t *handle,
		      int priority = -1) override;
  ErrorCode delete_entry(entry_handle_t handle);
  ErrorCode modify_entry(entry_handle_t handle,
			 const ActionFnEntry &action_entry,
			 const ControlFlowNode *next_table);

  entry_handle_t get_entry_handle(const MatchEntry &entry) const {
    return ((char *) &entry - (char *) entries.data()) / sizeof(entry);
  }

private:
  std::vector<TernaryMatchEntry> entries{};
};

#endif
