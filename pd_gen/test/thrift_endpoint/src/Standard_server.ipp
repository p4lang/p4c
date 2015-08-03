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

#include "Standard.h"

#include <iostream>
#include <sstream> 
#include <iomanip>

namespace bm_runtime { namespace standard {

class StandardHandler : virtual public StandardIf {
public:
  StandardHandler() { }

  std::string ToHex(const std::string& s, bool upper_case = false) {
    std::ostringstream ret;

    for (std::string::size_type i = 0; i < s.length(); i++) {
      ret << std::setw(2) << std::setfill('0') << std::hex
	  << (upper_case ? std::uppercase : std::nouppercase)
	  << (int) static_cast<unsigned char>(s[i]);
    }

    return ret.str();
  }

  void print_spec(const std::vector<std::string> &v) {
    for(const auto &e : v)
      std::cout << ToHex(e) << " ";
    std::cout << std::endl;
  }

  void print_match_param(const BmMatchParam &param) {
    switch(param.type) {
    case BmMatchParamType::type::EXACT:
      std::cout << "EXACT: "
		<< ToHex(param.exact.key);
      break;
    case BmMatchParamType::type::LPM:
      std::cout << "LPM: "
		<< ToHex(param.lpm.key) << "/" << param.lpm.prefix_length;
      break;
    case BmMatchParamType::type::TERNARY:
      std::cout << "TERNARY: "
		<<ToHex(param.ternary.key) << "&&&" << ToHex(param.ternary.mask);
      break;
    case BmMatchParamType::type::VALID:
      std::cout << "VALID: "
		<<std::boolalpha << param.valid.key << std::noboolalpha;
      break;
    default:
      assert(0 && "invalid match type");
      break;
    }
    std::cout << std::endl;
  }

  BmEntryHandle bm_mt_add_entry(const std::string& table_name, const BmMatchParams& match_key, const std::string& action_name, const BmActionData& action_data, const BmAddEntryOptions& options) {
    std::cout << "bm_table_add_entry" << std::endl
	      << table_name << std::endl;
    for(const auto &p : match_key)
      print_match_param(p);
    std::cout << action_name << std::endl;
    print_spec(action_data);
    if(options.__isset.priority)
      std::cout << options.priority << std::endl;
    return 0;
  }

  void bm_mt_set_default_action(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    std::cout << "bm_set_default_action" << std::endl
	      << table_name << std::endl
	      << action_name << std::endl;
    print_spec(action_data);
  }

  void bm_mt_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    std::cout << "bm_table_delete_entry" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl;
  }

  void bm_mt_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const std::string &action_name, const BmActionData& action_data) {
    std::cout << "bm_table_modify_entry" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl
	      << action_name << std::endl;
    print_spec(action_data);
  }

  void bm_mt_set_entry_ttl(const std::string& table_name, const BmEntryHandle entry_handle, const int32_t timeout_ms) {
    std::cout << "bm_mt_set_entry_ttl" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl
	      << timeout_ms << std::endl;
  }

  BmMemberHandle bm_mt_indirect_add_member(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    std::cout << "bm_mt_indirect_add_member" << std::endl
	      << table_name << std::endl
	      << action_name << std::endl;
    print_spec(action_data);
    return 0;
  }

  void bm_mt_indirect_delete_member(const std::string& table_name, const BmMemberHandle mbr_handle) {
    std::cout << "bm_mt_indirect_delete_member" << std::endl
	      << table_name << std::endl
	      << mbr_handle << std::endl;
  }

  void bm_mt_indirect_modify_member(const std::string& table_name, const BmMemberHandle mbr_handle, const std::string& action_name, const BmActionData& action_data) {
    std::cout << "bm_mt_indirect_modify_member" << std::endl
	      << table_name << std::endl
	      << mbr_handle << std::endl
	      << action_name << std::endl;
    print_spec(action_data);
  }

  BmEntryHandle bm_mt_indirect_add_entry(const std::string& table_name, const BmMatchParams& match_key, const BmMemberHandle mbr_handle, const BmAddEntryOptions& options) {
    std::cout << "bm_mt_indirect_add_entry" << std::endl
	      << table_name << std::endl;
    for(const auto &p : match_key)
      print_match_param(p);
    std::cout << mbr_handle << std::endl;
    if(options.__isset.priority)
      std::cout << options.priority << std::endl;
    return 0;
  }

