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

#include <bm/bm_sim/context.h>

#include <string>
#include <vector>
#include <set>

namespace bm {

Context::Context() {
  p4objects = std::make_shared<P4Objects>();
  p4objects_rt = p4objects;
}

// ---------- runtime interfaces ----------

MatchErrorCode
Context::mt_add_entry(const std::string &table_name,
                      const std::vector<MatchKeyParam> &match_key,
                      const std::string &action_name,
                      ActionData action_data,
                      entry_handle_t *handle,
                      int priority) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if (!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  const ActionFn *action = p4objects_rt->get_action(table_name, action_name);
  assert(action);
  return table->add_entry(
    match_key, action, std::move(action_data), handle, priority);
}

MatchErrorCode
Context::mt_set_default_action(const std::string &table_name,
                               const std::string &action_name,
                               ActionData action_data) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if (!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  const ActionFn *action = p4objects_rt->get_action(table_name, action_name);
  assert(action);
  return table->set_default_action(action, std::move(action_data));
}

MatchErrorCode
Context::mt_delete_entry(const std::string &table_name,
                         entry_handle_t handle) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if (!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  return table->delete_entry(handle);
}

MatchErrorCode
Context::mt_modify_entry(const std::string &table_name,
                         entry_handle_t handle,
                         const std::string &action_name,
                         const ActionData action_data) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if (!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  const ActionFn *action = p4objects_rt->get_action(table_name, action_name);
  assert(action);
  return table->modify_entry(handle, action, std::move(action_data));
}

MatchErrorCode
Context::mt_set_entry_ttl(const std::string &table_name,
                          entry_handle_t handle,
                          unsigned int ttl_ms) {
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->set_entry_ttl(handle, ttl_ms);
}

MatchErrorCode
Context::get_mt_indirect(const std::string &table_name,
                         MatchTableIndirect **table) const {
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  *table = dynamic_cast<MatchTableIndirect *>(abstract_table);
  if (!(*table)) return MatchErrorCode::WRONG_TABLE_TYPE;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
Context::mt_indirect_add_member(
    const std::string &table_name, const std::string &action_name,
    ActionData action_data, mbr_hdl_t *mbr) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  const ActionFn *action = p4objects_rt->get_action(table_name, action_name);
  if (!action) return MatchErrorCode::INVALID_ACTION_NAME;
  return table->add_member(action, std::move(action_data), mbr);
}

MatchErrorCode
Context::mt_indirect_delete_member(const std::string &table_name,
                                   mbr_hdl_t mbr) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->delete_member(mbr);
}

MatchErrorCode
Context::mt_indirect_modify_member(const std::string &table_name,
                                   mbr_hdl_t mbr,
                                   const std::string &action_name,
                                   ActionData action_data) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  const ActionFn *action = p4objects_rt->get_action(table_name, action_name);
  if (!action) return MatchErrorCode::INVALID_ACTION_NAME;
  return table->modify_member(mbr, action, std::move(action_data));
}

MatchErrorCode
Context::mt_indirect_add_entry(
    const std::string &table_name,
    const std::vector<MatchKeyParam> &match_key,
    mbr_hdl_t mbr, entry_handle_t *handle, int priority) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->add_entry(match_key, mbr, handle, priority);
}

MatchErrorCode
Context::mt_indirect_modify_entry(const std::string &table_name,
                                  entry_handle_t handle,
                                  mbr_hdl_t mbr) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->modify_entry(handle, mbr);
}

MatchErrorCode
Context::mt_indirect_delete_entry(const std::string &table_name,
                                  entry_handle_t handle) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->delete_entry(handle);
}

MatchErrorCode
Context::mt_indirect_set_entry_ttl(const std::string &table_name,
                                   entry_handle_t handle,
                                   unsigned int ttl_ms) {
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->set_entry_ttl(handle, ttl_ms);
}

MatchErrorCode
Context::mt_indirect_set_default_member(const std::string &table_name,
                                        mbr_hdl_t mbr) {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->set_default_member(mbr);
}

MatchErrorCode
Context::get_mt_indirect_ws(const std::string &table_name,
                            MatchTableIndirectWS **table) const {
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  *table = dynamic_cast<MatchTableIndirectWS *>(abstract_table);
  if (!(*table)) return MatchErrorCode::WRONG_TABLE_TYPE;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
Context::mt_indirect_ws_create_group(const std::string &table_name,
                                     grp_hdl_t *grp) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->create_group(grp);
}

MatchErrorCode
Context::mt_indirect_ws_delete_group(const std::string &table_name,
                                     grp_hdl_t grp) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->delete_group(grp);
}

MatchErrorCode
Context::mt_indirect_ws_add_member_to_group(
    const std::string &table_name, mbr_hdl_t mbr, grp_hdl_t grp) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->add_member_to_group(mbr, grp);
}

