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
#include <fstream>

#include "bm_sim/switch.h"
#include "bm_sim/P4Objects.h"
#include "bm_sim/options_parse.h"
#include "bm_sim/logger.h"

static void
packet_handler(int port_num, const char *buffer, int len, void *cookie)
{
    ((Switch *) cookie)->receive(port_num, buffer, len);
}

Switch::Switch(bool enable_swap)
  : DevMgr(),
    enable_swap(enable_swap) {
  p4objects = std::make_shared<P4Objects>();
  p4objects_rt = p4objects;
}

void
Switch::add_required_field(const std::string &header_name,
			   const std::string &field_name) {
  required_fields.insert(std::make_pair(header_name, field_name));
}

void
Switch::force_arith_field(const std::string &header_name,
			  const std::string &field_name) {
  arith_fields.insert(std::make_pair(header_name, field_name));
}

int
Switch::init_objects(const std::string &json_path) {
  std::ifstream fs(json_path, std::ios::in);
  if(!fs) {
    std::cout << "JSON input file " << json_path << " cannot be opened\n";
    return 1;
  }

  int status = p4objects->init_objects(fs, required_fields, arith_fields);
  if(status != 0) return status;
  Packet::set_phv_factory(p4objects->get_phv_factory());

  return status;
}

int
Switch::init_from_command_line_options(int argc, char *argv[]) {
  OptionsParser parser;
  parser.parse(argc, argv);
  int status = init_objects(parser.config_file_path);
  if(status != 0) return status;

  if(parser.console_logging)
    Logger::set_logger_console();

  if(parser.file_logger != "")
    Logger::set_logger_file(parser.file_logger);

  setUseFiles(parser.useFiles, parser.waitTime);

  for(const auto &iface : parser.ifaces) {
    std::cout << "Adding interface " << iface.second
	      << " as port " << iface.first
              << (parser.useFiles ? " (files)" : "")
              << std::endl;

    const char* inFileName = NULL;
    const char* outFileName = NULL;

    std::string inFile;
    std::string outFile;

    if (parser.useFiles)
    {
	inFile = iface.second + "_in.pcap";
	inFileName = inFile.c_str();
	outFile = iface.second + "_out.pcap";
	outFileName = outFile.c_str();
    }
    else if (parser.pcap) {
	inFile = iface.second + ".pcap";
	inFileName = inFile.c_str();
	outFileName = inFileName;
    }

    port_add(iface.second, iface.first, inFileName, outFileName);
  }
  thrift_port = parser.thrift_port;
  device_id = parser.device_id;

  // TODO: is this the right place to do this?
  set_packet_handler(packet_handler, (void *) this);
  start();
  return status;
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

MatchErrorCode Switch::mt_read_counters(
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

MatchErrorCode Switch::mt_reset_counters(
  const std::string &table_name
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  return abstract_table->reset_counters();
}

MatchErrorCode Switch::mt_write_counters(
  const std::string &table_name,
  entry_handle_t handle,
  MatchTableAbstract::counter_value_t bytes,
  MatchTableAbstract::counter_value_t packets
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  MatchTableAbstract *abstract_table = 
    p4objects_rt->get_abstract_match_table(table_name);
  assert(abstract_table);
  return abstract_table->write_counters(handle, bytes, packets);
}

Counter::CounterErrorCode Switch::read_counters(
  const std::string &counter_name,
  size_t index,
  MatchTableAbstract::counter_value_t *bytes,
  MatchTableAbstract::counter_value_t *packets
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  CounterArray *counter_array = p4objects_rt->get_counter_array(counter_name);
  assert(counter_array);
  return (*counter_array)[index].query_counter(bytes, packets);
}

Counter::CounterErrorCode Switch::reset_counters(
  const std::string &counter_name
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  CounterArray *counter_array = p4objects_rt->get_counter_array(counter_name);
  assert(counter_array);
  return counter_array->reset_counters();
}

Counter::CounterErrorCode Switch::write_counters(
  const std::string &counter_name,
  size_t index,
  MatchTableAbstract::counter_value_t bytes,
  MatchTableAbstract::counter_value_t packets
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  CounterArray *counter_array = p4objects_rt->get_counter_array(counter_name);
  assert(counter_array);
  return (*counter_array)[index].write_counter(bytes, packets);
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

RuntimeInterface::RegisterErrorCode
Switch::register_read(
  const std::string &register_name,
  const size_t idx, Data *value
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  RegisterArray *register_array =
    p4objects_rt->get_register_array(register_name);
  if(!register_array) return Register::ERROR;
  if(idx >= register_array->size()) return Register::INVALID_INDEX;
  value->set((*register_array)[idx]);
  return Register::SUCCESS;
}

RuntimeInterface::RegisterErrorCode
Switch::register_write(
  const std::string &register_name,
  const size_t idx, Data value
) {
  boost::shared_lock<boost::shared_mutex> lock(request_mutex);
  RegisterArray *register_array =
    p4objects_rt->get_register_array(register_name);
  if(!register_array) return Register::ERROR;
  if(idx >= register_array->size()) return Register::INVALID_INDEX;
  (*register_array)[idx].set(std::move(value));
  return Register::SUCCESS;
}

RuntimeInterface::ErrorCode Switch::load_new_config(const std::string &new_config) {
  if(!enable_swap) return CONFIG_SWAP_DISABLED;
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // check that there is no ongoing config swap
  if(p4objects != p4objects_rt) return ONGOING_SWAP;
  p4objects_rt = std::make_shared<P4Objects>();
  std::istringstream ss(new_config);
  p4objects_rt->init_objects(ss, required_fields, arith_fields);
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

RuntimeInterface::ErrorCode Switch::reset_state() {
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  p4objects_rt->reset_state();
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
