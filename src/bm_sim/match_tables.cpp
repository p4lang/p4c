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

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/match_tables.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/lookup_structures.h>
#include <bm/bm_sim/P4Objects.h>

#include <string>
#include <vector>
#include <iostream>
#include <limits>  // std::numeric_limits

namespace bm {

namespace {

template <typename V>
std::unique_ptr<MatchUnitAbstract<V> >
create_match_unit(const std::string match_type, const size_t size,
                  const MatchKeyBuilder &match_key_builder,
                  LookupStructureFactory *lookup_factory) {
  using MUExact = MatchUnitExact<V>;
  using MULPM = MatchUnitLPM<V>;
  using MUTernary = MatchUnitTernary<V>;
  using MURange = MatchUnitRange<V>;

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
  const ControlFlowNode *next_node;

  auto lock = lock_read();
  auto lock_impl = lock_impl_read();

  const ActionEntry &action_entry = lookup(*pkt, &hit, &handle, &next_node);

  // TODO(antonin): I hate this part, which requires this class to know that the
  // lower 24 bits of the handle are used as an index. Is is expected that few
  // people will ever use this index, but it is required for the implementation
  // of direct-like extern instances.
  pkt->set_entry_index(
      hit ? (handle & 0x00ffffff) : Packet::INVALID_ENTRY_INDEX);

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

  return next_node;
}

void
MatchTableAbstract::reset_state(bool reset_default_entry) {
  auto lock = lock_write();
  reset_state_(reset_default_entry);
}

void
MatchTableAbstract::set_default_default_entry(const ActionFn *action_fn,
                                              ActionData action_data,
                                              bool is_const) {
  assert(action_data.size() == action_fn->get_num_params());
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));

  const auto *next_node = get_next_node_default(action_fn->get_id());

  default_default_entry = ActionEntry(std::move(action_fn_entry), next_node);
  const_default_entry = is_const;

  set_default_default_entry_();

  BMLOG_DEBUG("Set default default entry for table '{}': {}",
              get_name(), default_default_entry);
}

void
MatchTableAbstract::serialize(std::ostream *out) const {
  auto lock = lock_read();
  (*out) << name << "\n";
  if (!next_node_miss)
    (*out) << "__NULL__" << "\n";
  else
    (*out) << next_node_miss->get_name() << "\n";
  serialize_(out);
}

void
MatchTableAbstract::deserialize(std::istream *in, const P4Objects &objs) {
  auto lock = lock_write();
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
  default_default_entry.next_node = next_node;
  set_default_default_entry_();
}

void
MatchTableAbstract::set_next_node_miss_default(
    const ControlFlowNode *next_node) {
  if (has_next_node_miss) return;
  next_node_miss = next_node;
  default_default_entry.next_node = next_node;
  set_default_default_entry_();
}

void
MatchTableAbstract::set_direct_meters(MeterArray *meter_array,
                                      header_id_t target_header,
                                      int target_offset) {
  auto lock = lock_write();
  match_unit_->set_direct_meters(meter_array);
  meter_target_header = target_header;
  meter_target_offset = target_offset;
  with_meters = true;
}

