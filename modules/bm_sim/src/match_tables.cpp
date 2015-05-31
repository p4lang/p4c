#include "bm_sim/match_tables.h"

typedef MatchTableAbstract::ActionEntry ActionEntry;

const ControlFlowNode *
MatchTableAbstract::apply_action(Packet *pkt)
{
  entry_handle_t handle;
  bool hit;

  ReadLock lock = lock_read();

  const ActionEntry &action_entry = lookup(*pkt, &hit, &handle);

  // we're holding the lock for this...
  if(hit) {
    ELOGGER->table_hit(*pkt, *this, handle);
  }
  else {
    ELOGGER->table_miss(*pkt, *this);
  }

  action_entry.action_fn(pkt);

  const ControlFlowNode *next_node = action_entry.next_node;

  lock.unlock();

  if(hit && with_counters) update_counters(handle, *pkt);

  return next_node;
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

const ActionEntry &
MatchTable::lookup(
  const Packet &pkt, bool *hit, entry_handle_t *handle
)
{
  MatchUnitAbstract<ActionEntry>::MatchUnitLookup res = match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;
  return (*hit) ? (*res.value) : default_entry;
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

  WriteLock lock = lock_write();

  return match_unit->add_entry(
    match_key,
    ActionEntry(std::move(action_fn_entry), next_node),
    handle, priority
  );
}

MatchErrorCode
MatchTable::delete_entry(entry_handle_t handle)
{
  WriteLock lock = lock_write();

  return match_unit->delete_entry(handle);
}

MatchErrorCode
MatchTable::modify_entry(
  entry_handle_t handle, const ActionFn *action_fn, ActionData action_data
)
{
  WriteLock lock = lock_write();

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

  WriteLock lock = lock_write();

  default_entry = ActionEntry(std::move(action_fn_entry), next_node);

  return MatchErrorCode::SUCCESS;
}

std::unique_ptr<MatchTable>
MatchTable::create(
  const std::string &match_type, const std::string &name, p4object_id_t id,
  size_t size, const MatchKeyBuilder &match_key_builder, bool with_counters
)
{
  typedef MatchUnitExact<ActionEntry> MUExact;
  typedef MatchUnitLPM<ActionEntry> MULPM;
  typedef MatchUnitTernary<ActionEntry> MUTernary;

  std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit;
  if(match_type == "exact")
    match_unit = std::unique_ptr<MUExact>(new MUExact(size, match_key_builder));
  else if(match_type == "lpm")
    match_unit = std::unique_ptr<MULPM>(new MULPM(size, match_key_builder));
  else if(match_type == "ternary")
    match_unit = std::unique_ptr<MUTernary>(new MUTernary(size, match_key_builder));
  else
    assert(0 && "invalid match type");
  
  return std::unique_ptr<MatchTable>(
    new MatchTable(name, id, std::move(match_unit), with_counters)
  );
}

std::unique_ptr<MatchTableIndirect>
MatchTableIndirect::create(
  const std::string &match_type, 
  const std::string &name, p4object_id_t id,
  size_t size, const MatchKeyBuilder &match_key_builder,
  bool with_counters
)
{
  typedef MatchUnitExact<IndirectIndex> MUExact;
  typedef MatchUnitLPM<IndirectIndex> MULPM;
  typedef MatchUnitTernary<IndirectIndex> MUTernary;
  
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit;
  if(match_type == "exact")
    match_unit = std::unique_ptr<MUExact>(new MUExact(size, match_key_builder));
  else if(match_type == "lpm")
    match_unit = std::unique_ptr<MULPM>(new MULPM(size, match_key_builder));
  else if(match_type == "ternary")
    match_unit = std::unique_ptr<MUTernary>(new MUTernary(size, match_key_builder));
  else
    assert(0 && "invalid match type");

  return std::unique_ptr<MatchTableIndirect>(
    new MatchTableIndirect(name, id, std::move(match_unit), with_counters)
  );
}

const ActionEntry &
MatchTableIndirect::lookup(
  const Packet &pkt, bool *hit, entry_handle_t *handle
)
{
  MatchUnitAbstract<IndirectIndex>::MatchUnitLookup res = match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;

  mbr_hdl_t mbr = (*hit) ? (*res.value).get() : default_index.get();
  assert(mbr_handles.valid_handle(mbr));

  return action_entries[mbr];
}

void
MatchTableIndirect::entries_insert(mbr_hdl_t mbr, ActionEntry &&entry)
{
  assert(mbr <= (unsigned) action_entries.size());
  assert(action_entries.size() == mbr_ref_count.size());

  if(mbr == action_entries.size()) {
    action_entries.push_back(std::move(entry));
    mbr_ref_count.push_back(0);
  }
  else {
    action_entries[mbr] = std::move(entry);
    mbr_ref_count[mbr] = 0;
  }
}

MatchErrorCode
MatchTableIndirect::add_member(
  const ActionFn *action_fn, ActionData action_data,
  mbr_hdl_t *mbr
)
{
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  WriteLock lock = lock_write();

  if(mbr_handles.get_handle(mbr)) return MatchErrorCode::ERROR;

  entries_insert(*mbr, ActionEntry(std::move(action_fn_entry), next_node));

  num_members++;

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::delete_member(mbr_hdl_t mbr)
{
  WriteLock lock = lock_write();

  if(mbr_handles.release_handle(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;

  // TODO: check if an entry is still pointing to it
  if(mbr_ref_count[mbr] > 0) return MatchErrorCode::MBR_STILL_USED;

  num_members--;

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::add_entry(
  const std::vector<MatchKeyParam> &match_key, mbr_hdl_t mbr,
  entry_handle_t *handle, int priority
)
{
  WriteLock lock = lock_write();

  if(!mbr_handles.valid_handle(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  mbr_ref_count[mbr]++;
  return match_unit->add_entry(
    match_key, IndirectIndex::make_mbr_index(mbr), handle, priority
  );
}

MatchErrorCode
MatchTableIndirect::delete_entry(entry_handle_t handle)
{
  WriteLock lock = lock_write();

  return match_unit->delete_entry(handle);
}

MatchErrorCode
MatchTableIndirect::modify_entry(
  entry_handle_t handle, mbr_hdl_t mbr
)
{
  WriteLock lock = lock_write();

  if(!mbr_handles.valid_handle(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  mbr_ref_count[mbr]--;
  return match_unit->modify_entry(handle, IndirectIndex::make_mbr_index(mbr));
}

MatchErrorCode
MatchTableIndirect::set_default_member(mbr_hdl_t mbr)
{
  WriteLock lock = lock_write();

  if(!mbr_handles.valid_handle(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  default_index = IndirectIndex::make_mbr_index(mbr);

  return MatchErrorCode::SUCCESS;
}