MatchErrorCode
Context::mt_indirect_ws_remove_member_from_group(
    const std::string &table_name,
    mbr_hdl_t mbr, grp_hdl_t grp) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->remove_member_from_group(mbr, grp);
}

MatchErrorCode
Context::mt_indirect_ws_add_entry(
    const std::string &table_name,
    const std::vector<MatchKeyParam> &match_key,
    grp_hdl_t grp, entry_handle_t *handle, int priority) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->add_entry_ws(match_key, grp, handle, priority);
}

MatchErrorCode
Context::mt_indirect_ws_modify_entry(const std::string &table_name,
                                     entry_handle_t handle,
                                     grp_hdl_t grp) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->modify_entry_ws(handle, grp);
}

MatchErrorCode
Context::mt_indirect_ws_set_default_group(const std::string &table_name,
                                          grp_hdl_t grp) {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->set_default_group(grp);
}

MatchErrorCode
Context::mt_read_counters(const std::string &table_name,
                          entry_handle_t handle,
                          MatchTableAbstract::counter_value_t *bytes,
                          MatchTableAbstract::counter_value_t *packets) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->query_counters(handle, bytes, packets);
}

MatchErrorCode
Context::mt_reset_counters(const std::string &table_name) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->reset_counters();
}

MatchErrorCode
Context::mt_write_counters(const std::string &table_name,
                           entry_handle_t handle,
                           MatchTableAbstract::counter_value_t bytes,
                           MatchTableAbstract::counter_value_t packets) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->write_counters(handle, bytes, packets);
}

MatchErrorCode
Context::mt_set_meter_rates(const std::string &table_name,
                            entry_handle_t handle,
                            const std::vector<Meter::rate_config_t> &configs) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->set_meter_rates(handle, configs);
}

MatchErrorCode
Context::mt_get_meter_rates(const std::string &table_name,
                            entry_handle_t handle,
                            std::vector<Meter::rate_config_t> *configs) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
    p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->get_meter_rates(handle, configs);
}

MatchTableType
Context::mt_get_type(const std::string &table_name) const {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
      p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchTableType::NONE;
  return abstract_table->get_table_type();
}

template <typename T>
std::vector<typename T::Entry>
Context::mt_get_entries(const std::string &table_name) const {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
      p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return {};
  T *table = dynamic_cast<T *>(abstract_table);
  // TODO(antonin): return an error code instead?
  if (!table) return {};
  return table->get_entries();
}

// explicit instantiation
template std::vector<MatchTable::Entry>
Context::mt_get_entries<MatchTable>(const std::string &) const;
template std::vector<MatchTableIndirect::Entry>
Context::mt_get_entries<MatchTableIndirect>(const std::string &) const;
template std::vector<MatchTableIndirectWS::Entry>
Context::mt_get_entries<MatchTableIndirectWS>(const std::string &) const;