MatchErrorCode
MatchTableAbstract::query_counters(entry_handle_t handle,
                                   counter_value_t *bytes,
                                   counter_value_t *packets) const {
  auto lock = lock_read();
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
  auto lock = lock_write();
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
  auto lock = lock_write();
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
  auto lock = lock_write();
  if (!is_valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  const Meter &meter = match_unit_->get_meter(handle);
  *configs = meter.get_rates();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableAbstract::set_entry_ttl(entry_handle_t handle, unsigned int ttl_ms) {
  if (!with_ageing) return MatchErrorCode::AGEING_DISABLED;
  auto lock = lock_write();
  return match_unit_->set_entry_ttl(handle, ttl_ms);
}

void
MatchTableAbstract::sweep_entries(std::vector<entry_handle_t> *entries) const {
  auto lock = lock_read();  // TODO(antonin): how to avoid this?
  match_unit_->sweep_entries(entries);
}

MatchTableAbstract::handle_iterator
MatchTableAbstract::handles_begin() const {
  auto lock = lock_read();
  return handle_iterator(this, match_unit_->handles_begin());
}

MatchTableAbstract::handle_iterator
MatchTableAbstract::handles_end() const {
  auto lock = lock_read();
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

void
MatchTableAbstract::set_entry_common_info(EntryCommon *entry) const {
  if (!with_ageing) return;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;
  // when retrieving many entries, the clock is read many times; but this is the
  // most readable solution IMO
  auto tp = Packet::clock::now();
  auto now_ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
  MatchUnit::EntryMeta &meta = match_unit_->get_entry_meta(entry->handle);
  entry->timeout_ms = meta.timeout_ms;
  entry->time_since_hit_ms = now_ms - meta.ts.get_ms();
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
MatchTable::lookup(const Packet &pkt, bool *hit, entry_handle_t *handle,
                   const ControlFlowNode **next_node) {
  MatchUnitAbstract<ActionEntry>::MatchUnitLookup res = match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;
  const auto &entry = (*hit) ? (*res.value) : default_entry;
  *next_node = entry.next_node;
  return entry;
}

MatchErrorCode
MatchTable::add_entry(const std::vector<MatchKeyParam> &match_key,
                      const ActionFn *action_fn,
                      ActionData action_data,  // move it
                      entry_handle_t *handle, int priority) {
  if (immutable_entries) return MatchErrorCode::IMMUTABLE_TABLE_ENTRIES;

  if (action_data.size() != action_fn->get_num_params())
    return MatchErrorCode::BAD_ACTION_DATA;
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));

  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();

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
  if (immutable_entries) return MatchErrorCode::IMMUTABLE_TABLE_ENTRIES;

  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();
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
  if (immutable_entries) return MatchErrorCode::IMMUTABLE_TABLE_ENTRIES;

  if (action_data.size() != action_fn->get_num_params())
    return MatchErrorCode::BAD_ACTION_DATA;
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));

  const ControlFlowNode *next_node = get_next_node(action_fn->get_id());

  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();
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

  if (action_data.size() != action_fn->get_num_params())
    return MatchErrorCode::BAD_ACTION_DATA;
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));

  const ControlFlowNode *next_node = get_next_node_default(action_fn->get_id());

  {
    auto lock = lock_write();
    default_entry = ActionEntry(std::move(action_fn_entry), next_node);
  }

  BMLOG_DEBUG("Set default entry for table '{}': {}",
              get_name(), default_entry);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTable::reset_default_entry() {
  {
    auto lock = lock_write();
    default_entry = default_default_entry;
  }

  BMLOG_DEBUG("Reset default entry for table '{}' to: {}",
              get_name(), default_default_entry);

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

  set_entry_common_info(entry);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTable::get_entry(entry_handle_t handle, Entry *entry) const {
  auto lock = lock_read();
  return get_entry_(handle, entry);
}

// You will notice some duplicated code between MatchTable, MatchTableIndirect
// and MatchTableIndirectWS for the implementation of get_entry_from_key and
// get_entries. This is regrettable. Ideally I would use CRTP to have something
// like this:
// template <typename T>
// class MatchTableCommon {
//  public:
//   MatchErrorCode get_entry_from_key(
//       const std::vector<MatchKeyParam> &match_key,
//       typename T::Entry *entry, int priotiy) const {
//     // ...
//   }
// };
// However this is not possible, because Entry is a nested class for T
// There are a few solutions like the following:
// template <typename T>
// class MatchTableCommon {
//  public:
//   template <U = T>
//   MatchErrorCode get_entry_from_key(
//       const std::vector<MatchKeyParam> &match_key,
//       typename U::Entry *entry, int priotiy) const {
//     // ...
//   }
// };
// Does that make the code unreadable? If not I may choose to use this solution.

MatchErrorCode
MatchTable::get_entry_from_key(const std::vector<MatchKeyParam> &match_key,
                               Entry *entry, int priority) const {
  auto lock = lock_read();
  entry_handle_t handle;
  const auto rc = match_unit->retrieve_handle(match_key, &handle, priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  return get_entry_(handle, entry);
}

std::vector<MatchTable::Entry>
MatchTable::get_entries() const {
  auto lock = lock_read();
  // maybe calling get_entry_ is not the most efficient way of building the
  // vector, but at least we avoid code duplication
  std::vector<Entry> entries(get_num_entries());
  size_t idx = 0;
  for (auto it = match_unit->handles_begin(); it != match_unit->handles_end();
       it++) {
    MatchErrorCode rc = get_entry_(*it, &entries[idx++]);
    _BM_UNUSED(rc);
    assert(rc == MatchErrorCode::SUCCESS);
  }

  return entries;
}

MatchErrorCode
MatchTable::get_default_entry(Entry *entry) const {
  auto lock = lock_read();
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
MatchTable::reset_state_(bool reset_default_entry) {
  if (reset_default_entry)
    default_entry = default_default_entry;
  match_unit->reset_state();
}

void
MatchTable::set_default_default_entry_() {
  default_entry = default_default_entry;
}

void
MatchTable::set_const_default_action_fn(
    const ActionFn *const_default_action_fn) {
  assert(!const_default_action);
  const_default_action = const_default_action_fn;
}

void
MatchTable::set_immutable_entries() {
  immutable_entries = true;
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

void
MatchTableIndirect::set_action_profile(ActionProfile *action_profile) {
  this->action_profile = action_profile;
}

ActionProfile *
MatchTableIndirect::get_action_profile() const {
  return action_profile;
}

const ActionEntry &
MatchTableIndirect::lookup(const Packet &pkt,
                           bool *hit, entry_handle_t *handle,
                           const ControlFlowNode **next_node) {
  MatchUnitAbstract<IndirectIndex>::MatchUnitLookup res =
    match_unit->lookup(pkt);
  *hit = res.found();
  *handle = res.handle;

  // default_default_entry will be an empty (no-op) action unless otherwise
  // specified in the P4 / JSON
  if (!(*hit) && !default_set) {
    *next_node = default_default_entry.next_node;
    return default_default_entry;
  }

  const IndirectIndex &index = (*hit) ? *res.value : default_index;
  if (index.is_grp() && action_profile->group_is_empty(index.get_grp())) {
    BMLOG_ERROR_PKT(
        pkt, "Lookup in table '{}' yielded empty group", get_name());
    return empty_action;
  }

  const auto &entry = action_profile->lookup(pkt, index);
  // Unfortunately this has to be done at this stage and cannot be done when
  // inserting a member because for 2 match tables sharing the same action
  // profile (and therefore the same members), the next node mapping can vary
  *next_node = (*hit) ?
      get_next_node(entry.action_fn.get_action_id()) :
      get_next_node_default(entry.action_fn.get_action_id());
  return entry;
}

MatchErrorCode
MatchTableIndirect::add_entry(const std::vector<MatchKeyParam> &match_key,
                              mbr_hdl_t mbr, entry_handle_t *handle,
                              int priority) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    if (!action_profile->is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else {
      IndirectIndex index = IndirectIndex::make_mbr_index(mbr);
      action_profile->ref_count_increase(index);
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
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    const IndirectIndex *index;
    rc = match_unit->get_value(handle, &index);

    if (rc == MatchErrorCode::SUCCESS) {
      action_profile->ref_count_decrease(*index);

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
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    const IndirectIndex *index;
    rc = match_unit->get_value(handle, &index);

    if (rc == MatchErrorCode::SUCCESS)
      action_profile->ref_count_decrease(*index);

    if (rc == MatchErrorCode::SUCCESS && !action_profile->is_valid_mbr(mbr))
      rc = MatchErrorCode::INVALID_MBR_HANDLE;

    if (rc == MatchErrorCode::SUCCESS) {
      IndirectIndex new_index = IndirectIndex::make_mbr_index(mbr);
      action_profile->ref_count_increase(new_index);

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
  if (const_default_entry)
    return MatchErrorCode::DEFAULT_ENTRY_IS_CONST;

  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    if (!action_profile->is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else {
      if (default_set) action_profile->ref_count_decrease(default_index);
      default_index = IndirectIndex::make_mbr_index(mbr);
      default_set = true;
      action_profile->ref_count_increase(default_index);
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
MatchTableIndirect::reset_default_entry() {
  {
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();
    if (default_set) action_profile->ref_count_decrease(default_index);
    default_set = false;
  }

  BMLOG_DEBUG("Reset default entry for table '{}' to: {}",
              get_name(), default_default_entry);

  return MatchErrorCode::SUCCESS;
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

  set_entry_common_info(entry);

  return MatchErrorCode::SUCCESS;
}

MatchTableIndirect::ReadLock
MatchTableIndirect::lock_impl_read() const {
  return action_profile->lock_read();
}

MatchTableIndirect::WriteLock
MatchTableIndirect::lock_impl_write() const {
  return action_profile->lock_write();
}

MatchErrorCode
MatchTableIndirect::get_entry(entry_handle_t handle, Entry *entry) const {
  auto lock = lock_read();
  return get_entry_(handle, entry);
}

MatchErrorCode
MatchTableIndirect::get_entry_from_key(
    const std::vector<MatchKeyParam> &match_key,
    Entry *entry, int priority) const {
  auto lock = lock_read();
  entry_handle_t handle;
  const auto rc = match_unit->retrieve_handle(match_key, &handle, priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  return get_entry_(handle, entry);
}

std::vector<MatchTableIndirect::Entry>
MatchTableIndirect::get_entries() const {
  auto lock = lock_read();
  std::vector<Entry> entries(get_num_entries());
  size_t idx = 0;
  for (auto it = match_unit->handles_begin(); it != match_unit->handles_end();
       it++) {
    MatchErrorCode rc = get_entry_(*it, &entries[idx++]);
    _BM_UNUSED(rc);
    assert(rc == MatchErrorCode::SUCCESS);
  }

  return entries;
}

MatchErrorCode
MatchTableIndirect::get_default_entry(Entry *entry) const {
  auto lock = lock_read();
  if (!default_set) return MatchErrorCode::NO_DEFAULT_ENTRY;
  assert(default_index.is_mbr());
  entry->mbr = default_index.get_mbr();
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirect::dump_entry_(std::ostream *out,
                                entry_handle_t handle) const {
  const IndirectIndex *index;
  MatchErrorCode rc;
  rc = match_unit->dump_match_entry(out, handle);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  rc = match_unit->get_value(handle, &index);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  *out << "Index: " << *index << "\n";
  action_profile->dump_entry(out, *index);
  return MatchErrorCode::SUCCESS;
}

void
MatchTableIndirect::reset_state_(bool reset_default_entry) {
  if (reset_default_entry)
    default_set = false;
  match_unit->reset_state();
}

void
MatchTableIndirect::set_default_default_entry_() { }

void
MatchTableIndirect::serialize_(std::ostream *out) const {
  match_unit->serialize(out);
  (*out) << default_set << "\n";
  if (default_set) default_index.serialize(out);
}

void
MatchTableIndirect::deserialize_(std::istream *in, const P4Objects &objs) {
  match_unit->deserialize(in , objs);
  (*in) >> default_set;
  if (default_set) default_index.deserialize(in, objs);
}


MatchTableIndirectWS::MatchTableIndirectWS(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit,
    bool with_counters, bool with_ageing)
    : MatchTableIndirect(name, id, std::move(match_unit),
                         with_counters, with_ageing) { }

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

const ActionEntry &
MatchTableIndirectWS::lookup(const Packet &pkt, bool *hit,
                             entry_handle_t *handle,
                             const ControlFlowNode **next_node) {
  return MatchTableIndirect::lookup(pkt, hit, handle, next_node);
}

MatchErrorCode
MatchTableIndirectWS::add_entry_ws(const std::vector<MatchKeyParam> &match_key,
                                   grp_hdl_t grp, entry_handle_t *handle,
                                   int priority ) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    if (!action_profile->is_valid_grp(grp))
      rc = MatchErrorCode::INVALID_GRP_HANDLE;

    if (rc == MatchErrorCode::SUCCESS && action_profile->group_is_empty(grp))
      rc = MatchErrorCode::EMPTY_GRP;

    if (rc == MatchErrorCode::SUCCESS) {
      IndirectIndex index = IndirectIndex::make_grp_index(grp);
      action_profile->ref_count_increase(index);
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
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    const IndirectIndex *index;
    rc = match_unit->get_value(handle, &index);

    if (rc == MatchErrorCode::SUCCESS)
      action_profile->ref_count_decrease(*index);

    if (rc == MatchErrorCode::SUCCESS && !action_profile->is_valid_grp(grp))
      rc = MatchErrorCode::INVALID_GRP_HANDLE;

    if (rc == MatchErrorCode::SUCCESS && action_profile->group_is_empty(grp))
      rc = MatchErrorCode::EMPTY_GRP;

    if (rc == MatchErrorCode::SUCCESS) {
      IndirectIndex new_index = IndirectIndex::make_grp_index(grp);
      action_profile->ref_count_increase(new_index);

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
  if (const_default_entry)
    return MatchErrorCode::DEFAULT_ENTRY_IS_CONST;

  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();
    auto lock_impl = lock_impl_write();

    if (!action_profile->is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else {
      if (default_set) action_profile->ref_count_decrease(default_index);
      default_index = IndirectIndex::make_grp_index(grp);
      default_set = true;
      action_profile->ref_count_increase(default_index);
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

  set_entry_common_info(entry);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchTableIndirectWS::get_entry(entry_handle_t handle, Entry *entry) const {
  auto lock = lock_read();
  return get_entry_(handle, entry);
}

MatchErrorCode
MatchTableIndirectWS::get_entry_from_key(
    const std::vector<MatchKeyParam> &match_key,
    Entry *entry, int priority) const {
  auto lock = lock_read();
  entry_handle_t handle;
  const auto rc = match_unit->retrieve_handle(match_key, &handle, priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  return get_entry_(handle, entry);
}

std::vector<MatchTableIndirectWS::Entry>
MatchTableIndirectWS::get_entries() const {
  auto lock = lock_read();
  std::vector<Entry> entries(get_num_entries());
  size_t idx = 0;
  for (auto it = match_unit->handles_begin(); it != match_unit->handles_end();
       it++) {
    MatchErrorCode rc = get_entry_(*it, &entries[idx++]);
    _BM_UNUSED(rc);
    assert(rc == MatchErrorCode::SUCCESS);
  }

  return entries;
}

MatchErrorCode
MatchTableIndirectWS::get_default_entry(Entry *entry) const {
  auto lock = lock_read();
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
MatchTableIndirectWS::dump_entry_(std::ostream *out,
                                  entry_handle_t handle) const {
  return MatchTableIndirect::dump_entry_(out, handle);
}

void
MatchTableIndirectWS::serialize_(std::ostream *out) const {
  MatchTableIndirect::serialize_(out);
}

void
MatchTableIndirectWS::deserialize_(std::istream *in, const P4Objects &objs) {
  MatchTableIndirect::deserialize_(in, objs);
}

}  // namespace bm
