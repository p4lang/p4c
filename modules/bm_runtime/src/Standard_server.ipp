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

#include <bm_sim/switch.h>

namespace bm_runtime { namespace standard {

typedef RuntimeInterface::mbr_hdl_t mbr_hdl_t;
typedef RuntimeInterface::grp_hdl_t grp_hdl_t;

class StandardHandler : virtual public StandardIf {
public:
  StandardHandler(Switch *sw)
    : switch_(sw) { }

  static TableOperationErrorCode::type get_exception_code(MatchErrorCode bm_code) {
    switch(bm_code) {
    case MatchErrorCode::TABLE_FULL:
      return TableOperationErrorCode::TABLE_FULL;
    case MatchErrorCode::INVALID_HANDLE:
      return TableOperationErrorCode::INVALID_HANDLE;
    case MatchErrorCode::EXPIRED_HANDLE:
      return TableOperationErrorCode::EXPIRED_HANDLE;
    case MatchErrorCode::COUNTERS_DISABLED:
      return TableOperationErrorCode::COUNTERS_DISABLED;
    case MatchErrorCode::AGEING_DISABLED:
      return TableOperationErrorCode::AGEING_DISABLED;
    case MatchErrorCode::INVALID_TABLE_NAME:
      return TableOperationErrorCode::INVALID_TABLE_NAME;
    case MatchErrorCode::INVALID_ACTION_NAME:
      return TableOperationErrorCode::INVALID_ACTION_NAME;
    case MatchErrorCode::WRONG_TABLE_TYPE:
      return TableOperationErrorCode::WRONG_TABLE_TYPE;
    case MatchErrorCode::INVALID_MBR_HANDLE:
      return TableOperationErrorCode::INVALID_MBR_HANDLE;
    case MatchErrorCode::MBR_STILL_USED:
      return TableOperationErrorCode::MBR_STILL_USED;
    case MatchErrorCode::MBR_ALREADY_IN_GRP:
      return TableOperationErrorCode::MBR_ALREADY_IN_GRP;
    case MatchErrorCode::MBR_NOT_IN_GRP:
      return TableOperationErrorCode::MBR_NOT_IN_GRP;
    case MatchErrorCode::INVALID_GRP_HANDLE:
      return TableOperationErrorCode::INVALID_GRP_HANDLE;
    case MatchErrorCode::GRP_STILL_USED:
      return TableOperationErrorCode::GRP_STILL_USED;
    case MatchErrorCode::EMPTY_GRP:
      return TableOperationErrorCode::EMPTY_GRP;
    case MatchErrorCode::DUPLICATE_ENTRY:
      return TableOperationErrorCode::DUPLICATE_ENTRY;
    case MatchErrorCode::BAD_MATCH_KEY:
      return TableOperationErrorCode::BAD_MATCH_KEY;
    case MatchErrorCode::ERROR:
      return TableOperationErrorCode::ERROR;
    default:
      assert(0 && "invalid error code");
    }
  }

  static void build_match_key(std::vector<MatchKeyParam> &params,
			      const BmMatchParams& match_key) {
    params.reserve(match_key.size()); // the number of elements will be the same
    for(const BmMatchParam &bm_param : match_key) {
      switch(bm_param.type) {
      case BmMatchParamType::type::EXACT:
	params.emplace_back(MatchKeyParam::Type::EXACT,
			    bm_param.exact.key);
	break;
      case BmMatchParamType::type::LPM:
	params.emplace_back(MatchKeyParam::Type::LPM,
			    bm_param.lpm.key, bm_param.lpm.prefix_length);
	break;
      case BmMatchParamType::type::TERNARY:
	params.emplace_back(MatchKeyParam::Type::TERNARY,
			    bm_param.ternary.key, bm_param.ternary.mask);
	break;
      case BmMatchParamType::type::VALID:
	params.emplace_back(MatchKeyParam::Type::VALID,
			    bm_param.valid.key ? std::string("\x01", 1) : std::string("\x00", 1));
	break;
      default:
	assert(0 && "wrong type");
      }
    }
  }

