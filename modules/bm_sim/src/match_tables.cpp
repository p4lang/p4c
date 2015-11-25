/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include "bm_sim/match_tables.h"
#include "bm_sim/logger.h"

namespace {

template <typename V>
std::unique_ptr<MatchUnitAbstract<V> > create_match_unit(
  const std::string match_type,
  const size_t size, const MatchKeyBuilder &match_key_builder
)
{
  typedef MatchUnitExact<V> MUExact;
  typedef MatchUnitLPM<V> MULPM;
  typedef MatchUnitTernary<V> MUTernary;

  std::unique_ptr<MatchUnitAbstract<V> > match_unit;
  if(match_type == "exact")
    match_unit = std::unique_ptr<MUExact>(new MUExact(size, match_key_builder));
  else if(match_type == "lpm")
    match_unit = std::unique_ptr<MULPM>(new MULPM(size, match_key_builder));
  else if(match_type == "ternary")
    match_unit = std::unique_ptr<MUTernary>(new MUTernary(size, match_key_builder));
  else
    assert(0 && "invalid match type");
  return match_unit;
}

}

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
    BMLOG_DEBUG_PKT(*pkt, "Table '{}': hit with handle {}",
		    get_name(), handle);
  }
  else {
    ELOGGER->table_miss(*pkt, *this);
    BMLOG_DEBUG_PKT(*pkt, "Table '{}': miss", get_name());
  }

  BMLOG_DEBUG_PKT(*pkt, "Action entry is {}", action_entry);

  action_entry.action_fn(pkt);

  const ControlFlowNode *next_node = action_entry.next_node;

  return next_node;
}

void
MatchTableAbstract::reset_state() {
  WriteLock lock = lock_write();
  reset_state_();
}

