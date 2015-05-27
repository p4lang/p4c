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

  const ActionEntry &lookup_action(const Packet &pkt);

  virtual void lookup(const Packet &pkt,
		      const ActionEntry **action_entry,
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
  const ControlFlowNode *get_next_node(p4object_id_t action_id) const {
    return next_nodes.at(action_id);
  }

  void update_counters(entry_handle_t handle, const Packet &pkt) {
    assert(with_counters);
    Counter &c = counters[handle];
    c.bytes += pkt.get_ingress_length();
    c.packets += 1;
  }

protected:
  size_t size{0};

  std::atomic_bool with_counters{false};
  std::vector<Counter> counters{};

  std::unordered_map<p4object_id_t, const ControlFlowNode *> next_nodes{};
  ActionEntry default_entry{};
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

  void lookup(const Packet &pkt,
	      const ActionEntry **action_entry,
	      entry_handle_t *handle) override;

  size_t get_num_entries() const override {
    return match_unit->get_num_entries();
  }

public:
  template <template <class C> class U>
  static std::unique_ptr<MatchTable> create_match_table(
    const std::string &name, p4object_id_t id,
    size_t size, size_t nbytes_key, const MatchKeyBuilder &match_key_builder,
    bool with_counters
  ) {
    static_assert(
      std::is_base_of<MatchUnitAbstract<ActionEntry>, U<ActionEntry> >::value,
      "incorrect class template"
    );
    std::unique_ptr<U<ActionEntry> > match_unit(
      new U<ActionEntry>(size, nbytes_key, match_key_builder)
    );    
    return std::unique_ptr<MatchTable>(
      new MatchTable(name, id, std::move(match_unit), with_counters)
    );
  }

private:
  std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit;

};

// class MatchTableIndirectAction : public MatchTableAbstract
// {
// public:
//   MatchTableIndirectAction() 
//     : match_unit(new MatchUnitAbstract<int>())
//   {

//   }

//   const ActionEntry &operator(const Packet &pkt) {

//   }

//   int add_entry(const std::vector<MatchKeyParam> &match_key,
// 		int member_hdl,
// 		entry_handle_t *handle,
// 		int priority = -1);

// private:
//   std::unique_ptr<MatchUnitAbstract<int> > match_unit;

// };

#endif
