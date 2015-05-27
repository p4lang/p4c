#include "bm_sim/match_tables.h"

typedef MatchTableAbstract::ActionEntry ActionEntry;

const ActionEntry &
MatchTableAbstract::lookup_action(const Packet &pkt)
{
  const ActionEntry *action_entry;
  entry_handle_t handle;
  lookup(pkt, &action_entry, &handle);
  if(action_entry) {
    // ELOGGER->table_hit(*pkt, *this, *entry);
    if(with_counters) update_counters(handle, pkt);
    return *action_entry;
  }
  else {
    // ELOGGER->table_miss(*pkt, *this);
    return default_entry;
  }
}

MatchErrorCode
MatchTableAbstract::query_counters(entry_handle_t handle,
				   counter_value_t *bytes,
				   counter_value_t *packets) const {
  if(!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  const Counter &c = counters[handle];
  *bytes = c.bytes;
  *packets = c.packets;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::reset_counters() {
  if(!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  // could take a while, but do not block anyone else
  // alternative would be to do a fill and use a lock
  for(Counter &c : counters) {
    c.bytes = 0;
    c.packets = 0;
  }
  return MatchErrorCode::SUCCESS;
}

void
MatchTable::lookup(
  const Packet &pkt, const ActionEntry **action_entry, entry_handle_t *handle
)
{
  MatchUnitAbstract<ActionEntry>::MatchUnitLookup res = match_unit->lookup(pkt);
  *action_entry = res.value;
  *handle = res.handle;
}

MatchErrorCode
MatchTable::add_entry(
  const std::vector<MatchKeyParam> &match_key,
  const ActionFn *action_fn, ActionData action_data, // move it
  entry_handle_t *handle, int priority
)
{
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());
  return match_unit->add_entry(
    match_key,
    ActionEntry(std::move(action_fn_entry), next_node),
    handle, priority
  );
}

MatchErrorCode
MatchTable::delete_entry(entry_handle_t handle)
{
  return match_unit->delete_entry(handle);
}

MatchErrorCode
MatchTable::modify_entry(
  entry_handle_t handle, const ActionFn *action_fn, ActionData action_data
)
{
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());
  return match_unit->modify_entry(
    handle, ActionEntry(std::move(action_fn_entry), next_node)
  );
}

MatchErrorCode
MatchTable::set_default_action(
  const ActionFn *action_fn, ActionData action_data
)
{
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());
  default_entry = ActionEntry(std::move(action_fn_entry), next_node);
  return MatchErrorCode::SUCCESS;
}
