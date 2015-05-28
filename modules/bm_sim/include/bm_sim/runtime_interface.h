#ifndef _BM_RUNTIME_INTERFACE_H_
#define _BM_RUNTIME_INTERFACE_H_

#include <string>

#include "match_tables.h"

class RuntimeInterface {
public:
  enum ErrorCode {
    SUCCESS = 0,
    CONFIG_SWAP_DISABLED,
    ONGOING_SWAP,
    NO_ONGOING_SWAP
  };

public:
  virtual ~RuntimeInterface() { }

  virtual MatchErrorCode
  match_table_add_entry(const std::string &table_name,
			const std::vector<MatchKeyParam> &match_key,
			const std::string &action_name,
			ActionData action_data, // will be moved
			entry_handle_t *handle,
			int priority = -1/*only used for ternary*/) = 0;
		  
  virtual MatchErrorCode
  match_table_set_default_action(const std::string &table_name,
				 const std::string &action_name,
				 ActionData action_data) = 0;

  virtual MatchErrorCode
  match_table_delete_entry(const std::string &table_name,
			   entry_handle_t handle) = 0;

  virtual MatchErrorCode
  match_table_modify_entry(const std::string &table_name,
			   entry_handle_t handle,
			   const std::string &action_name,
			   ActionData action_data) = 0;

  virtual MatchErrorCode
  table_read_counters(const std::string &table_name,
		      entry_handle_t handle,
		      MatchTableAbstract::counter_value_t *bytes,
		      MatchTableAbstract::counter_value_t *packets) = 0;

  virtual MatchErrorCode
  table_reset_counters(const std::string &table_name) = 0;

  virtual ErrorCode
  load_new_config(const std::string &new_config) = 0;
  
  virtual ErrorCode
  swap_configs() = 0;

};

#endif
