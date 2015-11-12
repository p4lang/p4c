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

#ifndef _BM_SWITCH_H_
#define _BM_SWITCH_H_

#include <memory>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <set>

#include <boost/thread/shared_mutex.hpp>

#include "P4Objects.h"
#include "queue.h"
#include "packet.h"
#include "learning.h"
#include "runtime_interface.h"
#include "dev_mgr.h"

class Switch : public RuntimeInterface, public DevMgr {
public:
  Switch(bool enable_swap = false);

  /* Specify that is required for this target switch, i.e. the field
     needs to be defined in the input json */
  void add_required_field(const std::string &header_name,
			  const std::string &field_name);

  /* Force arithmetic on field. No effect if field is not defined in the
     input json */
  void force_arith_field(const std::string &header_name,
			 const std::string &field_name);

  int init_objects(const std::string &json_path);

  int init_from_command_line_options(int argc, char *argv[]);

  P4Objects *get_p4objects() { return p4objects.get(); }

  virtual int receive(int port_num, const char *buffer, int len) = 0;

  virtual void start_and_return() = 0;

  // returns the Thrift port if one was specified on the command line
  int get_runtime_port() { return thrift_port; }

public:
  MatchErrorCode
  mt_add_entry(const std::string &table_name,
	       const std::vector<MatchKeyParam> &match_key,
	       const std::string &action_name,
	       ActionData action_data,
	       entry_handle_t *handle,
	       int priority = -1/*only used for ternary*/) override;
  
  MatchErrorCode
  mt_set_default_action(const std::string &table_name,
			const std::string &action_name,
			ActionData action_data) override;
  
  MatchErrorCode
  mt_delete_entry(const std::string &table_name,
		  entry_handle_t handle) override;

  MatchErrorCode
  mt_modify_entry(const std::string &table_name,
		  entry_handle_t handle,
		  const std::string &action_name,
		  ActionData action_data) override;

  MatchErrorCode
  mt_set_entry_ttl(const std::string &table_name,
		   entry_handle_t handle,
		   unsigned int ttl_ms);

  MatchErrorCode
  mt_indirect_add_member(const std::string &table_name,
			 const std::string &action_name,
			 ActionData action_data,
			 mbr_hdl_t *mbr) override;
  
  MatchErrorCode
  mt_indirect_delete_member(const std::string &table_name,
			    mbr_hdl_t mbr) override;

  MatchErrorCode
  mt_indirect_modify_member(const std::string &table_name,
			    mbr_hdl_t mbr_hdl,
			    const std::string &action_name,
			    ActionData action_data) override;
  
  MatchErrorCode
  mt_indirect_add_entry(const std::string &table_name,
			const std::vector<MatchKeyParam> &match_key,
			mbr_hdl_t mbr,
			entry_handle_t *handle,
			int priority = 1) override;

  MatchErrorCode
  mt_indirect_modify_entry(const std::string &table_name,
			   entry_handle_t handle,
			   mbr_hdl_t mbr) override;
  
  MatchErrorCode
  mt_indirect_delete_entry(const std::string &table_name,
			   entry_handle_t handle) override;

  MatchErrorCode
  mt_indirect_set_entry_ttl(const std::string &table_name,
			    entry_handle_t handle,
			    unsigned int ttl_ms);

  MatchErrorCode
  mt_indirect_set_default_member(const std::string &table_name,
				 mbr_hdl_t mbr) override;
  
  MatchErrorCode
  mt_indirect_ws_create_group(const std::string &table_name,
			      grp_hdl_t *grp) override;
  
  MatchErrorCode
  mt_indirect_ws_delete_group(const std::string &table_name,
			      grp_hdl_t grp) override;
  
  MatchErrorCode
  mt_indirect_ws_add_member_to_group(const std::string &table_name,
				     mbr_hdl_t mbr, grp_hdl_t grp) override;
 
  MatchErrorCode
  mt_indirect_ws_remove_member_from_group(const std::string &table_name,
					  mbr_hdl_t mbr, grp_hdl_t grp) override;

  MatchErrorCode
  mt_indirect_ws_add_entry(const std::string &table_name,
			   const std::vector<MatchKeyParam> &match_key,
			   grp_hdl_t grp,
			   entry_handle_t *handle,
			   int priority = 1) override;

