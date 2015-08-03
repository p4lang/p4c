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

#include <cassert>

#include "bm_sim/switch.h"
#include "bm_sim/P4Objects.h"
#include "bm_sim/options_parse.h"

Switch::Switch(bool enable_swap)
  : DevMgr(),
    enable_swap(enable_swap) {
  p4objects = std::make_shared<P4Objects>();
  p4objects_rt = p4objects;
}

void
Switch::init_objects(const std::string &json_path) {
  std::fstream fs(json_path);
  p4objects->init_objects(fs);
  Packet::set_phv_factory(p4objects->get_phv_factory());

  // TODO: is this the right place to do this?
  set_packet_handler(&Switch::packet_handler, (void *) this);
}

void
Switch::init_from_command_line_options(int argc, char *argv[]) {
  OptionsParser parser;
  parser.parse(argc, argv);
  init_objects(parser.config_file_path);
  int port_num = 0;
  for(const auto &iface : parser.ifaces) {
    std::cout << "Adding interface " << iface
	      << " as port " << port_num << std::endl;
    if(parser.pcap) {
      const std::string pcap = iface + ".pcap";
      port_add(iface, port_num++, pcap.c_str());
    }
    else {
      port_add(iface, port_num++, NULL);
    }
  }
  thrift_port = parser.thrift_port;
}