MatchErrorCode
MatchTableAbstract::query_counters(entry_handle_t handle,
				   counter_value_t *bytes,
				   counter_value_t *packets) const
{
  ReadLock lock = lock_read();
  if(!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  if(!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  const MatchUnit::EntryMeta &meta = match_unit_->get_entry_meta(handle);
  // should I hide counter implementation more?
  meta.counter.query_counter(bytes, packets);
  return MatchErrorCode::SUCCESS;
}

/* really needed ? */
MatchErrorCode
MatchTableAbstract::reset_counters()
{
  if(!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  match_unit_->reset_counters();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::write_counters(entry_handle_t handle,
				   counter_value_t bytes,
				   counter_value_t packets)
{
  ReadLock lock = lock_write();
  if(!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  if(!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  MatchUnit::EntryMeta &meta = match_unit_->get_entry_meta(handle);
  // should I hide counter implementation more?
  meta.counter.write_counter(bytes, packets);
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::set_entry_ttl(
  entry_handle_t handle, unsigned int ttl_ms
)
{
  if(!with_ageing) return MatchErrorCode::AGEING_DISABLED;
  WriteLock lock = lock_write();
  return match_unit_->set_entry_ttl(handle, ttl_ms);
}

void
MatchTableAbstract::sweep_entries(std::vector<entry_handle_t> &entries) const
{
  ReadLock lock = lock_read(); // TODO: how to avoid this?
  match_unit_->sweep_entries(entries);
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

void
MatchTable::dump(std::ostream &stream) const
{
  ReadLock lock = lock_read();
  stream << name << ":\n";
  match_unit->dump(stream);
  stream << "default: " << default_entry << "\n";
}

void
MatchTable::reset_state_()
{
  // reset default_entry ?
  match_unit->reset_state();
}

std::unique_ptr<MatchTable>
MatchTable::create(
  const std::string &match_type, const std::string &name, p4object_id_t id,
  size_t size, const MatchKeyBuilder &match_key_builder,
  bool with_counters, bool with_ageing
)
{
  std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit = 
    create_match_unit<ActionEntry>(match_type, size, match_key_builder);

  return std::unique_ptr<MatchTable>(
    new MatchTable(name, id, std::move(match_unit), with_counters, with_ageing)
  );
}

std::unique_ptr<MatchTableIndirect>
MatchTableIndirect::create(
  const std::string &match_type, 
  const std::string &name, p4object_id_t id,
  size_t size, const MatchKeyBuilder &match_key_builder,
  bool with_counters, bool with_ageing
)
{
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit = 
    create_match_unit<IndirectIndex>(match_type, size, match_key_builder);

  return std::unique_ptr<MatchTableIndirect>(
    new MatchTableIndirect(name, id, std::move(match_unit), with_counters, with_ageing)
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

  const IndirectIndex &index = (*hit) ? *res.value : default_index;
  mbr_hdl_t mbr = index.get_mbr();
  assert(is_valid_mbr(mbr));

  return action_entries[mbr];
}

void
MatchTableIndirect::entries_insert(mbr_hdl_t mbr, ActionEntry &&entry)
{
  assert(mbr <= action_entries.size());

  if(mbr == action_entries.size()) {
    action_entries.push_back(std::move(entry));
  }
  else {
    action_entries[mbr] = std::move(entry);
  }

  index_ref_count.set(IndirectIndex::make_mbr_index(mbr), 0);
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

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;

  if(index_ref_count.get(IndirectIndex::make_mbr_index(mbr)) > 0)
    return MatchErrorCode::MBR_STILL_USED;

  assert(!mbr_handles.release_handle(mbr));

  num_members--;

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::modify_member(
  mbr_hdl_t mbr, const ActionFn *action_fn, ActionData action_data
)
{
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  WriteLock lock = lock_write();

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;

  action_entries[mbr] = ActionEntry(std::move(action_fn_entry), next_node);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::add_entry(
  const std::vector<MatchKeyParam> &match_key, mbr_hdl_t mbr,
  entry_handle_t *handle, int priority
)
{
  WriteLock lock = lock_write();

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  IndirectIndex index = IndirectIndex::make_mbr_index(mbr);
  index_ref_count.increase(index);
  return match_unit->add_entry(match_key, std::move(index), handle, priority);
}

MatchErrorCode
MatchTableIndirect::delete_entry(entry_handle_t handle)
{
  WriteLock lock = lock_write();

  const IndirectIndex *index;
  MatchErrorCode rc = match_unit->get_value(handle, &index);
  if(rc != MatchErrorCode::SUCCESS) return rc;
  index_ref_count.decrease(*index);
  
  return match_unit->delete_entry(handle);
}

MatchErrorCode
MatchTableIndirect::modify_entry(
  entry_handle_t handle, mbr_hdl_t mbr
)
{
  WriteLock lock = lock_write();

  const IndirectIndex *index;
  MatchErrorCode rc = match_unit->get_value(handle, &index);
  if(rc != MatchErrorCode::SUCCESS) return rc;
  index_ref_count.decrease(*index);

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;

  IndirectIndex new_index = IndirectIndex::make_mbr_index(mbr);
  index_ref_count.increase(new_index);

  return match_unit->modify_entry(handle, std::move(new_index));
}

MatchErrorCode
MatchTableIndirect::set_default_member(mbr_hdl_t mbr)
{
  WriteLock lock = lock_write();

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  default_index = IndirectIndex::make_mbr_index(mbr);

  return MatchErrorCode::SUCCESS;
}

void
MatchTableIndirect::dump_(std::ostream &stream) const
{
  stream << name << ":\n";
  match_unit->dump(stream);
  stream << "members:\n";
  for(mbr_hdl_t mbr : mbr_handles) {
    stream << mbr << ": ";
    action_entries[mbr].dump(stream);
    stream << "\n";
  }
}

void
MatchTableIndirect::dump(std::ostream &stream) const
{
  ReadLock lock = lock_read();
  dump_(stream);
}

void
MatchTableIndirect::reset_state_()
{
  index_ref_count = IndirectIndexRefCount();
  mbr_handles.clear();
  action_entries.clear();
  num_members = 0;
  match_unit->reset_state();
}


MatchErrorCode
MatchTableIndirectWS::GroupInfo::add_member(mbr_hdl_t mbr)
{
  if(!mbrs.add(mbr)) return MatchErrorCode::MBR_ALREADY_IN_GRP;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::GroupInfo::delete_member(mbr_hdl_t mbr)
{
  if(!mbrs.remove(mbr)) return MatchErrorCode::MBR_NOT_IN_GRP;
  return MatchErrorCode::SUCCESS;
}

bool
MatchTableIndirectWS::GroupInfo::contains_member(mbr_hdl_t mbr) const
{
  return mbrs.contains(mbr);
}

size_t
MatchTableIndirectWS::GroupInfo::size() const
{
  return mbrs.count();
}

MatchTableIndirect::mbr_hdl_t
MatchTableIndirectWS::GroupInfo::get_nth(size_t n) const
{
  return mbrs.get_nth(n);
}

void
MatchTableIndirectWS::GroupInfo::dump(std::ostream &stream) const
{
  stream << "{ ";
  for(const auto &mbr : mbrs)
    stream << mbr << ", ";
  stream << "}";
}


std::unique_ptr<MatchTableIndirectWS>
MatchTableIndirectWS::create(
  const std::string &match_type, 
  const std::string &name, p4object_id_t id,
  size_t size, const MatchKeyBuilder &match_key_builder,
  bool with_counters, bool with_ageing
)
{
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit = 
    create_match_unit<IndirectIndex>(match_type, size, match_key_builder);

  return std::unique_ptr<MatchTableIndirectWS>(
    new MatchTableIndirectWS(name, id, std::move(match_unit), with_counters, with_ageing)
  );
}

MatchTableIndirect::mbr_hdl_t
MatchTableIndirectWS::choose_from_group(grp_hdl_t grp, const Packet &pkt) const
{
  const GroupInfo &group_info = group_entries[grp];
  size_t s = group_info.size();
  assert(s > 0);
  if(!hash) return group_info.get_nth(0);
  hash_t h = static_cast<hash_t>(hash->output(pkt));
  return group_info.get_nth(h % s);
}

const ActionEntry &
MatchTableIndirectWS::lookup(
  const Packet &pkt, bool *hit, entry_handle_t *handle
)
{
  MatchUnitAbstract<IndirectIndex>::MatchUnitLookup res = match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;

  const IndirectIndex &index = (*hit) ? *res.value : default_index;

  mbr_hdl_t mbr;
  if(index.is_mbr()) {
    mbr = index.get_mbr();
  }
  else {
    grp_hdl_t grp = index.get_grp();
    assert(is_valid_grp(grp));
    mbr = choose_from_group(grp, pkt);
  }
  assert(is_valid_mbr(mbr));

  return action_entries[mbr];
}

void
MatchTableIndirectWS::groups_insert(grp_hdl_t grp)
{
  assert(grp <= group_entries.size());

  if(grp == group_entries.size()) {
    group_entries.emplace_back();
  }
  else {
    group_entries[grp] = GroupInfo();
  }

  index_ref_count.set(IndirectIndex::make_grp_index(grp), 0);
}

MatchErrorCode
MatchTableIndirectWS::create_group(grp_hdl_t *grp)
{
  WriteLock lock = lock_write();

  if(grp_handles.get_handle(grp)) return MatchErrorCode::ERROR;

  groups_insert(*grp);

  num_groups++;

  return MatchErrorCode::SUCCESS;  
}

MatchErrorCode
MatchTableIndirectWS::delete_group(grp_hdl_t grp)
{
  WriteLock lock = lock_write();

  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  if(index_ref_count.get(IndirectIndex::make_grp_index(grp)) > 0)
    return MatchErrorCode::GRP_STILL_USED;

  /* we allow deletion of non-empty groups, but we must remember to decrease the
     ref count for the members. Note that we do not allow deletion of a member
     which is in a group */
  GroupInfo &group_info = group_entries[grp];
  for(auto mbr : group_info)
    index_ref_count.decrease(IndirectIndex::make_mbr_index(mbr));

  assert(!grp_handles.release_handle(grp));

  num_groups--;

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::add_member_to_group(mbr_hdl_t mbr, grp_hdl_t grp)
{
  WriteLock lock = lock_write();

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  GroupInfo &group_info = group_entries[grp];
  MatchErrorCode rc = group_info.add_member(mbr);
  if(rc != MatchErrorCode::SUCCESS) return rc;

  index_ref_count.increase(IndirectIndex::make_mbr_index(mbr));

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::remove_member_from_group(mbr_hdl_t mbr, grp_hdl_t grp)
{
  WriteLock lock = lock_write();

  if(!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  GroupInfo &group_info = group_entries[grp];
  MatchErrorCode rc = group_info.delete_member(mbr);
  if(rc != MatchErrorCode::SUCCESS) return rc;

  index_ref_count.decrease(IndirectIndex::make_mbr_index(mbr));

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::add_entry_ws(
  const std::vector<MatchKeyParam> &match_key,
  grp_hdl_t grp, entry_handle_t *handle, int priority
)
{
  WriteLock lock = lock_write();

  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  if(get_grp_size(grp) == 0) return MatchErrorCode::EMPTY_GRP;

  IndirectIndex index = IndirectIndex::make_grp_index(grp);
  index_ref_count.increase(index);
  return match_unit->add_entry(match_key, std::move(index), handle, priority);
}

MatchErrorCode
MatchTableIndirectWS::modify_entry_ws(
  entry_handle_t handle, grp_hdl_t grp
)
{
  WriteLock lock = lock_write();

  const IndirectIndex *index;
  MatchErrorCode rc = match_unit->get_value(handle, &index);
  if(rc != MatchErrorCode::SUCCESS) return rc;
  index_ref_count.decrease(*index);

  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  if(get_grp_size(grp) == 0) return MatchErrorCode::EMPTY_GRP;

  IndirectIndex new_index = IndirectIndex::make_grp_index(grp);
  index_ref_count.increase(new_index);

  return match_unit->modify_entry(handle, std::move(new_index));
}

MatchErrorCode
MatchTableIndirectWS::set_default_group(grp_hdl_t grp)
{
  WriteLock lock = lock_write();

  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;
  default_index = IndirectIndex::make_grp_index(grp);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::get_num_members_in_group(grp_hdl_t grp, size_t *nb) const
{
  ReadLock lock = lock_read();

  if(!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  const GroupInfo &group_info = group_entries[grp];
  *nb = group_info.size();

  return MatchErrorCode::SUCCESS;
}

void
MatchTableIndirectWS::dump(std::ostream &stream) const
{
  ReadLock lock = lock_read();
  MatchTableIndirect::dump_(stream);
  stream << "groups:\n";
  for(grp_hdl_t grp : grp_handles) {
    stream << grp << ": ";
    group_entries[grp].dump(stream);
    stream << "\n";
  }
}

void
MatchTableIndirectWS::reset_state_()
{
  MatchTableIndirect::reset_state_();
  num_groups = 0;
  group_entries.clear();
}