  MatchErrorCode
  mt_indirect_ws_modify_entry(const std::string &table_name,
			      entry_handle_t handle,
			      grp_hdl_t grp) override;

  MatchErrorCode
  mt_indirect_ws_set_default_group(const std::string &table_name,
				   grp_hdl_t grp) override;

  MatchErrorCode
  mt_read_counters(const std::string &table_name,
		   entry_handle_t handle,
		   MatchTableAbstract::counter_value_t *bytes,
		   MatchTableAbstract::counter_value_t *packets) override;

  MatchErrorCode
  mt_reset_counters(const std::string &table_name) override;

  MatchErrorCode
  mt_write_counters(const std::string &table_name,
		    entry_handle_t handle,
		    MatchTableAbstract::counter_value_t bytes,
		    MatchTableAbstract::counter_value_t packets) override;

  Counter::CounterErrorCode
  read_counters(const std::string &counter_name,
		size_t index,
		MatchTableAbstract::counter_value_t *bytes,
		MatchTableAbstract::counter_value_t *packets) override;

  Counter::CounterErrorCode
  reset_counters(const std::string &counter_name) override;

  Counter::CounterErrorCode
  write_counters(const std::string &counter_name,
		 size_t index,
		 MatchTableAbstract::counter_value_t bytes,
		 MatchTableAbstract::counter_value_t packets) override;

  MeterErrorCode
  meter_array_set_rates(const std::string &meter_name,
			const std::vector<Meter::rate_config_t> &configs) override;

  MeterErrorCode
  meter_set_rates(const std::string &meter_name, size_t idx,
		  const std::vector<Meter::rate_config_t> &configs) override;

  RegisterErrorCode
  register_read(const std::string &register_name,
		const size_t idx, Data *value) override;

  RegisterErrorCode
  register_write(const std::string &meter_name,
		 const size_t idx, Data value) override;

  RuntimeInterface::ErrorCode
  load_new_config(const std::string &new_config) override;

  RuntimeInterface::ErrorCode
  swap_configs() override;

  RuntimeInterface::ErrorCode
  reset_state() override;

  MatchErrorCode
  dump_table(const std::string& table_name,
	     std::ostream &stream) const override;

  LearnEngine *get_learn_engine();

  template<typename T>
  std::shared_ptr<T> get_component() {
    const auto &search = components.find(std::type_index(typeid(T)));
    if(search == components.end()) return nullptr;
    return std::static_pointer_cast<T>(search->second);
  }

  AgeingMonitor *get_ageing_monitor();

protected:
  int swap_requested() { return swap_ordered; }
  // TODO: should I return shared_ptrs instead of raw_ptrs?
  // invalidate all pointers obtained with get_pipeline(), get_parser(),...
  int do_swap();

  template<typename T>
  bool add_component(std::shared_ptr<T> ptr) {
    std::shared_ptr<void> ptr_ = std::static_pointer_cast<void>(ptr);
    const auto &r = components.insert({std::type_index(typeid(T)), ptr_});
    return r.second;
  }

  // do these methods need any protection
  Pipeline *get_pipeline(const std::string &name) {
    return p4objects->get_pipeline(name);
  }

  Parser *get_parser(const std::string &name) {
    return p4objects->get_parser(name);
  }

  Deparser *get_deparser(const std::string &name) {
    return p4objects->get_deparser(name);
  }

  FieldList *get_field_list(const p4object_id_t field_list_id) {
    return p4objects->get_field_list(field_list_id);
  }

private:
  MatchErrorCode get_mt_indirect(const std::string &table_name,
				 MatchTableIndirect **table);
  MatchErrorCode get_mt_indirect_ws(const std::string &table_name,
				    MatchTableIndirectWS **table);

protected:
  std::unordered_map<std::type_index, std::shared_ptr<void> > components{};

private:
  typedef P4Objects::header_field_pair header_field_pair;

private:
  mutable boost::shared_mutex request_mutex{};
  std::atomic<bool> swap_ordered{false};

  std::shared_ptr<P4Objects> p4objects{nullptr};
  std::shared_ptr<P4Objects> p4objects_rt{nullptr};

  bool enable_swap{false};

  std::set<header_field_pair> required_fields{};
  std::set<header_field_pair> arith_fields{};
  
  int thrift_port{};

  int device_id{};
};

#endif
