#include <cassert>

#include "bm_sim/switch.h"
#include "bm_sim/P4Objects.h"

Switch::Switch(bool enable_swap)
  : enable_swap(enable_swap) {
  p4objects = std::make_shared<P4Objects>();
  p4objects_rt = p4objects;
  // p4objects = std::unique_ptr<P4Objects>(new P4Objects());
  pre = std::unique_ptr<McPre>(new McPre());
}

void Switch::init_objects(const std::string &json_path) {
  std::fstream fs(json_path);
  p4objects->init_objects(fs);
  Packet::set_phv_factory(p4objects->get_phv_factory());
}

MatchErrorCode Switch::match_table_add_entry(
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

MatchErrorCode Switch::match_table_set_default_action(
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

MatchErrorCode Switch::match_table_delete_entry(
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

MatchErrorCode Switch::match_table_modify_entry(
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

RuntimeInterface::ErrorCode Switch::load_new_config(const std::string &new_config) {
  if(!enable_swap) return CONFIG_SWAP_DISABLED;
  boost::unique_lock<boost::shared_mutex> lock(request_mutex);
  // check that there is no ongoing config swap
  if(p4objects != p4objects_rt) return ONGOING_SWAP;
  p4objects_rt = std::make_shared<P4Objects>();
  std::stringstream ss(new_config);
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