MatchErrorCode
Switch::mt_add_entry(
    const std::string &table_name,
    const std::vector<MatchKeyParam> &match_key,
    const std::string &action_name,
    ActionData action_data,
    entry_handle_t *handle,
    int priority
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if(!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  const ActionFn *action = p4objects_rt->get_action(action_name);
  assert(action);
  return table->add_entry(
    match_key, action, std::move(action_data), handle, priority
  );
}

MatchErrorCode
Switch::mt_set_default_action(
    const std::string &table_name,
    const std::string &action_name,
    ActionData action_data
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if(!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  const ActionFn *action = p4objects_rt->get_action(action_name);
  assert(action);
  return table->set_default_action(action, std::move(action_data));
}

MatchErrorCode
Switch::mt_delete_entry(
    const std::string &table_name,
    entry_handle_t handle
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if(!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  return table->delete_entry(handle);
}

MatchErrorCode
Switch::mt_modify_entry(
    const std::string &table_name,
    entry_handle_t handle,
    const std::string &action_name,
    const ActionData action_data
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  MatchTable *table = dynamic_cast<MatchTable *>(abstract_table);
  if(!table) return MatchErrorCode::WRONG_TABLE_TYPE;
  const ActionFn *action = p4objects_rt->get_action(action_name);
  assert(action);
  return table->modify_entry(handle, action, std::move(action_data));
}

MatchErrorCode
Switch::mt_set_entry_ttl(
    const std::string &table_name,
    entry_handle_t handle,
    unsigned int ttl_ms
)
{
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  if(!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->set_entry_ttl(handle, ttl_ms);
}

MatchErrorCode
Switch::get_mt_indirect(
  const std::string &table_name, MatchTableIndirect **table
)
{
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  if(!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  *table = dynamic_cast<MatchTableIndirect *>(abstract_table);
  if(!(*table)) return MatchErrorCode::WRONG_TABLE_TYPE;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
Switch::mt_indirect_add_member(
  const std::string &table_name, const std::string &action_name,
  ActionData action_data, mbr_hdl_t *mbr
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  const ActionFn *action = p4objects_rt->get_action(action_name);
  if(!action) return MatchErrorCode::INVALID_ACTION_NAME;
  return table->add_member(action, std::move(action_data), mbr);
}

MatchErrorCode
Switch::mt_indirect_delete_member(
  const std::string &table_name, mbr_hdl_t mbr
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->delete_member(mbr);
}

MatchErrorCode
Switch::mt_indirect_modify_member(
  const std::string &table_name, mbr_hdl_t mbr,
  const std::string &action_name, ActionData action_data
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  const ActionFn *action = p4objects_rt->get_action(action_name);
  if(!action) return MatchErrorCode::INVALID_ACTION_NAME;
  return table->modify_member(mbr, action, std::move(action_data));
}
 
MatchErrorCode
Switch::mt_indirect_add_entry(
  const std::string &table_name,
  const std::vector<MatchKeyParam> &match_key,
  mbr_hdl_t mbr, entry_handle_t *handle, int priority
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->add_entry(match_key, mbr, handle, priority);
}

MatchErrorCode
Switch::mt_indirect_modify_entry(
  const std::string &table_name, entry_handle_t handle, mbr_hdl_t mbr
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->modify_entry(handle, mbr);
}
 
MatchErrorCode
Switch::mt_indirect_delete_entry(
  const std::string &table_name, entry_handle_t handle
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->delete_entry(handle);
}

MatchErrorCode
Switch::mt_indirect_set_entry_ttl(
    const std::string &table_name,
    entry_handle_t handle,
    unsigned int ttl_ms
)
{
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  if(!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  return abstract_table->set_entry_ttl(handle, ttl_ms);
}

MatchErrorCode
Switch::mt_indirect_set_default_member(
  const std::string &table_name, mbr_hdl_t mbr
)
{
  MatchErrorCode rc;
  MatchTableIndirect *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->set_default_member(mbr);
}

MatchErrorCode
Switch::get_mt_indirect_ws(
  const std::string &table_name, MatchTableIndirectWS **table
)
{
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  if(!abstract_table) return MatchErrorCode::INVALID_TABLE_NAME;
  *table = dynamic_cast<MatchTableIndirectWS *>(abstract_table);
  if(!(*table)) return MatchErrorCode::WRONG_TABLE_TYPE;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
Switch::mt_indirect_ws_create_group(
  const std::string &table_name, grp_hdl_t *grp
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->create_group(grp);
}
 
MatchErrorCode
Switch::mt_indirect_ws_delete_group(
  const std::string &table_name, grp_hdl_t grp
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->delete_group(grp);
}
 
MatchErrorCode
Switch::mt_indirect_ws_add_member_to_group(
  const std::string &table_name, mbr_hdl_t mbr, grp_hdl_t grp
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->add_member_to_group(mbr, grp);
}

MatchErrorCode
Switch::mt_indirect_ws_remove_member_from_group(
  const std::string &table_name, mbr_hdl_t mbr, grp_hdl_t grp
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->remove_member_from_group(mbr, grp);
}

MatchErrorCode
Switch::mt_indirect_ws_add_entry(
  const std::string &table_name,
  const std::vector<MatchKeyParam> &match_key,
  grp_hdl_t grp, entry_handle_t *handle, int priority
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->add_entry_ws(match_key, grp, handle, priority);
}

MatchErrorCode
Switch::mt_indirect_ws_modify_entry(
  const std::string &table_name, entry_handle_t handle, grp_hdl_t grp
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->modify_entry_ws(handle, grp);
}

MatchErrorCode
Switch::mt_indirect_ws_set_default_group(
  const std::string &table_name, grp_hdl_t grp
)
{
  MatchErrorCode rc;
  MatchTableIndirectWS *table;
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  if((rc = get_mt_indirect_ws(table_name, &table)) != MatchErrorCode::SUCCESS)
    return rc;
  return table->set_default_group(grp);
}

MatchErrorCode Switch::table_read_counters(
    const std::string &table_name,
    entry_handle_t handle,
    MatchTableAbstract::counter_value_t *bytes,
    MatchTableAbstract::counter_value_t *packets
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  return abstract_table->query_counters(handle, bytes, packets);
}

MatchErrorCode Switch::table_reset_counters(
    const std::string &table_name
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  return abstract_table->reset_counters();
}

RuntimeInterface::MeterErrorCode
Switch::meter_array_set_rates(
  const std::string &meter_name, const std::vector<Meter::rate_config_t> &configs
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MeterArray *meter_array = p4objects_rt->get_meter_array(meter_name);
  assert(meter_array);
  return meter_array->set_rates(configs);
}

RuntimeInterface::MeterErrorCode
Switch::meter_set_rates(
  const std::string &meter_name, size_t idx,
  const std::vector<Meter::rate_config_t> &configs
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MeterArray *meter_array = p4objects_rt->get_meter_array(meter_name);
  assert(meter_array);
  return meter_array->get_meter(idx).set_rates(configs);
}

RuntimeInterface::ErrorCode Switch::load_new_config(const std::string &new_config) {
  if(!enable_swap) return CONFIG_SWAP_DISABLED;
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // check that there is no ongoing config swap
  if(p4objects != p4objects_rt) return ONGOING_SWAP;
  p4objects_rt = std::make_shared<P4Objects>();
  std::istringstream ss(new_config);
  p4objects_rt->init_objects(ss);
  return SUCCESS;
}
 
RuntimeInterface::ErrorCode Switch::swap_configs() {
  if(!enable_swap) return CONFIG_SWAP_DISABLED;
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // no ongoing swap
  if(p4objects == p4objects_rt) return NO_ONGOING_SWAP;
  swap_ordered = true;
  return SUCCESS;
}

MatchErrorCode Switch::dump_table(
  const std::string& table_name,
  std::ostream &stream
) const {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  abstract_table->dump(stream);
  return MatchErrorCode::SUCCESS;
}

int Switch::do_swap() {
  if(!swap_ordered) return 1;
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  p4objects = p4objects_rt;
  Packet::swap_phv_factory(p4objects->get_phv_factory());
  swap_ordered = false;
  return 0;
}

LearnEngine *Switch::get_learn_engine()
{
  return p4objects->get_learn_engine();
}

AgeingMonitor *Switch::get_ageing_monitor()
{
  return p4objects->get_ageing_monitor();
}
