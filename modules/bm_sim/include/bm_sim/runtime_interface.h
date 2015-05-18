#ifndef _BM_RUNTIME_INTERFACE_H_
#define _BM_RUNTIME_INTERFACE_H_

#include <string>

#include "tables.h"

class RuntimeInterface {
public:
  virtual ~RuntimeInterface() { }

  virtual MatchTable::ErrorCode
  table_add_entry(const std::string &table_name,
		  const std::vector<MatchKeyParam> &match_key,
		  const std::string &action_name,
		  const ActionData &action_data,
		  entry_handle_t *handle,
		  int priority = -1/*only used for ternary*/) = 0;
		  
  virtual MatchTable::ErrorCode
  table_set_default_action(const std::string &table_name,
			   const std::string &action_name,
			   const ActionData &action_data) = 0;

  virtual MatchTable::ErrorCode
  table_delete_entry(const std::string &table_name,
		     entry_handle_t handle) = 0;

  virtual MatchTable::ErrorCode
  table_modify_entry(const std::string &table_name,
		     entry_handle_t handle,
		     const std::string &action_name,
		     const ActionData &action_data) = 0;

  virtual MatchTable::ErrorCode
  table_read_counters(const std::string &table_name,
		      entry_handle_t handle,
		      MatchTable::counter_value_t *bytes,
		      MatchTable::counter_value_t *packets) = 0;

  virtual MatchTable::ErrorCode
  table_reset_counters(const std::string &table_name) = 0;
};

#endif
