#ifndef _BM_RUNTIME_INTERFACE_H_
#define _BM_RUNTIME_INTERFACE_H_

#include <string>

#include "match_tables.h"

class RuntimeInterface {
public:
  typedef MatchTableIndirect::mbr_hdl_t mbr_hdl_t;
  typedef MatchTableIndirectWS::grp_hdl_t grp_hdl_t;

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
  mt_add_entry(const std::string &table_name,
	       const std::vector<MatchKeyParam> &match_key,
	       const std::string &action_name,
	       ActionData action_data, // will be moved
	       entry_handle_t *handle,
	       int priority = -1/*only used for ternary*/) = 0;
  
  virtual MatchErrorCode
  mt_set_default_action(const std::string &table_name,
			const std::string &action_name,
			ActionData action_data) = 0;
  
  virtual MatchErrorCode
  mt_delete_entry(const std::string &table_name,
		  entry_handle_t handle) = 0;
  
  virtual MatchErrorCode
  mt_modify_entry(const std::string &table_name,
		  entry_handle_t handle,
		  const std::string &action_name,
		  ActionData action_data) = 0;
  
  virtual MatchErrorCode
  mt_indirect_add_member(const std::string &table_name,
			 const std::string &action_name,
			 ActionData action_data,
			 mbr_hdl_t *mbr) = 0;
  
  virtual MatchErrorCode
  mt_indirect_delete_member(const std::string &table_name,
			    mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_modify_member(const std::string &table_name,
			    mbr_hdl_t mbr_hdl,
			    const std::string &action_name,
			    ActionData action_data) = 0;
  
  virtual MatchErrorCode
  mt_indirect_add_entry(const std::string &table_name,
			const std::vector<MatchKeyParam> &match_key,
			mbr_hdl_t mbr,
			entry_handle_t *handle,
			int priority = 1) = 0;

  virtual MatchErrorCode
  mt_indirect_modify_entry(const std::string &table_name,
			   entry_handle_t handle,
			   mbr_hdl_t mbr) = 0;
  
  virtual MatchErrorCode
  mt_indirect_delete_entry(const std::string &table_name,
			   entry_handle_t handle) = 0;

  virtual MatchErrorCode
  mt_indirect_set_default_member(const std::string &table_name,
				 mbr_hdl_t mbr) = 0;
  
  virtual MatchErrorCode
  mt_indirect_ws_create_group(const std::string &table_name,
			      grp_hdl_t *grp) = 0;
  
  virtual MatchErrorCode
  mt_indirect_ws_delete_group(const std::string &table_name,
			      grp_hdl_t grp) = 0;
  
  virtual MatchErrorCode
  mt_indirect_ws_add_member_to_group(const std::string &table_name,
				     mbr_hdl_t mbr, grp_hdl_t grp) = 0;
 
  virtual MatchErrorCode
  mt_indirect_ws_remove_member_from_group(const std::string &table_name,
					  mbr_hdl_t mbr, grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_add_entry(const std::string &table_name,
			   const std::vector<MatchKeyParam> &match_key,
			   grp_hdl_t grp,
			   entry_handle_t *handle,
			   int priority = 1) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_modify_entry(const std::string &table_name,
			      entry_handle_t handle,
			      grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_set_default_group(const std::string &table_name,
				   grp_hdl_t grp) = 0;
  

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
