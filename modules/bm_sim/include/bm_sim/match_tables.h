#ifndef _BM_MATCH_TABLES_H_
#define _BM_MATCH_TABLES_H_

#include <atomic>
#include <vector>
#include <type_traits>

#include "match_units.h"
#include "actions.h"

class MatchTableAbstract : public NamedP4Object
{
public:
  struct Counter {
    std::atomic<std::uint_fast64_t> bytes{0};
    std::atomic<std::uint_fast64_t> packets{0};
  };
  typedef uint64_t counter_value_t;

  struct ActionEntry {
    ActionEntry() { }

    ActionEntry(ActionFnEntry action_fn, const ControlFlowNode *next_node)
      : action_fn(std::move(action_fn)), next_node(next_node) { }

    ActionEntry(const ActionEntry &other) = delete;
    ActionEntry &operator=(const ActionEntry &other) = delete;
    
    ActionEntry(ActionEntry &&other) /*noexcept*/ = default;
    ActionEntry &operator=(ActionEntry &&other) /*noexcept*/ = default;

    ActionFnEntry action_fn{};
    const ControlFlowNode *next_node{nullptr};
  };

public:
  MatchTableAbstract(const std::string &name, p4object_id_t id,
		     size_t size, bool with_counters)
    : NamedP4Object(name, id), size(size),
      with_counters(with_counters), counters(size) { }

  virtual ~MatchTableAbstract() { }

  const ControlFlowNode *apply_action(Packet *pkt);

  virtual const ActionEntry &lookup(const Packet &pkt, bool *hit,
				    entry_handle_t *handle) = 0;

  virtual size_t get_num_entries() const = 0;

  void set_next_node(p4object_id_t action_id, const ControlFlowNode *next_node) {
    next_nodes[action_id] = next_node;
  }

  MatchErrorCode query_counters(entry_handle_t handle,
				counter_value_t *bytes,
				counter_value_t *packets) const;
  MatchErrorCode reset_counters();

protected:
  typedef boost::shared_lock<boost::shared_mutex> ReadLock;
  typedef boost::unique_lock<boost::shared_mutex> WriteLock;

protected:
  const ControlFlowNode *get_next_node(p4object_id_t action_id) const {
    return next_nodes.at(action_id);
  }

  void update_counters(entry_handle_t handle, const Packet &pkt) {
    assert(with_counters);
    Counter &c = counters[handle];
    c.bytes += pkt.get_ingress_length();
    c.packets += 1;
  }

  ReadLock lock_read() { return ReadLock(t_mutex); }
  WriteLock lock_write() { return WriteLock(t_mutex); }
  void unlock(ReadLock &lock) { lock.unlock(); }
  void unlock(WriteLock &lock) { lock.unlock(); }

protected:
  size_t size{0};

  std::atomic_bool with_counters{false};
  std::vector<Counter> counters{};

  std::unordered_map<p4object_id_t, const ControlFlowNode *> next_nodes{};

private:
  mutable boost::shared_mutex t_mutex{};
};

// MatchTable is exposed to the runtime for configuration

class MatchTable : public MatchTableAbstract
{
public:
  typedef MatchTableAbstract::ActionEntry ActionEntry;

public:
  MatchTable(const std::string &name, p4object_id_t id,
	     std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit,
	     bool with_counters = false) 
    : MatchTableAbstract(name, id, match_unit->get_size(), with_counters),
      match_unit(std::move(match_unit)) { }

  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   const ActionFn *action_fn,
			   ActionData action_data, // move it
			   entry_handle_t *handle,
			   int priority = -1);

  MatchErrorCode delete_entry(entry_handle_t handle);

  MatchErrorCode modify_entry(entry_handle_t handle,
			      const ActionFn *action_fn,
			      ActionData action_data);

  MatchErrorCode set_default_action(const ActionFn *action_fn,
				    ActionData action_data);

  const ActionEntry &lookup(const Packet &pkt, bool *hit,
			    entry_handle_t *handle) override;

  size_t get_num_entries() const override {
    return match_unit->get_num_entries();
  }

public:
  static std::unique_ptr<MatchTable> create(
    const std::string &match_type, 
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters
  );

private:
  ActionEntry default_entry{};
  std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit;

};

class MatchTableIndirect : public MatchTableAbstract
{
public:
  typedef MatchTableAbstract::ActionEntry ActionEntry;

  typedef uintptr_t mbr_hdl_t;

  class IndirectIndex {
  public:
    IndirectIndex() { }

    bool is_mbr() const { return ((index >> 24) == _mbr); }
    bool is_grp() const { return ((index >> 24) == _grp); }
    mbr_hdl_t get() const { return (index & _index_mask); }

    static IndirectIndex make_mbr_index(unsigned int index) {
      assert(index <= _index_mask);
      return IndirectIndex((_mbr << 24) | index);
    }

    static IndirectIndex make_grp_index(unsigned int index) {
      assert(index <= _index_mask);
      return IndirectIndex((_grp << 24) | index);
    }

  private:
    IndirectIndex(unsigned int index) : index(index) { }

    static const unsigned char _mbr = 0x00;
    static const unsigned char _grp = 0x01;
    static const unsigned int _index_mask = 0x00FFFFFF;

    unsigned int index{0};
  };

public:
  MatchTableIndirect(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit,
    bool with_counters = false
  ) 
    : MatchTableAbstract(name, id, match_unit->get_size(), with_counters),
      match_unit(std::move(match_unit)) { }

  MatchErrorCode add_member(const ActionFn *action_fn,
			    ActionData action_data, // move it
			    mbr_hdl_t *mbr);

  MatchErrorCode delete_member(mbr_hdl_t mbr);

  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   mbr_hdl_t mbr,
			   entry_handle_t *handle,
			   int priority = -1);

  MatchErrorCode delete_entry(entry_handle_t handle);

  MatchErrorCode modify_entry(entry_handle_t handle, mbr_hdl_t mbr);

  MatchErrorCode set_default_member(mbr_hdl_t mbr);

  const ActionEntry &lookup(const Packet &pkt, bool *hit,
			    entry_handle_t *handle) override;

  size_t get_num_entries() const override {
    return match_unit->get_num_entries();
  }

  size_t get_num_members() const {
    return num_members;
  }

public:
  static std::unique_ptr<MatchTableIndirect> create(
    const std::string &match_type, 
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters
  );

private:
  void entries_insert(mbr_hdl_t mbr, ActionEntry &&entry);

private:
  IndirectIndex default_index{};
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit;
  HandleMgr mbr_handles{};
  size_t num_members{0};
  std::vector<ActionEntry> action_entries{};
  std::vector<size_t> mbr_ref_count{};
};

#endif
