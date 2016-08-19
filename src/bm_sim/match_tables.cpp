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

#include <bm/bm_sim/match_tables.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/lookup_structures.h>
#include <bm/bm_sim/P4Objects.h>

#include <string>
#include <vector>
#include <limits>  // std::numeric_limits

namespace bm {

namespace {

template <typename V>
std::unique_ptr<MatchUnitAbstract<V> >
create_match_unit(const std::string match_type, const size_t size,
                  const MatchKeyBuilder &match_key_builder,
                  LookupStructureFactory *lookup_factory) {
  typedef MatchUnitExact<V> MUExact;
  typedef MatchUnitLPM<V> MULPM;
  typedef MatchUnitTernary<V> MUTernary;
  typedef MatchUnitRange<V> MURange;

  std::unique_ptr<MatchUnitAbstract<V> > match_unit;
  if (match_type == "exact")
    match_unit = std::unique_ptr<MUExact>(
        new MUExact(size, match_key_builder, lookup_factory));
  else if (match_type == "lpm")
    match_unit = std::unique_ptr<MULPM>(
        new MULPM(size, match_key_builder, lookup_factory));
  else if (match_type == "ternary")
    match_unit = std::unique_ptr<MUTernary>(
        new MUTernary(size, match_key_builder, lookup_factory));
  else if (match_type == "range")
    match_unit = std::unique_ptr<MURange>(
        new MURange(size, match_key_builder, lookup_factory));
  else
    assert(0 && "invalid match type");
  return match_unit;
}

}  // namespace

typedef MatchTableAbstract::ActionEntry ActionEntry;

void
ActionEntry::serialize(std::ostream *out) const {
  action_fn.serialize(out);
  if (!next_node)
    (*out) << "__NULL__" << "\n";
  else
    (*out) << next_node->get_name() << "\n";
}

void
ActionEntry::deserialize(std::istream *in, const P4Objects &objs) {
  action_fn.deserialize(in, objs);
  std::string next_node_name; (*in) >> next_node_name;
  if (next_node_name != "__NULL__") {
    next_node = objs.get_control_node(next_node_name);
    assert(next_node);
  }
}

MatchTableAbstract::MatchTableAbstract(
    const std::string &name, p4object_id_t id,
    bool with_counters, bool with_ageing,
    MatchUnitAbstract_ *mu)
    : NamedP4Object(name, id),
      with_counters(with_counters), with_ageing(with_ageing),
      match_unit_(mu) { }

const ControlFlowNode *
MatchTableAbstract::apply_action(Packet *pkt) {
  entry_handle_t handle;
  bool hit;

  ReadLock lock = lock_read();

  const ActionEntry &action_entry = lookup(*pkt, &hit, &handle);

  if (hit && with_meters) {
    // we only execute the direct meter if hit, should we have a miss meter?
    Field &target_f = pkt->get_phv()->get_field(
        meter_target_header, meter_target_offset);
    Meter &meter = match_unit_->get_meter(handle);
    target_f.set(meter.execute(*pkt));
  }

  // we're holding the lock for this...
  if (hit) {
    BMELOG(table_hit, *pkt, *this, handle);
    BMLOG_DEBUG_PKT(*pkt, "Table '{}': hit with handle {}",
                    get_name(), handle);
    // TODO(antonin): change to trace?
    BMLOG_DEBUG_PKT(*pkt, "{}", dump_entry_string_(handle));
  } else {
    BMELOG(table_miss, *pkt, *this);
    BMLOG_DEBUG_PKT(*pkt, "Table '{}': miss", get_name());
  }

  DEBUGGER_NOTIFY_UPDATE_V(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      Debugger::FIELD_ACTION, action_entry.action_fn.get_action_id());

  BMLOG_DEBUG_PKT(*pkt, "Action entry is {}", action_entry);

  action_entry.action_fn(pkt);

  return hit ? action_entry.next_node : next_node_miss;
}

void
MatchTableAbstract::reset_state() {
  WriteLock lock = lock_write();
  reset_state_();
}

void
MatchTableAbstract::serialize(std::ostream *out) const {
  ReadLock lock = lock_read();
  (*out) << name << "\n";
  if (!next_node_miss)
    (*out) << "__NULL__" << "\n";
  else
    (*out) << next_node_miss->get_name() << "\n";
  serialize_(out);
}

void
MatchTableAbstract::deserialize(std::istream *in, const P4Objects &objs) {
  WriteLock lock = lock_write();
  std::string name_sentinel; (*in) >> name_sentinel;
  assert(name_sentinel == name);
  std::string next_node_miss_name; (*in) >> next_node_miss_name;
  if (next_node_miss_name != "__NULL__") {
    next_node_miss = objs.get_control_node(next_node_miss_name);
    assert(next_node_miss);
  }
  deserialize_(in, objs);
}

void
MatchTableAbstract::set_next_node(p4object_id_t action_id,
                                  const ControlFlowNode *next_node) {
  next_nodes[action_id] = next_node;
}

void
MatchTableAbstract::set_next_node_hit(const ControlFlowNode *next_node) {
  has_next_node_hit = true;
  next_node_hit = next_node;
}

void
MatchTableAbstract::set_next_node_miss(const ControlFlowNode *next_node) {
  has_next_node_miss = true;
  next_node_miss = next_node;
  next_node_miss_default = next_node;
}

void
MatchTableAbstract::set_next_node_miss_default(
    const ControlFlowNode *next_node) {
  if (has_next_node_miss) return;
  next_node_miss = next_node;
  next_node_miss_default = next_node;
}

void
MatchTableAbstract::set_direct_meters(MeterArray *meter_array,
                                      header_id_t target_header,
                                      int target_offset) {
  WriteLock lock = lock_write();
  match_unit_->set_direct_meters(meter_array);
  meter_target_header = target_header;
  meter_target_offset = target_offset;
  with_meters = true;
}

MatchErrorCode
MatchTableAbstract::query_counters(entry_handle_t handle,
                                   counter_value_t *bytes,
                                   counter_value_t *packets) const {
  ReadLock lock = lock_read();
  if (!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  if (!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  const MatchUnit::EntryMeta &meta = match_unit_->get_entry_meta(handle);
  // should I hide counter implementation more?
  meta.counter.query_counter(bytes, packets);
  return MatchErrorCode::SUCCESS;
}

/* really needed ? */
MatchErrorCode
MatchTableAbstract::reset_counters() {
  if (!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  match_unit_->reset_counters();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::write_counters(entry_handle_t handle,
                                   counter_value_t bytes,
                                   counter_value_t packets) {
  ReadLock lock = lock_write();
  if (!with_counters) return MatchErrorCode::COUNTERS_DISABLED;
  if (!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  MatchUnit::EntryMeta &meta = match_unit_->get_entry_meta(handle);
  // should I hide counter implementation more?
  meta.counter.write_counter(bytes, packets);
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::set_meter_rates(
    entry_handle_t handle,
    const std::vector<Meter::rate_config_t> &configs) const {
  if (!with_meters) return MatchErrorCode::METERS_DISABLED;
  WriteLock lock = lock_write();
  if (!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  Meter &meter = match_unit_->get_meter(handle);
  Meter::MeterErrorCode rc = meter.set_rates(configs);
  return (rc == Meter::MeterErrorCode::SUCCESS) ?
      MatchErrorCode::SUCCESS : MatchErrorCode::ERROR;
}

MatchErrorCode
MatchTableAbstract::get_meter_rates(
    entry_handle_t handle, std::vector<Meter::rate_config_t> *configs) const {
  if (!with_meters) return MatchErrorCode::METERS_DISABLED;
  WriteLock lock = lock_write();
  if (!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  const Meter &meter = match_unit_->get_meter(handle);
  *configs = meter.get_rates();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::set_entry_ttl(entry_handle_t handle, unsigned int ttl_ms) {
  if (!with_ageing) return MatchErrorCode::AGEING_DISABLED;
  WriteLock lock = lock_write();
  return match_unit_->set_entry_ttl(handle, ttl_ms);
}

void
MatchTableAbstract::sweep_entries(std::vector<entry_handle_t> *entries) const {
  ReadLock lock = lock_read();  // TODO(antonin): how to avoid this?
  match_unit_->sweep_entries(entries);
}

MatchTableAbstract::handle_iterator
MatchTableAbstract::handles_begin() const {
  ReadLock lock = lock_read();
  return handle_iterator(this, match_unit_->handles_begin());
}

MatchTableAbstract::handle_iterator
MatchTableAbstract::handles_end() const {
  ReadLock lock = lock_read();
  return handle_iterator(this, match_unit_->handles_end());
}

const ControlFlowNode *
MatchTableAbstract::get_next_node(p4object_id_t action_id) const {
  if (has_next_node_hit)
    return next_node_hit;
  return next_nodes.at(action_id);
}

const ControlFlowNode *
MatchTableAbstract::get_next_node_default(p4object_id_t action_id) const {
  if (has_next_node_miss)
    return next_node_miss;
  return next_nodes.at(action_id);
}

std::string
MatchTableAbstract::dump_entry_string_(entry_handle_t handle) const {
  std::ostringstream ret;
  if (dump_entry_(&ret, handle) != MatchErrorCode::SUCCESS) {
    return "";
  }
  return ret.str();
}

MatchTable::MatchTable(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit,
    bool with_counters, bool with_ageing)
    : MatchTableAbstract(name, id, with_counters, with_ageing,
                         match_unit.get()),
      match_unit(std::move(match_unit)) { }

const ActionEntry &
MatchTable::lookup(const Packet &pkt, bool *hit, entry_handle_t *handle) {
  MatchUnitAbstract<ActionEntry>::MatchUnitLookup res = match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;
  return (*hit) ? (*res.value) : default_entry;
}

MatchErrorCode
MatchTable::add_entry(const std::vector<MatchKeyParam> &match_key,
                      const ActionFn *action_fn,
                      ActionData action_data,  // move it
                      entry_handle_t *handle, int priority) {
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    rc = match_unit->add_entry(
        match_key,
        ActionEntry(std::move(action_fn_entry), next_node),
        handle, priority);
  }

  // because we let go of the lock, there is a possibility of the entry being
  // removed
  // TODO(antonin): we can try to solve this by using an boost upgrade lock, but
  // I don't want to make things too complicated for logging. Maybe ideally, we
  // would print the parameters passed to the function, and not dump the entry,
  // but it is convenient to be able to print the entry in exactly the same
  // format as for a lookup

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Entry {} added to table '{}'", *handle, get_name());
    BMLOG_DEBUG(dump_entry_string(*handle));
  } else {
    BMLOG_ERROR("Error when trying to add entry to table '{}'", get_name());
  }

  return rc;
}

MatchErrorCode
MatchTable::delete_entry(entry_handle_t handle) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();
    rc = match_unit->delete_entry(handle);
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed entry {} from table '{}'", handle, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove entry {} from table '{}'",
                handle, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTable::modify_entry(entry_handle_t handle,
                         const ActionFn *action_fn, ActionData action_data) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
    const ControlFlowNode *next_node = get_next_node(action_fn->get_id());
    rc = match_unit->modify_entry(
        handle, ActionEntry(std::move(action_fn_entry), next_node));
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Modified entry {} in table '{}'", handle, get_name());
    BMLOG_DEBUG(dump_entry_string(handle));
  } else {
    BMLOG_ERROR("Error when trying to modify entry {} in table '{}'",
                handle, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTable::set_default_action(const ActionFn *action_fn,
                               ActionData action_data) {
  if (const_default_entry)
    return MatchErrorCode::DEFAULT_ENTRY_IS_CONST;
  if (const_default_action && (const_default_action != action_fn))
    return MatchErrorCode::DEFAULT_ACTION_IS_CONST;

  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node_default(action_fn->get_id());
  next_node_miss = next_node;

  {
    WriteLock lock = lock_write();
    default_entry = ActionEntry(std::move(action_fn_entry), next_node);
  }

  BMLOG_DEBUG("Set default entry for table '{}': {}",
              get_name(), default_entry);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTable::get_entry_(entry_handle_t handle, Entry *entry) const {
  const ActionEntry *action_entry;
  MatchErrorCode rc = match_unit->get_entry(handle, &entry->match_key,
                                            &action_entry, &entry->priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;

  entry->handle = handle;
  entry->action_fn = action_entry->action_fn.get_action_fn();
  entry->action_data = action_entry->action_fn.get_action_data();

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTable::get_entry(entry_handle_t handle, Entry *entry) const {
  ReadLock lock = lock_read();
  return get_entry_(handle, entry);
}

std::vector<MatchTable::Entry>
MatchTable::get_entries() const {
  ReadLock lock = lock_read();
  // maybe calling get_entry_ is not the most efficient way of building the
  // vector, but at least we avoid code duplication
  std::vector<Entry> entries(get_num_entries());
  size_t idx = 0;
  for (auto it = match_unit->handles_begin(); it != match_unit->handles_end();
       it++) {
    MatchErrorCode rc = get_entry_(*it, &entries[idx++]);
    assert(rc == MatchErrorCode::SUCCESS);
  }

  return entries;
}

MatchErrorCode
MatchTable::get_default_entry(Entry *entry) const {
  ReadLock lock = lock_read();
  entry->action_fn = default_entry.action_fn.get_action_fn();
  if (!entry->action_fn) return MatchErrorCode::NO_DEFAULT_ENTRY;
  entry->action_data = default_entry.action_fn.get_action_data();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTable::dump_entry_(std::ostream *out, entry_handle_t handle) const {
  MatchErrorCode rc;
  rc = match_unit->dump_match_entry(out, handle);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  const ActionEntry *entry;
  rc = match_unit->get_value(handle, &entry);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  *out << "Action entry: " << *entry << "\n";
  return MatchErrorCode::SUCCESS;
}

void
MatchTable::reset_state_() {
  // reset default_entry ?
  match_unit->reset_state();
}

void
MatchTable::set_const_default_action_fn(
    const ActionFn *const_default_action_fn) {
  assert(!const_default_action);
  const_default_action = const_default_action_fn;
}

void
MatchTable::set_default_entry(const ActionFn *action_fn,
                              ActionData action_data, bool is_const) {
  assert(!const_default_entry);
  auto rc = set_default_action(action_fn, std::move(action_data));
  assert(rc == MatchErrorCode::SUCCESS);
  const_default_entry = is_const;
}

void
MatchTable::serialize_(std::ostream *out) const {
  match_unit->serialize(out);
  default_entry.serialize(out);
}

void
MatchTable::deserialize_(std::istream *in, const P4Objects &objs) {
  match_unit->deserialize(in , objs);
  default_entry.deserialize(in, objs);
}


std::unique_ptr<MatchTable>
MatchTable::create(const std::string &match_type,
                   const std::string &name, p4object_id_t id,
                   size_t size, const MatchKeyBuilder &match_key_builder,
                   LookupStructureFactory *lookup_factory,
                   bool with_counters, bool with_ageing) {
  std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit =
    create_match_unit<ActionEntry>(match_type, size, match_key_builder,
                                   lookup_factory);

  return std::unique_ptr<MatchTable>(
    new MatchTable(name, id, std::move(match_unit),
                   with_counters, with_ageing));
}

MatchTableIndirect::MatchTableIndirect(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit,
    bool with_counters, bool with_ageing)
    : MatchTableAbstract(name, id, with_counters, with_ageing,
                         match_unit.get()),
      match_unit(std::move(match_unit)) { }

std::unique_ptr<MatchTableIndirect>
MatchTableIndirect::create(const std::string &match_type,
                           const std::string &name, p4object_id_t id,
                           size_t size,
                           const MatchKeyBuilder &match_key_builder,
                           LookupStructureFactory *lookup_factory,
                           bool with_counters, bool with_ageing) {
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit =
    create_match_unit<IndirectIndex>(match_type, size, match_key_builder,
                                     lookup_factory);

  return std::unique_ptr<MatchTableIndirect>(
    new MatchTableIndirect(name, id, std::move(match_unit),
                           with_counters, with_ageing));
}

const ActionEntry &
MatchTableIndirect::lookup(const Packet &pkt,
                           bool *hit, entry_handle_t *handle) {
  MatchUnitAbstract<IndirectIndex>::MatchUnitLookup res =
    match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;

  // could avoid the if statement, by reserving an empty action in
  // action_entries and making sure default_index points to it, but it seems
  // more error-prone and probably not worth the trouble
  if (!(*hit) && !default_set) return empty_action;

  const IndirectIndex &index = (*hit) ? *res.value : default_index;
  mbr_hdl_t mbr = index.get_mbr();
  assert(is_valid_mbr(mbr));

  return action_entries[mbr];
}

void
MatchTableIndirect::entries_insert(mbr_hdl_t mbr, ActionEntry &&entry) {
  assert(mbr <= action_entries.size());

  if (mbr == action_entries.size())
    action_entries.push_back(std::move(entry));
  else
    action_entries[mbr] = std::move(entry);

  index_ref_count.set(IndirectIndex::make_mbr_index(mbr), 0);
}

MatchErrorCode
MatchTableIndirect::add_member(const ActionFn *action_fn,
                               ActionData action_data,
                               mbr_hdl_t *mbr) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  {
    WriteLock lock = lock_write();

    uintptr_t mbr_;
    if (mbr_handles.get_handle(&mbr_)) {
      rc = MatchErrorCode::ERROR;
    } else {
      *mbr = static_cast<mbr_hdl_t>(mbr_);
      entries_insert(*mbr, ActionEntry(std::move(action_fn_entry), next_node));
      num_members++;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Added member {} to table '{}'", *mbr, get_name());
  } else {
    BMLOG_ERROR("Error when trying to add member to table '{}'", get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::delete_member(mbr_hdl_t mbr) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else if (index_ref_count.get(IndirectIndex::make_mbr_index(mbr)) > 0) {
      rc = MatchErrorCode::MBR_STILL_USED;
    } else {
      assert(!mbr_handles.release_handle(mbr));
      num_members--;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed member {} from table '{}'", mbr, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove member {} from table '{}'",
                mbr, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::modify_member(mbr_hdl_t mbr, const ActionFn *action_fn,
                                  ActionData action_data) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));
  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr))
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    else
      action_entries[mbr] = ActionEntry(std::move(action_fn_entry), next_node);
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Modified member {} from table '{}'", mbr, get_name());
  } else {
    BMLOG_ERROR("Error when trying to modify member {} from table '{}'",
                mbr, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::add_entry(const std::vector<MatchKeyParam> &match_key,
                              mbr_hdl_t mbr, entry_handle_t *handle,
                              int priority) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else {
      IndirectIndex index = IndirectIndex::make_mbr_index(mbr);
      index_ref_count.increase(index);
      rc = match_unit->add_entry(match_key, std::move(index), handle, priority);
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Entry {} added to table '{}'", *handle, get_name());
    BMLOG_DEBUG(dump_entry_string(*handle));
  } else {
    BMLOG_ERROR("Error when trying to add entry to table '{}'", get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::delete_entry(entry_handle_t handle) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    const IndirectIndex *index;
    rc = match_unit->get_value(handle, &index);

    if (rc == MatchErrorCode::SUCCESS) {
      index_ref_count.decrease(*index);

      rc = match_unit->delete_entry(handle);
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Entry {} removed from table '{}'", handle, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove entry {} from table '{}'",
                handle, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::modify_entry(entry_handle_t handle, mbr_hdl_t mbr) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    const IndirectIndex *index;
    rc = match_unit->get_value(handle, &index);

    if (rc == MatchErrorCode::SUCCESS)
      index_ref_count.decrease(*index);

    if (rc == MatchErrorCode::SUCCESS && !is_valid_mbr(mbr))
      rc = MatchErrorCode::INVALID_MBR_HANDLE;

    if (rc == MatchErrorCode::SUCCESS) {
      IndirectIndex new_index = IndirectIndex::make_mbr_index(mbr);
      index_ref_count.increase(new_index);

      rc = match_unit->modify_entry(handle, std::move(new_index));
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Modified entry {} in table '{}'", handle, get_name());
    BMLOG_DEBUG(dump_entry_string(handle));
  } else {
    BMLOG_ERROR("Error when trying to modify entry {} in table '{}'",
                handle, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::set_default_member(mbr_hdl_t mbr) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else {
      default_index = IndirectIndex::make_mbr_index(mbr);
      default_set = true;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Set default member for table '{}' to {}", get_name(), mbr);
  } else {
    BMLOG_ERROR("Error when trying to set default member for table '{}' to {}",
                get_name(), mbr);
  }

  return rc;
}

MatchErrorCode
MatchTableIndirect::get_entry_(entry_handle_t handle, Entry *entry) const {
  const IndirectIndex *index;
  MatchErrorCode rc = match_unit->get_entry(handle, &entry->match_key, &index,
                                            &entry->priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;

  entry->handle = handle;
  assert(index->is_mbr());
  entry->mbr = index->get_mbr();

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::get_entry(entry_handle_t handle, Entry *entry) const {
  ReadLock lock = lock_read();
  return get_entry_(handle, entry);
}

std::vector<MatchTableIndirect::Entry>
MatchTableIndirect::get_entries() const {
  ReadLock lock = lock_read();
  std::vector<Entry> entries(get_num_entries());
  size_t idx = 0;
  for (auto it = match_unit->handles_begin(); it != match_unit->handles_end();
       it++) {
    MatchErrorCode rc = get_entry_(*it, &entries[idx++]);
    assert(rc == MatchErrorCode::SUCCESS);
  }

  return entries;
}

MatchErrorCode
MatchTableIndirect::get_default_entry(Entry *entry) const {
  ReadLock lock = lock_read();
  if (!default_set) return MatchErrorCode::NO_DEFAULT_ENTRY;
  assert(default_index.is_mbr());
  entry->mbr = default_index.get_mbr();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::get_member_(mbr_hdl_t mbr, Member *member) const {
  const ActionEntry &entry = action_entries.at(mbr);
  member->mbr = mbr;
  member->action_fn = entry.action_fn.get_action_fn();
  member->action_data = entry.action_fn.get_action_data();

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::get_member(mbr_hdl_t mbr, Member *member) const {
  ReadLock lock = lock_read();

  if (!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  return get_member_(mbr, member);
}

std::vector<MatchTableIndirect::Member>
MatchTableIndirect::get_members() const {
  ReadLock lock = lock_read();
  std::vector<Member> members(get_num_members());
  size_t idx = 0;
  for (const auto h : mbr_handles) {
    MatchErrorCode rc = get_member_(h, &members[idx++]);
    assert(rc == MatchErrorCode::SUCCESS);
  }
  return members;
}

MatchErrorCode
MatchTableIndirect::dump_entry_common(std::ostream *out,
                                      entry_handle_t handle,
                                      const IndirectIndex **index) const {
  MatchErrorCode rc;
  rc = match_unit->dump_match_entry(out, handle);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  rc = match_unit->get_value(handle, index);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  const IndirectIndex *index_ptr = *index;
  *out << "Index: " << *index_ptr << "\n";
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::dump_entry_(std::ostream *out,
                                entry_handle_t handle) const {
  const IndirectIndex *index;
  MatchErrorCode rc;
  rc = dump_entry_common(out, handle, &index);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  *out << "Action entry: " << action_entries[index->get_mbr()] << "\n";
  return MatchErrorCode::SUCCESS;
}

void
MatchTableIndirect::reset_state_() {
  index_ref_count = IndirectIndexRefCount();
  mbr_handles.clear();
  action_entries.clear();
  num_members = 0;
  match_unit->reset_state();
}

void
MatchTableIndirect::serialize_(std::ostream *out) const {
  match_unit->serialize(out);
  (*out) << default_set << "\n";
  if (default_set) default_index.serialize(out);
  (*out) << action_entries.size() << "\n";
  (*out) << num_members << "\n";
  for (const auto h : mbr_handles) {
    (*out) << h << "\n";
    action_entries.at(h).serialize(out);
  }
  index_ref_count.serialize(out);
}

void
MatchTableIndirect::deserialize_(std::istream *in, const P4Objects &objs) {
  match_unit->deserialize(in , objs);
  (*in) >> default_set;
  if (default_set) default_index.deserialize(in, objs);
  size_t action_entries_size; (*in) >> action_entries_size;
  action_entries.resize(action_entries_size);
  (*in) >> num_members;
  for (size_t i = 0; i < num_members; i++) {
    mbr_hdl_t mbr_hdl; (*in) >> mbr_hdl;
    assert(!mbr_handles.set_handle(mbr_hdl));
    action_entries.at(mbr_hdl).deserialize(in, objs);
  }
  index_ref_count.deserialize(in);
}


void
MatchTableIndirect::IndirectIndexRefCount::serialize(std::ostream *out) const {
  (*out) << mbr_count.size() << "\n";
  for (const auto c : mbr_count) (*out) << c << "\n";
  (*out) << grp_count.size() << "\n";
  for (const auto c : grp_count) (*out) << c << "\n";
}

void
MatchTableIndirect::IndirectIndexRefCount::deserialize(std::istream *in) {
  size_t s;
  (*in) >> s;
  mbr_count.resize(s);
  for (auto &c : mbr_count) (*in) >> c;
  (*in) >> s;
  grp_count.resize(s);
  for (auto &c : grp_count) (*in) >> c;
}


void
MatchTableIndirect::IndirectIndex::serialize(std::ostream *out) const {
  (*out) << index << "\n";
}

void
MatchTableIndirect::IndirectIndex::deserialize(std::istream *in,
                                               const P4Objects &objs) {
  (void) objs;
  (*in) >> index;
}


MatchTableIndirectWS::MatchTableIndirectWS(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit,
    bool with_counters, bool with_ageing)
    : MatchTableIndirect(name, id, std::move(match_unit),
                         with_counters, with_ageing) { }

MatchErrorCode
MatchTableIndirectWS::GroupInfo::add_member(mbr_hdl_t mbr) {
  if (!mbrs.add(mbr)) return MatchErrorCode::MBR_ALREADY_IN_GRP;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::GroupInfo::delete_member(mbr_hdl_t mbr) {
  if (!mbrs.remove(mbr)) return MatchErrorCode::MBR_NOT_IN_GRP;
  return MatchErrorCode::SUCCESS;
}

bool
MatchTableIndirectWS::GroupInfo::contains_member(mbr_hdl_t mbr) const {
  return mbrs.contains(mbr);
}

size_t
MatchTableIndirectWS::GroupInfo::size() const {
  return mbrs.count();
}

MatchTableIndirect::mbr_hdl_t
MatchTableIndirectWS::GroupInfo::get_nth(size_t n) const {
  return mbrs.get_nth(n);
}

void
MatchTableIndirectWS::GroupInfo::serialize(std::ostream *out) const {
  (*out) << size() << "\n";
  for (const auto mbr : mbrs) (*out) << mbr << "\n";
}

void
MatchTableIndirectWS::GroupInfo::deserialize(std::istream *in) {
  size_t s; (*in) >> s;
  for (size_t i = 0; i < s; i++) {
    mbr_hdl_t mbr; (*in) >> mbr;
    add_member(mbr);
  }
}


std::unique_ptr<MatchTableIndirectWS>
MatchTableIndirectWS::create(const std::string &match_type,
                             const std::string &name, p4object_id_t id,
                             size_t size,
                             const MatchKeyBuilder &match_key_builder,
                             LookupStructureFactory *lookup_factory,
                             bool with_counters, bool with_ageing) {
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit =
    create_match_unit<IndirectIndex>(match_type, size, match_key_builder,
                                     lookup_factory);

  return std::unique_ptr<MatchTableIndirectWS>(
    new MatchTableIndirectWS(name, id, std::move(match_unit),
                             with_counters, with_ageing));
}

MatchTableIndirect::mbr_hdl_t
MatchTableIndirectWS::choose_from_group(grp_hdl_t grp,
                                        const Packet &pkt) const {
  const GroupInfo &group_info = group_entries[grp];
  size_t s = group_info.size();
  assert(s > 0);
  if (!hash) return group_info.get_nth(0);
  hash_t h = static_cast<hash_t>(hash->output(pkt));
  return group_info.get_nth(h % s);
}

const ActionEntry &
MatchTableIndirectWS::lookup(const Packet &pkt, bool *hit,
                             entry_handle_t *handle) {
  MatchUnitAbstract<IndirectIndex>::MatchUnitLookup res =
    match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;

  if (!(*hit) && !default_set) return empty_action;

  const IndirectIndex &index = (*hit) ? *res.value : default_index;

  mbr_hdl_t mbr;
  if (index.is_mbr()) {
    mbr = index.get_mbr();
  } else {
    grp_hdl_t grp = index.get_grp();
    assert(is_valid_grp(grp));
    mbr = choose_from_group(grp, pkt);
    // TODO(antonin): change to trace?
    BMLOG_DEBUG_PKT(pkt, "Choosing member {} from group {}", mbr, grp);
  }
  assert(is_valid_mbr(mbr));

  return action_entries[mbr];
}

void
MatchTableIndirectWS::groups_insert(grp_hdl_t grp) {
  assert(grp <= group_entries.size());

  if (grp == group_entries.size())
    group_entries.emplace_back();
  else
    group_entries[grp] = GroupInfo();

  index_ref_count.set(IndirectIndex::make_grp_index(grp), 0);
}

MatchErrorCode
MatchTableIndirectWS::create_group(grp_hdl_t *grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    uintptr_t grp_;
    if (grp_handles.get_handle(&grp_)) {
      rc = MatchErrorCode::ERROR;
    } else {
      *grp = static_cast<grp_hdl_t>(grp_);
      groups_insert(*grp);
      num_groups++;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Created group {} in table '{}'", *grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to create group in table '{}'", get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::delete_group(grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else if (index_ref_count.get(IndirectIndex::make_grp_index(grp)) > 0) {
      rc = MatchErrorCode::GRP_STILL_USED;
    } else {
      // we allow deletion of non-empty groups, but we must remember to decrease
      // the ref count for the members. Note that we do not allow deletion of a
      // member which is in a group
      GroupInfo &group_info = group_entries[grp];
      for (auto mbr : group_info)
        index_ref_count.decrease(IndirectIndex::make_mbr_index(mbr));

      assert(!grp_handles.release_handle(grp));

      num_groups--;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed group {} from table '{}'", grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove group {} from table '{}'",
                grp, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::add_member_to_group(mbr_hdl_t mbr, grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else {
      GroupInfo &group_info = group_entries[grp];
      rc = group_info.add_member(mbr);
      if (rc == MatchErrorCode::SUCCESS)
        index_ref_count.increase(IndirectIndex::make_mbr_index(mbr));
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Added member {} to group {} in table '{}'",
                mbr, grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to add member {} to group {} in table '{}'",
                mbr, grp, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::remove_member_from_group(mbr_hdl_t mbr, grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else {
      GroupInfo &group_info = group_entries[grp];
      rc = group_info.delete_member(mbr);
      if (rc == MatchErrorCode::SUCCESS)
        index_ref_count.decrease(IndirectIndex::make_mbr_index(mbr));
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed member {} from group {} in table '{}'",
                mbr, grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove member {} from group {} "
                "in table '{}'", mbr, grp, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::add_entry_ws(const std::vector<MatchKeyParam> &match_key,
                                   grp_hdl_t grp, entry_handle_t *handle,
                                   int priority ) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_grp(grp))
      rc = MatchErrorCode::INVALID_GRP_HANDLE;

    if (rc == MatchErrorCode::SUCCESS && get_grp_size(grp) == 0)
      rc = MatchErrorCode::EMPTY_GRP;

    if (rc == MatchErrorCode::SUCCESS) {
      IndirectIndex index = IndirectIndex::make_grp_index(grp);
      index_ref_count.increase(index);
      rc = match_unit->add_entry(match_key, std::move(index), handle, priority);
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Entry {} added to table '{}'", *handle, get_name());
    BMLOG_DEBUG(dump_entry_string(*handle));
  } else {
    BMLOG_ERROR("Error when trying to add entry to table '{}'", get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::modify_entry_ws(entry_handle_t handle, grp_hdl_t grp) {
  MatchErrorCode rc;

  {
    WriteLock lock = lock_write();

    const IndirectIndex *index;
    rc = match_unit->get_value(handle, &index);

    if (rc == MatchErrorCode::SUCCESS)
      index_ref_count.decrease(*index);

    if (rc == MatchErrorCode::SUCCESS && !is_valid_grp(grp))
      rc = MatchErrorCode::INVALID_GRP_HANDLE;

    if (rc == MatchErrorCode::SUCCESS && get_grp_size(grp) == 0)
      rc = MatchErrorCode::EMPTY_GRP;

    if (rc == MatchErrorCode::SUCCESS) {
      IndirectIndex new_index = IndirectIndex::make_grp_index(grp);
      index_ref_count.increase(new_index);

      rc = match_unit->modify_entry(handle, std::move(new_index));
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Modified entry {} in table '{}'", handle, get_name());
    BMLOG_DEBUG(dump_entry_string(handle));
  } else {
    BMLOG_ERROR("Error when trying to modify entry {} in table '{}'",
                handle, get_name());
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::set_default_group(grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else {
      default_index = IndirectIndex::make_grp_index(grp);
      default_set = true;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Set default group for table '{}' to {}", get_name(), grp);
  } else {
    BMLOG_ERROR("Error when trying to set default group for table '{}' to {}",
                get_name(), grp);
  }

  return rc;
}

MatchErrorCode
MatchTableIndirectWS::get_entry_(entry_handle_t handle, Entry *entry) const {
  const IndirectIndex *index;
  MatchErrorCode rc = match_unit->get_entry(handle, &entry->match_key, &index,
                                            &entry->priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;

  entry->handle = handle;

  if (index->is_mbr()) {
    entry->mbr = index->get_mbr();
    entry->grp = std::numeric_limits<grp_hdl_t>::max();
  } else {
    entry->mbr = std::numeric_limits<mbr_hdl_t>::max();
    entry->grp = index->get_grp();
  }

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::get_entry(entry_handle_t handle, Entry *entry) const {
  ReadLock lock = lock_read();
  return get_entry_(handle, entry);
}

std::vector<MatchTableIndirectWS::Entry>
MatchTableIndirectWS::get_entries() const {
  ReadLock lock = lock_read();
  std::vector<Entry> entries(get_num_entries());
  size_t idx = 0;
  for (auto it = match_unit->handles_begin(); it != match_unit->handles_end();
       it++) {
    MatchErrorCode rc = get_entry_(*it, &entries[idx++]);
    assert(rc == MatchErrorCode::SUCCESS);
  }

  return entries;
}

MatchErrorCode
MatchTableIndirectWS::get_default_entry(Entry *entry) const {
  ReadLock lock = lock_read();
  if (!default_set) return MatchErrorCode::NO_DEFAULT_ENTRY;
  if (default_index.is_mbr()) {
    entry->mbr = default_index.get_mbr();
    entry->grp = std::numeric_limits<grp_hdl_t>::max();
  } else {
    entry->mbr = std::numeric_limits<mbr_hdl_t>::max();
    entry->grp = default_index.get_grp();
  }
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::get_group_(grp_hdl_t grp, Group *group) const {
  group->grp = grp;
  group->mbr_handles.clear();

  for (const auto &mbr : group_entries[grp])
    group->mbr_handles.push_back(mbr);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::get_group(grp_hdl_t grp, Group *group) const {
  ReadLock lock = lock_read();
  if (!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;
  return get_group_(grp, group);
}

std::vector<MatchTableIndirectWS::Group>
MatchTableIndirectWS::get_groups() const {
  ReadLock lock = lock_read();
  std::vector<Group> groups(get_num_groups());
  size_t idx = 0;
  for (const auto h : grp_handles) {
    MatchErrorCode rc = get_group_(h, &groups[idx++]);
    assert(rc == MatchErrorCode::SUCCESS);
  }
  return groups;
}

MatchErrorCode
MatchTableIndirectWS::get_num_members_in_group(grp_hdl_t grp,
                                               size_t *nb) const {
  ReadLock lock = lock_read();

  if (!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  const GroupInfo &group_info = group_entries[grp];
  *nb = group_info.size();

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::dump_entry_(std::ostream *out,
                                  entry_handle_t handle) const {
  const IndirectIndex *index;
  MatchErrorCode rc;
  rc = dump_entry_common(out, handle, &index);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  if (index->is_mbr()) {
    *out << "Action entry: " << action_entries[index->get_mbr()] << "\n";
  } else {
    *out << "Group members:\n";
    for (const auto mbr : group_entries[index->get_grp()])
      *out << "  " << "mbr " << mbr << ": " << action_entries[mbr] << "\n";
  }
  return MatchErrorCode::SUCCESS;
}

void
MatchTableIndirectWS::reset_state_() {
  MatchTableIndirect::reset_state_();
  num_groups = 0;
  group_entries.clear();
}

void
MatchTableIndirectWS::serialize_(std::ostream *out) const {
  MatchTableIndirect::serialize_(out);
  (*out) << num_groups << "\n";
  for (const auto h : grp_handles) {
    (*out) << h << "\n";
    group_entries.at(h).serialize(out);
  }
}

void
MatchTableIndirectWS::deserialize_(std::istream *in, const P4Objects &objs) {
  MatchTableIndirect::deserialize_(in, objs);
  (*in) >> num_groups;
  group_entries.resize(num_groups);
  for (size_t i = 0; i < num_groups; i++) {
    grp_hdl_t grp_hdl; (*in) >> grp_hdl;
    assert(!grp_handles.set_handle(grp_hdl));
    group_entries.at(grp_hdl).deserialize(in);
  }
}

}  // namespace bm