template <typename T>
MatchErrorCode
Context::mt_get_entry(const std::string &table_name,
                      entry_handle_t handle, typename T::Entry *entry) const {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
      p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  T *table = dynamic_cast<T *>(abstract_table);
  if (!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  return table->get_entry(handle, entry);
}

// explicit instantiation
template MatchErrorCode
Context::mt_get_entry<MatchTable>(
    const std::string &, entry_handle_t, MatchTable::Entry *) const;
template MatchErrorCode
Context::mt_get_entry<MatchTableIndirect>(
    const std::string &, entry_handle_t, MatchTableIndirect::Entry *) const;
template MatchErrorCode
Context::mt_get_entry<MatchTableIndirectWS>(
    const std::string &, entry_handle_t, MatchTableIndirectWS::Entry *) const;

template <typename T>
MatchErrorCode
Context::mt_get_default_entry(const std::string &table_name,
                              typename T::Entry *default_entry) const {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table =
      p4objects_rt->get_abstract_match_table(table_name);
  if (!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  T *table = dynamic_cast<T *>(abstract_table);
  if (!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  return table->get_default_entry(default_entry);
}

// explicit instantiation
template MatchErrorCode
Context::mt_get_default_entry<MatchTable>(
    const std::string &, MatchTable::Entry *) const;
template MatchErrorCode
Context::mt_get_default_entry<MatchTableIndirect>(
    const std::string &, MatchTableIndirect::Entry *) const;
template MatchErrorCode
Context::mt_get_default_entry<MatchTableIndirectWS>(
    const std::string &, MatchTableIndirectWS::Entry *) const;

std::vector<MatchTableIndirect::Member>
Context::mt_indirect_get_members(const std::string &table_name) const {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return {};
  return table->get_members();
}

MatchErrorCode
Context::mt_indirect_get_member(const std::string &table_name, grp_hdl_t grp,
                                MatchTableIndirect::Member *member) const {
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->get_member(grp, member);
}

std::vector<MatchTableIndirectWS::Group>
Context::mt_indirect_ws_get_groups(const std::string &table_name) const {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return {};
  return table->get_groups();
}

MatchErrorCode
Context::mt_indirect_ws_get_group(const std::string &table_name, grp_hdl_t grp,
                                  MatchTableIndirectWS::Group *group) const {
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if ((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->get_group(grp, group);
}

Counter::CounterErrorCode
Context::read_counters(const std::string &counter_name, size_t idx,
                       MatchTableAbstract::counter_value_t *bytes,
                       MatchTableAbstract::counter_value_t *packets) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  CounterArray *counter_array = p4objects_rt->get_counter_array_rt(
      counter_name);
  if (!counter_array) return Counter::INVALID_COUNTER_NAME;
  if (idx >= counter_array->size()) return Counter::INVALID_INDEX;
  return (*counter_array)[idx].query_counter(bytes, packets);
}

Counter::CounterErrorCode
Context::reset_counters(const std::string &counter_name) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  CounterArray *counter_array = p4objects_rt->get_counter_array_rt(
      counter_name);
  if (!counter_array) return Counter::INVALID_COUNTER_NAME;
  return counter_array->reset_counters();
}

Counter::CounterErrorCode
Context::write_counters(const std::string &counter_name, size_t idx,
                        MatchTableAbstract::counter_value_t bytes,
                        MatchTableAbstract::counter_value_t packets) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  CounterArray *counter_array = p4objects_rt->get_counter_array_rt(
      counter_name);
  if (!counter_array) return Counter::INVALID_COUNTER_NAME;
  if (idx >= counter_array->size()) return Counter::INVALID_INDEX;
  return (*counter_array)[idx].write_counter(bytes, packets);
}

Context::MeterErrorCode
Context::meter_array_set_rates(
    const std::string &meter_name,
    const std::vector<Meter::rate_config_t> &configs) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MeterArray *meter_array = p4objects_rt->get_meter_array_rt(meter_name);
  if (!meter_array) return Meter::INVALID_METER_NAME;
  return meter_array->set_rates(configs);
}

Context::MeterErrorCode
Context::meter_set_rates(
    const std::string &meter_name, size_t idx,
    const std::vector<Meter::rate_config_t> &configs) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MeterArray *meter_array = p4objects_rt->get_meter_array_rt(meter_name);
  if (!meter_array) return Meter::INVALID_METER_NAME;
  if (idx >= meter_array->size()) return Meter::INVALID_INDEX;
  return meter_array->get_meter(idx).set_rates(configs);
}

Context::MeterErrorCode
Context::meter_get_rates(
    const std::string &meter_name, size_t idx,
    std::vector<Meter::rate_config_t> *configs) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MeterArray *meter_array = p4objects_rt->get_meter_array_rt(meter_name);
  if (!meter_array) return Meter::INVALID_METER_NAME;
  if (idx >= meter_array->size()) return Meter::INVALID_INDEX;
  *configs = meter_array->get_meter(idx).get_rates();
  return Meter::SUCCESS;
}

Context::RegisterErrorCode
Context::register_read(const std::string &register_name,
                       const size_t idx, Data *value) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  RegisterArray *register_array = p4objects_rt->get_register_array_rt(
      register_name);
  if (!register_array) return Register::INVALID_REGISTER_NAME;
  if (idx >= register_array->size()) return Register::INVALID_INDEX;
  auto register_lock = register_array->unique_lock();
  value->set((*register_array)[idx]);
  return Register::SUCCESS;
}

Context::RegisterErrorCode
Context::register_write(const std::string &register_name,
                        const size_t idx, Data value) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  RegisterArray *register_array = p4objects_rt->get_register_array_rt(
      register_name);
  if (!register_array) return Register::INVALID_REGISTER_NAME;
  if (idx >= register_array->size()) return Register::INVALID_INDEX;
  auto register_lock = register_array->unique_lock();
  (*register_array)[idx].set(std::move(value));
  return Register::SUCCESS;
}

Context::RegisterErrorCode
Context::register_write_range(const std::string &register_name,
                              const size_t start, const size_t end,
                              Data value) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  RegisterArray *register_array = p4objects_rt->get_register_array_rt(
      register_name);
  if (!register_array) return Register::INVALID_REGISTER_NAME;
  if (end > register_array->size() || start > end)
    return Register::INVALID_INDEX;
  auto register_lock = register_array->unique_lock();
  for (size_t idx = start; idx < end; idx++)
    register_array->at(idx).set(value);
  return Register::SUCCESS;
}

