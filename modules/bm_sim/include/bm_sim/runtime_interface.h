#ifndef _BM_RUNTIME_INTERFACE_H_
#define _BM_RUNTIME_INTERFACE_H_

#include <string>

#include "tables.h"

class RuntimeInterface {
public:
  virtual int table_add_entry(const std::string &table_name,
			      ExactMatchEntry &&entry,
			      entry_handle_t *handle) = 0;
  virtual int table_add_entry(const std::string &table_name,
  			      LongestPrefixMatchEntry &&entry,
			      entry_handle_t *handle) = 0;
  virtual int table_add_entry(const std::string &table_name,
  			      TernaryMatchEntry &&entry,
			      entry_handle_t *handle) = 0;

  virtual MatchTable::ErrorCode
  table_add_exact_match_entry(const std::string &table_name,
			      const std::string &action_name,
			      const ByteContainer &key,
			      const ActionData &action_data,
			      entry_handle_t *handle) = 0;

  virtual MatchTable::ErrorCode
  table_add_lpm_entry(const std::string &table_name,
		      const std::string &action_name,
		      const ByteContainer &key,
		      unsigned int prefix_length,
		      const ActionData &action_data,
		      entry_handle_t *handle) = 0;

  virtual MatchTable::ErrorCode
  table_add_ternary_match_entry(const std::string &table_name,
				const std::string &action_name,
				const ByteContainer &key,
				const ByteContainer &mask,
				int priority,
				const ActionData &action_data,
				entry_handle_t *handle) = 0;

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
};

#endif
