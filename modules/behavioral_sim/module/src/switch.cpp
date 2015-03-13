#include <cassert>

#include "behavioral_sim/switch.h"
#include "behavioral_sim/P4Objects.h"

Switch::Switch() {
  p4objects = std::unique_ptr<P4Objects>(new P4Objects());
}

void Switch::init_objects(const std::string &json_path) {
  std::fstream fs(json_path);
  p4objects->init_objects(fs);
}

int Switch::table_add_exact_match_entry(
    const std::string &table_name,
    const std::string &action_name,
    const ByteContainer &key,
    const ActionData &action_data,
    entry_handle_t *handle
) {
  ExactMatchTable *table = p4objects->get_exact_match_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  ActionFnEntry action_entry(action, action_data);
  const ControlFlowNode *next_node = table->get_next_node(action->get_id());
  ExactMatchEntry match_entry(key, action_entry, next_node);
  table->add_entry(std::move(match_entry), handle);
  return 0;
}

int Switch::table_add_lpm_entry(
    const std::string &table_name,
    const std::string &action_name,
    const ByteContainer &key,
    unsigned int prefix_length,
    const ActionData &action_data,
    entry_handle_t *handle
) {
  LongestPrefixMatchTable *table = p4objects->get_lpm_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  ActionFnEntry action_entry(action, action_data);
  const ControlFlowNode *next_node = table->get_next_node(action->get_id());
  LongestPrefixMatchEntry match_entry(key, action_entry, prefix_length, next_node);
  table->add_entry(std::move(match_entry), handle);
  return 0;
}

int Switch::table_add_ternary_match_entry(
    const std::string &table_name,
    const std::string &action_name,
    const ByteContainer &key,
    const ByteContainer &mask,
    int priority,
    const ActionData &action_data,
    entry_handle_t *handle
) {
  TernaryMatchTable *table = p4objects->get_ternary_match_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  ActionFnEntry action_entry(action, action_data);
  const ControlFlowNode *next_node = table->get_next_node(action->get_id());
  TernaryMatchEntry match_entry(key, action_entry, mask, priority, next_node);
  table->add_entry(std::move(match_entry), handle);
  return 0;
}

int Switch::table_add_entry(const std::string &table_name,
			    ExactMatchEntry &&entry,
			    entry_handle_t *handle) {
  ExactMatchTable *table = p4objects->get_exact_match_table(table_name);
  assert(table);
  table->add_entry(std::move(entry), handle);
  return 0;
}

int Switch::table_add_entry(const std::string &table_name,
			    LongestPrefixMatchEntry &&entry,
			    entry_handle_t *handle) {
  LongestPrefixMatchTable *table = p4objects->get_lpm_table(table_name);
  assert(table);
  table->add_entry(std::move(entry), handle);
  return 0;
}

int Switch::table_add_entry(const std::string &table_name,
			    TernaryMatchEntry &&entry,
			    entry_handle_t *handle) {
  TernaryMatchTable *table = p4objects->get_ternary_match_table(table_name);
  assert(table);
  table->add_entry(std::move(entry), handle);
  return 0;
}

int Switch::table_set_default_action(const std::string &table_name,
				     const std::string &action_name,
				     const ActionData &action_data) {
  MatchTable *table = p4objects->get_match_table(table_name);
  const ActionFn *action = p4objects->get_action(action_name);
  assert(table); assert(action);
  const ControlFlowNode *next_node = table->get_next_node(action->get_id());
  ActionFnEntry action_entry(action, action_data);
  table->set_default_action(action_entry, next_node);
  return 0;
}

int Switch::table_delete_entry(const std::string &table_name,
			       entry_handle_t handle) {
  MatchTable *table = p4objects->get_match_table(table_name);
  assert(table);
  table->delete_entry(handle);
  return 0;
}
