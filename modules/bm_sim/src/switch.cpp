#include <cassert>

#include "bm_sim/switch.h"
#include "bm_sim/P4Objects.h"

Switch::Switch() {
  p4objects = std::unique_ptr<P4Objects>(new P4Objects());
  pre = std::unique_ptr<McPre>(new McPre());
}

void Switch::init_objects(const std::string &json_path) {
  std::fstream fs(json_path);
  p4objects->init_objects(fs);
}

MatchTable::ErrorCode Switch::table_add_entry(
    const std::string &table_name,
    const std::vector<MatchKeyParam> &match_key,
    const std::string &action_name,
    const ActionData &action_data,
    entry_handle_t *handle,
    int priority
) {
  MatchTable *table = p4objects->get_match_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  return table->add_entry(match_key, *action, action_data, handle, priority);
}

MatchTable::ErrorCode Switch::table_set_default_action(
    const std::string &table_name,
    const std::string &action_name,
    const ActionData &action_data
) {
  MatchTable *table = p4objects->get_match_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  const ControlFlowNode *next_node = table->get_next_node(action->get_id());
  ActionFnEntry action_entry(action, action_data);
  return table->set_default_action(action_entry, next_node);
}

MatchTable::ErrorCode Switch::table_delete_entry(
    const std::string &table_name,
    entry_handle_t handle
) {
  MatchTable *table = p4objects->get_match_table(table_name);
  assert(table);
  return table->delete_entry(handle);
}

MatchTable::ErrorCode Switch::table_modify_entry(
    const std::string &table_name,
    entry_handle_t handle,
    const std::string &action_name,
    const ActionData &action_data
) {
  MatchTable *table = p4objects->get_match_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  const ControlFlowNode *next_node = table->get_next_node(action->get_id());
  ActionFnEntry action_entry(action, action_data);
  return table->modify_entry(handle, action_entry, next_node);
}

MatchTable::ErrorCode Switch::table_read_counters(
    const std::string &table_name,
    entry_handle_t handle,
    MatchTable::counter_value_t *bytes,
    MatchTable::counter_value_t *packets
) {
  MatchTable *table = p4objects->get_match_table(table_name);
  assert(table);
  return table->query_counters(handle, bytes, packets);
}

MatchTable::ErrorCode Switch::table_reset_counters(
    const std::string &table_name
) {
  MatchTable *table = p4objects->get_match_table(table_name);
  assert(table);
  return table->reset_counters();
}

LearnEngine *Switch::get_learn_engine()
{
  return p4objects->get_learn_engine();
}