Context::RegisterErrorCode
Context::register_reset(const std::string &register_name) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  RegisterArray *register_array = p4objects_rt->get_register_array_rt(
      register_name);
  if (!register_array) return Register::INVALID_REGISTER_NAME;
  register_array->reset_state();
  return Register::SUCCESS;
}

ParseVSet::ErrorCode
Context::parse_vset_add(const std::string &parse_vset_name,
                        const ByteContainer &value) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  ParseVSet *parse_vset = p4objects_rt->get_parse_vset_rt(parse_vset_name);
  if (!parse_vset) return ParseVSet::ErrorCode::INVALID_PARSE_VSET_NAME;
  parse_vset->add(value);
  return ParseVSet::ErrorCode::SUCCESS;
}

ParseVSet::ErrorCode
Context::parse_vset_remove(const std::string &parse_vset_name,
                           const ByteContainer &value) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  ParseVSet *parse_vset = p4objects_rt->get_parse_vset_rt(parse_vset_name);
  if (!parse_vset) return ParseVSet::ErrorCode::INVALID_PARSE_VSET_NAME;
  parse_vset->remove(value);
  return ParseVSet::ErrorCode::SUCCESS;
}

template <typename T>
CustomCrcErrorCode
Context::set_crc_custom_parameters(
    const std::string &calc_name,
    const typename CustomCrcMgr<T>::crc_config_t &crc_config) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  auto *named_calculation = p4objects_rt->get_named_calculation(calc_name);
  if (!named_calculation) return CustomCrcErrorCode::INVALID_CALCULATION_NAME;
  return CustomCrcMgr<T>::update_config(named_calculation, crc_config);
}

template CustomCrcErrorCode Context::set_crc_custom_parameters<uint16_t>(
    const std::string &calc_name,
    const CustomCrcMgr<uint16_t>::crc_config_t &crc_config);
template CustomCrcErrorCode Context::set_crc_custom_parameters<uint32_t>(
    const std::string &calc_name,
    const CustomCrcMgr<uint32_t>::crc_config_t &crc_config);

// ---------- End runtime interfaces ----------

LearnEngine *
Context::get_learn_engine() {
  return p4objects->get_learn_engine();
}

AgeingMonitor *
Context::get_ageing_monitor() {
  return p4objects->get_ageing_monitor();
}

PHVFactory &
Context::get_phv_factory() {
  return p4objects->get_phv_factory();
}

void
Context::set_notifications_transport(
    std::shared_ptr<TransportIface> transport) {
  notifications_transport = transport;
}

void
Context::set_device_id(int dev_id) {
  device_id = dev_id;
}

void
Context::set_cxt_id(int id) {
  cxt_id = id;
}

void
Context::set_force_arith(bool v) {
  force_arith = v;
}

int
Context::init_objects(std::istream *is,
                      LookupStructureFactory *lookup_factory,
                      const std::set<header_field_pair> &required_fields,
                      const ForceArith &arith_objects) {
  // initally p4objects_rt == p4objects, so this works
  int status = p4objects_rt->init_objects(is, lookup_factory, device_id, cxt_id,
                                          notifications_transport,
                                          required_fields, arith_objects);
  if (status) return status;
  if (force_arith)
    get_phv_factory().enable_all_arith();
  return 0;
}

Context::ErrorCode
Context::load_new_config(
    std::istream *is,
    LookupStructureFactory *lookup_factory,
    const std::set<header_field_pair> &required_fields,
    const ForceArith &arith_objects) {
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // check that there is no ongoing config swap
  if (p4objects != p4objects_rt) return ErrorCode::ONGOING_SWAP;
  p4objects_rt = std::make_shared<P4Objects>();
  init_objects(is, lookup_factory, required_fields, arith_objects);
  return ErrorCode::SUCCESS;
}

Context::ErrorCode
Context::swap_configs() {
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // no ongoing swap
  if (p4objects == p4objects_rt) return ErrorCode::NO_ONGOING_SWAP;
  swap_ordered = true;
  return ErrorCode::SUCCESS;
}

Context::ErrorCode
Context::reset_state() {
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  p4objects_rt->reset_state();
  return ErrorCode::SUCCESS;
}

Context::ErrorCode
Context::serialize(std::ostream *out) {
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  p4objects_rt->serialize(out);
  return ErrorCode::SUCCESS;
}

// we assume this is called when the switch is started, just after loading the
// JSON, so no traffic yet
Context::ErrorCode
Context::deserialize(std::istream *in) {
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // not really necessary for now (if done before traffic and before RPC server
  // running)
  // p4objects_rt->reset_state();
  p4objects_rt->deserialize(in);
  return ErrorCode::SUCCESS;
}

int
Context::do_swap() {
  if (!swap_ordered) return 1;
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  p4objects = p4objects_rt;
  swap_ordered = false;
  return 0;
}

}  // namespace bm