  void bm_mt_indirect_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const BmMemberHandle mbr_handle) {
    std::cout << "bm_mt_indirect_modify_entry" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl
	      << mbr_handle << std::endl;
  }

  void bm_mt_indirect_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    std::cout << "bm_mt_indirect_delete_entry" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl;
  }

  void bm_mt_indirect_set_entry_ttl(const std::string& table_name, const BmEntryHandle entry_handle, const int32_t timeout_ms) {
    std::cout << "bm_mt_indirect_set_entry_ttl" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl
	      << timeout_ms << std::endl;
  }

  void bm_mt_indirect_set_default_member(const std::string& table_name, const BmMemberHandle mbr_handle) {
    std::cout << "bm_mt_indirect_set_default_member" << std::endl
	      << table_name << std::endl
	      << mbr_handle << std::endl;
  }

  BmGroupHandle bm_mt_indirect_ws_create_group(const std::string& table_name) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_create_group\n");
    return 0;
  }

  void bm_mt_indirect_ws_delete_group(const std::string& table_name, const BmGroupHandle grp_handle) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_delete_group\n");
  }

  void bm_mt_indirect_ws_add_member_to_group(const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_add_member_to_group\n");
  }

  void bm_mt_indirect_ws_remove_member_from_group(const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_remove_member_from_group\n");
  }

  BmEntryHandle bm_mt_indirect_ws_add_entry(const std::string& table_name, const BmMatchParams& match_key, const BmGroupHandle grp_handle, const BmAddEntryOptions& options) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_add_entry\n");
    return 0;
  }

  void bm_mt_indirect_ws_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const BmGroupHandle grp_handle) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_modify_entry\n");
  }

  void bm_mt_indirect_ws_set_default_group(const std::string& table_name, const BmGroupHandle grp_handle) {
    // Your implementation goes here
    printf("bm_mt_indirect_ws_set_default_group\n");
  }

  void bm_table_read_counter(BmCounterValue& _return, const std::string& table_name, const BmEntryHandle entry_handle) {
    std::cout << "bm_table_read_counter" << std::endl
	      << table_name << std::endl
	      << entry_handle << std::endl;      
  }

  void bm_table_reset_counters(const std::string& table_name) {
    std::cout << "bm_table_reset_counters" << std::endl
	      << table_name << std::endl;
  }

  void bm_learning_ack(const BmLearningListId list_id, const BmLearningBufferId buffer_id, const std::vector<BmLearningSampleId> & sample_ids) {
    // Your implementation goes here
    printf("bm_learning_ack\n");
  }

  void bm_learning_ack_buffer(const BmLearningListId list_id, const BmLearningBufferId buffer_id) {
    // Your implementation goes here
    printf("bm_learning_ack_buffer\n");
  }

  void bm_load_new_config(const std::string& config_str) {
    // Your implementation goes here
    printf("bm_load_new_config\n");
  }

  void bm_swap_configs() {
    // Your implementation goes here
    printf("bm_swap_configs\n");
  }

  void bm_meter_array_set_rates(const std::string& meter_array_name, const std::vector<BmMeterRateConfig> & rates) {
    // Your implementation goes here
    printf("bm_meter_array_set_rates\n");
  }

  void bm_meter_set_rates(const std::string& meter_array_name, const int32_t index, const std::vector<BmMeterRateConfig> & rates) {
    std::cout << "bm_meter_set_rates" << std::endl
	      << meter_array_name << std::endl
	      << index << std::endl;
    for(const auto &rate : rates) {
      std::cout << rate.units_per_micros << " " << rate.burst_size << std::endl;
    }
  }

  void bm_dev_mgr_add_port(const std::string& iface_name, const int32_t port_num, const std::string& pcap_path) {
    // Your implementation goes here
    printf("bm_dev_mgr_add_port\n");
  }

  void bm_dev_mgr_remove_port(const int32_t port_num) {
    // Your implementation goes here
    printf("bm_dev_mgr_remove_port\n");
  }

  void bm_dump_table(std::string& _return, const std::string& table_name) {
    // Your implementation goes here
    printf("bm_dump_table\n");
  }
  
};

} }