  BmEntryHandle bm_mt_add_entry(const std::string& table_name, const BmMatchParams& match_key, const std::string& action_name, const BmActionData& action_data, const BmAddEntryOptions& options) {
    printf("bm_table_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_add_entry(
        table_name,
	params,
	action_name,
	std::move(data),
	&entry_handle,
	options.priority
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_set_default_action(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_set_default_action\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_set_default_action(
        table_name,
	action_name,
	std::move(data)
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_table_delete_entry\n");
    MatchErrorCode error_code = switch_->mt_delete_entry(
        table_name,
	entry_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const std::string &action_name, const BmActionData& action_data) {
    printf("bm_table_modify_entry\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_modify_entry(
        table_name,
	entry_handle,
	action_name,
	data
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_set_entry_ttl(const std::string& table_name, const BmEntryHandle entry_handle, const int32_t timeout_ms) {
    printf("bm_mt_set_entry_ttl\n");
    MatchErrorCode error_code = switch_->mt_set_entry_ttl(
        table_name, entry_handle,
	static_cast<unsigned int>(timeout_ms)
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmMemberHandle bm_mt_indirect_add_member(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_mt_indirect_add_member\n");
    mbr_hdl_t mbr_handle;
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_indirect_add_member(
        table_name, action_name,
	std::move(data), &mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return mbr_handle;
  }

  void bm_mt_indirect_delete_member(const std::string& table_name, const BmMemberHandle mbr_handle) {
    printf("bm_mt_indirect_delete_member\n");
    MatchErrorCode error_code = switch_->mt_indirect_delete_member(
        table_name, mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_modify_member(const std::string& table_name, const BmMemberHandle mbr_handle, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_mt_indirect_modify_member\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_indirect_modify_member(
      table_name, mbr_handle, action_name, std::move(data)
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmEntryHandle bm_mt_indirect_add_entry(const std::string& table_name, const BmMatchParams& match_key, const BmMemberHandle mbr_handle, const BmAddEntryOptions& options) {
    printf("bm_mt_indirect_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    MatchErrorCode error_code = switch_->mt_indirect_add_entry(
      table_name, params, mbr_handle, &entry_handle, options.priority
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_indirect_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const BmMemberHandle mbr_handle) {
    printf("bm_mt_indirect_modify_entry\n");
    MatchErrorCode error_code = switch_->mt_indirect_modify_entry(
      table_name, entry_handle, mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_mt_indirect_delete_entry\n");
    MatchErrorCode error_code = switch_->mt_indirect_delete_entry(
      table_name, entry_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_set_entry_ttl(const std::string& table_name, const BmEntryHandle entry_handle, const int32_t timeout_ms) {
    printf("bm_mt_indirect_set_entry_ttl\n");
    MatchErrorCode error_code = switch_->mt_indirect_set_entry_ttl(
        table_name, entry_handle,
	static_cast<unsigned int>(timeout_ms)
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_set_default_member(const std::string& table_name, const BmMemberHandle mbr_handle) {
    printf("bm_mt_indirect_set_default_member\n");
    MatchErrorCode error_code = switch_->mt_indirect_set_default_member(
      table_name, mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmGroupHandle bm_mt_indirect_ws_create_group(const std::string& table_name) {
    printf("bm_mt_indirect_ws_create_group\n");
    grp_hdl_t grp_handle;
    MatchErrorCode error_code = switch_->mt_indirect_ws_create_group(
      table_name, &grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return grp_handle;
  }

  void bm_mt_indirect_ws_delete_group(const std::string& table_name, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_delete_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_delete_group(
      table_name, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_add_member_to_group(const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_add_member_to_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_add_member_to_group(
      table_name, mbr_handle, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_remove_member_from_group(const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_remove_member_from_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_remove_member_from_group(
      table_name, mbr_handle, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmEntryHandle bm_mt_indirect_ws_add_entry(const std::string& table_name, const BmMatchParams& match_key, const BmGroupHandle grp_handle, const BmAddEntryOptions& options) {
    printf("bm_mt_indirect_ws_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    MatchErrorCode error_code = switch_->mt_indirect_ws_add_entry(
      table_name, params, grp_handle, &entry_handle, options.priority
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_indirect_ws_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_modify_entry\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_modify_entry(
      table_name, entry_handle, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_set_default_group(const std::string& table_name, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_set_default_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_set_default_group(
      table_name, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_read_counter(BmCounterValue& _return, const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_mt_read_counter\n");
    MatchTable::counter_value_t bytes; // unsigned
    MatchTable::counter_value_t packets;
    MatchErrorCode error_code = switch_->mt_read_counters(
        table_name,
	entry_handle,
	&bytes, &packets
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    _return.bytes = (int64_t) bytes;
    _return.packets = (int64_t) packets;
  }

  void bm_mt_reset_counters(const std::string& table_name) {
    printf("bm_mt_reset_counters\n");
    MatchErrorCode error_code = switch_->mt_reset_counters(
        table_name
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_write_counter(const std::string& table_name, const BmEntryHandle entry_handle, const BmCounterValue& value) {
    printf("bm_mt_write_counters\n");
    MatchTable::counter_value_t bytes = (uint64_t) value.bytes;
    MatchTable::counter_value_t packets = (uint64_t) value.packets;
    MatchErrorCode error_code = switch_->mt_write_counters(
        table_name,
	entry_handle,
	bytes, packets
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_counter_read(BmCounterValue& _return, const std::string& counter_name, const int32_t index) {
    printf("bm_counter_read\n");
    MatchTable::counter_value_t bytes; // unsigned
    MatchTable::counter_value_t packets;
    Counter::CounterErrorCode error_code = switch_->read_counters(
        counter_name,
	index,
	&bytes, &packets
    );
    if(error_code != Counter::CounterErrorCode::SUCCESS) {
      InvalidCounterOperation ico;
      ico.code = (CounterOperationErrorCode::type) error_code;
      throw ico;
    }
    _return.bytes = (int64_t) bytes;
    _return.packets = (int64_t) packets;
  }

  void bm_counter_reset_all(const std::string& counter_name) {
    printf("bm_counter_reset_all\n");
    Counter::CounterErrorCode error_code = switch_->reset_counters(counter_name);
    if(error_code != Counter::CounterErrorCode::SUCCESS) {
      InvalidCounterOperation ico;
      ico.code = (CounterOperationErrorCode::type) error_code;
      throw ico;
    }
  }

  void bm_counter_write(const std::string& counter_name, const int32_t index, const BmCounterValue& value) {
    printf("bm_counter_write\n");
    MatchTable::counter_value_t bytes = (uint64_t) value.bytes;
    MatchTable::counter_value_t packets = (uint64_t) value.packets;
    Counter::CounterErrorCode error_code = switch_->write_counters(
        counter_name,
	(size_t) index,
	bytes, packets
    );
    if(error_code != Counter::CounterErrorCode::SUCCESS) {
      InvalidCounterOperation ico;
      ico.code = (CounterOperationErrorCode::type) error_code;
      throw ico;
    }
  }

  void bm_learning_ack(const BmLearningListId list_id, const BmLearningBufferId buffer_id, const std::vector<BmLearningSampleId> & sample_ids) {
    printf("bm_learning_ack\n");
    switch_->get_learn_engine()->ack(list_id, buffer_id, sample_ids);
  }

  void bm_learning_ack_buffer(const BmLearningListId list_id, const BmLearningBufferId buffer_id) {
    printf("bm_learning_ack_buffer\n");
    switch_->get_learn_engine()->ack_buffer(list_id, buffer_id);
  }

  void bm_load_new_config(const std::string& config_str) {
    printf("bm_load_new_config\n");
    RuntimeInterface::ErrorCode error_code =
      switch_->load_new_config(config_str);
    if(error_code != RuntimeInterface::SUCCESS) {
      InvalidSwapOperation iso;
      iso.code = (SwapOperationErrorCode::type) error_code;
      throw iso;
    }
  }

  void bm_swap_configs() {
    printf("bm_swap_configs\n");
    RuntimeInterface::ErrorCode error_code = switch_->swap_configs();
    if(error_code != RuntimeInterface::SUCCESS) {
      InvalidSwapOperation iso;
      iso.code = (SwapOperationErrorCode::type) error_code;
      throw iso;
    }
  }

  void bm_meter_array_set_rates(const std::string& meter_array_name, const std::vector<BmMeterRateConfig> & rates) {
    printf("bm_meter_array_set_rates\n");
    std::vector<Meter::rate_config_t> rates_;
    rates_.reserve(rates.size());
    for(const auto &rate : rates) {
      rates_.push_back(
        {rate.units_per_micros, static_cast<size_t>(rate.burst_size)}
      );
    }
    Meter::MeterErrorCode error_code =
      switch_->meter_array_set_rates(meter_array_name, rates_);
    if(error_code != Meter::MeterErrorCode::SUCCESS) {
      InvalidMeterOperation imo;
      imo.code = (MeterOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_meter_set_rates(const std::string& meter_array_name, const int32_t index, const std::vector<BmMeterRateConfig> & rates) {
    printf("bm_meter_set_rates\n");
    std::vector<Meter::rate_config_t> rates_;
    rates_.reserve(rates.size());
    for(const auto &rate : rates) {
      rates_.push_back(
        {rate.units_per_micros, static_cast<size_t>(rate.burst_size)}
      );
    }
    Meter::MeterErrorCode error_code = switch_->meter_set_rates(
      meter_array_name, static_cast<size_t>(index), rates_
    );
    if(error_code != Meter::MeterErrorCode::SUCCESS) {
      InvalidMeterOperation imo;
      imo.code = (MeterOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  BmRegisterValue bm_register_read(const std::string& register_name, const int32_t index) {
    printf("bm_register_read\n");
    Data value; // make it thread_local ?
    Register::RegisterErrorCode error_code = switch_->register_read(
      register_name, (size_t) index, &value
    );
    if(error_code != Register::RegisterErrorCode::SUCCESS) {
      InvalidRegisterOperation iro;
      iro.code = (RegisterOperationErrorCode::type) error_code;
      throw iro;
    }
    return value.get<int64_t>();
  }

  void bm_register_write(const std::string& register_name, const int32_t index, const BmRegisterValue value) {
    printf("bm_register_write\n");
    Register::RegisterErrorCode error_code = switch_->register_write(
      register_name, (size_t) index, Data(value)
    );
    if(error_code != Register::RegisterErrorCode::SUCCESS) {
      InvalidRegisterOperation iro;
      iro.code = (RegisterOperationErrorCode::type) error_code;
      throw iro;
    }
  }

  void bm_dev_mgr_add_port(const std::string& iface_name, const int32_t port_num, const std::string& pcap_path) {
    printf("bm_dev_mgr_add_port\n");
    const char *pcap = NULL;
    if(pcap_path == "") pcap = pcap_path.c_str();
    DevMgr::ReturnCode error_code;
    error_code = switch_->port_add(iface_name, port_num, pcap, pcap);
    if(error_code != DevMgr::ReturnCode::SUCCESS) {
      InvalidDevMgrOperation idmo;
      idmo.code = (DevMgrErrorCode::type) 1; // TODO
      throw idmo;
    }
  }

  void bm_dev_mgr_remove_port(const int32_t port_num) {
    printf("bm_dev_mgr_remove_port\n");
    DevMgr::ReturnCode error_code;
    error_code = switch_->port_remove(port_num);
    if(error_code != DevMgr::ReturnCode::SUCCESS) {
      InvalidDevMgrOperation idmo;
      idmo.code = (DevMgrErrorCode::type) 1; // TODO
      throw idmo;
    }
  }

  void bm_dump_table(std::string& _return, const std::string& table_name) {
    printf("dump_table\n");
    std::ostringstream stream;
    switch_->dump_table(table_name, stream);
    _return.append(stream.str());
  }

  void bm_reset_state() {
    printf("bm_reset_state\n");
    switch_->reset_state();
  }

private:
  Switch *switch_;
};

} }
