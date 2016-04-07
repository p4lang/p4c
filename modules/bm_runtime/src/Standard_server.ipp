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
#include <bm_sim/logger.h>

namespace bm_runtime { namespace standard {

using namespace bm;

typedef RuntimeInterface::mbr_hdl_t mbr_hdl_t;
typedef RuntimeInterface::grp_hdl_t grp_hdl_t;

class StandardHandler : virtual public StandardIf {
public:
  StandardHandler(SwitchWContexts *sw)
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
    case MatchErrorCode::METERS_DISABLED:
      return TableOperationErrorCode::METERS_DISABLED;
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
    case MatchErrorCode::INVALID_METER_OPERATION:
      return TableOperationErrorCode::INVALID_METER_OPERATION;
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

  BmEntryHandle bm_mt_add_entry(const int32_t cxt_id, const std::string& table_name, const BmMatchParams& match_key, const std::string& action_name, const BmActionData& action_data, const BmAddEntryOptions& options) {
    Logger::get()->trace("bm_table_add_entry");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_add_entry(
        cxt_id, table_name, params, action_name,
        std::move(data), &entry_handle, options.priority);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_set_default_action(const int32_t cxt_id, const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    Logger::get()->trace("bm_set_default_action");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_set_default_action(
        cxt_id, table_name, action_name, std::move(data));
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_delete_entry(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle) {
    Logger::get()->trace("bm_table_delete_entry");
    MatchErrorCode error_code = switch_->mt_delete_entry(
        cxt_id, table_name, entry_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_modify_entry(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const std::string &action_name, const BmActionData& action_data) {
    Logger::get()->trace("bm_table_modify_entry");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_modify_entry(
        cxt_id, table_name, entry_handle, action_name, std::move(data));
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_set_entry_ttl(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const int32_t timeout_ms) {
    Logger::get()->trace("bm_mt_set_entry_ttl");
    MatchErrorCode error_code = switch_->mt_set_entry_ttl(
        cxt_id, table_name, entry_handle,
        static_cast<unsigned int>(timeout_ms));
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmMemberHandle bm_mt_indirect_add_member(const int32_t cxt_id, const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    Logger::get()->trace("bm_mt_indirect_add_member");
    mbr_hdl_t mbr_handle;
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_indirect_add_member(
        cxt_id, table_name, action_name, std::move(data), &mbr_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return mbr_handle;
  }

  void bm_mt_indirect_delete_member(const int32_t cxt_id, const std::string& table_name, const BmMemberHandle mbr_handle) {
    Logger::get()->trace("bm_mt_indirect_delete_member");
    MatchErrorCode error_code = switch_->mt_indirect_delete_member(
        cxt_id, table_name, mbr_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_modify_member(const int32_t cxt_id, const std::string& table_name, const BmMemberHandle mbr_handle, const std::string& action_name, const BmActionData& action_data) {
    Logger::get()->trace("bm_mt_indirect_modify_member");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_indirect_modify_member(
        cxt_id, table_name, mbr_handle, action_name, std::move(data));
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmEntryHandle bm_mt_indirect_add_entry(const int32_t cxt_id, const std::string& table_name, const BmMatchParams& match_key, const BmMemberHandle mbr_handle, const BmAddEntryOptions& options) {
    Logger::get()->trace("bm_mt_indirect_add_entry");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    MatchErrorCode error_code = switch_->mt_indirect_add_entry(
        cxt_id, table_name, params, mbr_handle,
        &entry_handle, options.priority);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_indirect_modify_entry(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const BmMemberHandle mbr_handle) {
    Logger::get()->trace("bm_mt_indirect_modify_entry");
    MatchErrorCode error_code = switch_->mt_indirect_modify_entry(
        cxt_id, table_name, entry_handle, mbr_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_delete_entry(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle) {
    Logger::get()->trace("bm_mt_indirect_delete_entry");
    MatchErrorCode error_code = switch_->mt_indirect_delete_entry(
        cxt_id, table_name, entry_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_set_entry_ttl(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const int32_t timeout_ms) {
    Logger::get()->trace("bm_mt_indirect_set_entry_ttl");
    MatchErrorCode error_code = switch_->mt_indirect_set_entry_ttl(
        cxt_id, table_name, entry_handle,
        static_cast<unsigned int>(timeout_ms));
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_set_default_member(const int32_t cxt_id, const std::string& table_name, const BmMemberHandle mbr_handle) {
    Logger::get()->trace("bm_mt_indirect_set_default_member");
    MatchErrorCode error_code = switch_->mt_indirect_set_default_member(
        cxt_id, table_name, mbr_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmGroupHandle bm_mt_indirect_ws_create_group(const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("bm_mt_indirect_ws_create_group");
    grp_hdl_t grp_handle;
    MatchErrorCode error_code = switch_->mt_indirect_ws_create_group(
        cxt_id, table_name, &grp_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return grp_handle;
  }

  void bm_mt_indirect_ws_delete_group(const int32_t cxt_id, const std::string& table_name, const BmGroupHandle grp_handle) {
    Logger::get()->trace("bm_mt_indirect_ws_delete_group");
    MatchErrorCode error_code = switch_->mt_indirect_ws_delete_group(
        cxt_id, table_name, grp_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_add_member_to_group(const int32_t cxt_id, const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    Logger::get()->trace("bm_mt_indirect_ws_add_member_to_group");
    MatchErrorCode error_code = switch_->mt_indirect_ws_add_member_to_group(
        cxt_id, table_name, mbr_handle, grp_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_remove_member_from_group(const int32_t cxt_id, const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    Logger::get()->trace("bm_mt_indirect_ws_remove_member_from_group");
    MatchErrorCode error_code = switch_->mt_indirect_ws_remove_member_from_group(
        cxt_id, table_name, mbr_handle, grp_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  BmEntryHandle bm_mt_indirect_ws_add_entry(const int32_t cxt_id, const std::string& table_name, const BmMatchParams& match_key, const BmGroupHandle grp_handle, const BmAddEntryOptions& options) {
    Logger::get()->trace("bm_mt_indirect_ws_add_entry");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    MatchErrorCode error_code = switch_->mt_indirect_ws_add_entry(
        cxt_id, table_name, params, grp_handle,
        &entry_handle, options.priority);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_indirect_ws_modify_entry(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const BmGroupHandle grp_handle) {
    Logger::get()->trace("bm_mt_indirect_ws_modify_entry");
    MatchErrorCode error_code = switch_->mt_indirect_ws_modify_entry(
        cxt_id, table_name, entry_handle, grp_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_set_default_group(const int32_t cxt_id, const std::string& table_name, const BmGroupHandle grp_handle) {
    Logger::get()->trace("bm_mt_indirect_ws_set_default_group");
    MatchErrorCode error_code = switch_->mt_indirect_ws_set_default_group(
        cxt_id, table_name, grp_handle);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_read_counter(BmCounterValue& _return, const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle) {
    Logger::get()->trace("bm_mt_read_counter");
    MatchTable::counter_value_t bytes; // unsigned
    MatchTable::counter_value_t packets;
    MatchErrorCode error_code = switch_->mt_read_counters(
        cxt_id, table_name, entry_handle, &bytes, &packets);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    _return.bytes = (int64_t) bytes;
    _return.packets = (int64_t) packets;
  }

  void bm_mt_reset_counters(const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("bm_mt_reset_counters");
    MatchErrorCode error_code = switch_->mt_reset_counters(
        cxt_id, table_name);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_write_counter(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const BmCounterValue& value) {
    Logger::get()->trace("bm_mt_write_counters");
    MatchTable::counter_value_t bytes = (uint64_t) value.bytes;
    MatchTable::counter_value_t packets = (uint64_t) value.packets;
    MatchErrorCode error_code = switch_->mt_write_counters(
        cxt_id, table_name, entry_handle, bytes, packets);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_set_meter_rates(const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle, const std::vector<BmMeterRateConfig> & rates) {
    Logger::get()->trace("bm_mt_set_meter_rates");
    std::vector<Meter::rate_config_t> rates_;
    rates_.reserve(rates.size());
    for(const auto &rate : rates) {
      rates_.push_back(
        {rate.units_per_micros, static_cast<size_t>(rate.burst_size)}
      );
    }
    MatchErrorCode error_code = switch_->mt_set_meter_rates(
        cxt_id, table_name, entry_handle, rates_);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_counter_read(BmCounterValue& _return, const int32_t cxt_id, const std::string& counter_name, const int32_t index) {
    Logger::get()->trace("bm_counter_read");
    MatchTable::counter_value_t bytes; // unsigned
    MatchTable::counter_value_t packets;
    Counter::CounterErrorCode error_code = switch_->read_counters(
        cxt_id, counter_name, index, &bytes, &packets);
    if(error_code != Counter::CounterErrorCode::SUCCESS) {
      InvalidCounterOperation ico;
      ico.code = (CounterOperationErrorCode::type) error_code;
      throw ico;
    }
    _return.bytes = (int64_t) bytes;
    _return.packets = (int64_t) packets;
  }

  void bm_counter_reset_all(const int32_t cxt_id, const std::string& counter_name) {
    Logger::get()->trace("bm_counter_reset_all");
    Counter::CounterErrorCode error_code = switch_->reset_counters(
        cxt_id, counter_name);
    if(error_code != Counter::CounterErrorCode::SUCCESS) {
      InvalidCounterOperation ico;
      ico.code = (CounterOperationErrorCode::type) error_code;
      throw ico;
    }
  }

  void bm_counter_write(const int32_t cxt_id, const std::string& counter_name, const int32_t index, const BmCounterValue& value) {
    Logger::get()->trace("bm_counter_write");
    MatchTable::counter_value_t bytes = (uint64_t) value.bytes;
    MatchTable::counter_value_t packets = (uint64_t) value.packets;
    Counter::CounterErrorCode error_code = switch_->write_counters(
        cxt_id, counter_name, (size_t) index, bytes, packets);
    if(error_code != Counter::CounterErrorCode::SUCCESS) {
      InvalidCounterOperation ico;
      ico.code = (CounterOperationErrorCode::type) error_code;
      throw ico;
    }
  }

  void bm_learning_ack(const int32_t cxt_id, const BmLearningListId list_id, const BmLearningBufferId buffer_id, const std::vector<BmLearningSampleId> & sample_ids) {
    Logger::get()->trace("bm_learning_ack");
    LearnEngine::LearnErrorCode error_code =
        switch_->get_learn_engine(cxt_id)->ack(list_id, buffer_id, sample_ids);
    if (error_code != LearnEngine::LearnErrorCode::SUCCESS) {
      InvalidLearnOperation ilo;
      ilo.code = (LearnOperationErrorCode::type) error_code;
      throw ilo;
    }
  }

  void bm_learning_ack_buffer(const int32_t cxt_id, const BmLearningListId list_id, const BmLearningBufferId buffer_id) {
    Logger::get()->trace("bm_learning_ack_buffer");
    LearnEngine::LearnErrorCode error_code =
        switch_->get_learn_engine(cxt_id)->ack_buffer(list_id, buffer_id);
    if (error_code != LearnEngine::LearnErrorCode::SUCCESS) {
      InvalidLearnOperation ilo;
      ilo.code = (LearnOperationErrorCode::type) error_code;
      throw ilo;
    }
  }

  void bm_learning_set_timeout(const int32_t cxt_id, const BmLearningListId list_id, const int32_t timeout_ms) {
    Logger::get()->trace("bm_learning_set_timeout");
    LearnEngine::LearnErrorCode error_code =
        switch_->get_learn_engine(cxt_id)->list_set_timeout(
            list_id, timeout_ms);
    if (error_code != LearnEngine::LearnErrorCode::SUCCESS) {
      InvalidLearnOperation ilo;
      ilo.code = (LearnOperationErrorCode::type) error_code;
      throw ilo;
    }
  }

  void bm_learning_set_buffer_size(const int32_t cxt_id, const BmLearningListId list_id, const int32_t nb_samples) {
    Logger::get()->trace("bm_learning_set_buffer_size");
    LearnEngine::LearnErrorCode error_code =
        switch_->get_learn_engine(cxt_id)->list_set_max_samples(
            list_id, nb_samples);
    if (error_code != LearnEngine::LearnErrorCode::SUCCESS) {
      InvalidLearnOperation ilo;
      ilo.code = (LearnOperationErrorCode::type) error_code;
      throw ilo;
    }
  }

  void bm_load_new_config(const std::string& config_str) {
    Logger::get()->trace("bm_load_new_config");
    RuntimeInterface::ErrorCode error_code =
      switch_->load_new_config(config_str);
    if(error_code != RuntimeInterface::ErrorCode::SUCCESS) {
      InvalidSwapOperation iso;
      iso.code = (SwapOperationErrorCode::type) error_code;
      throw iso;
    }
  }

  void bm_swap_configs() {
    Logger::get()->trace("bm_swap_configs");
    RuntimeInterface::ErrorCode error_code = switch_->swap_configs();
    if(error_code != RuntimeInterface::ErrorCode::SUCCESS) {
      InvalidSwapOperation iso;
      iso.code = (SwapOperationErrorCode::type) error_code;
      throw iso;
    }
  }

  void bm_meter_array_set_rates(const int32_t cxt_id, const std::string& meter_array_name, const std::vector<BmMeterRateConfig> & rates) {
    Logger::get()->trace("bm_meter_array_set_rates");
    std::vector<Meter::rate_config_t> rates_;
    rates_.reserve(rates.size());
    for(const auto &rate : rates) {
      rates_.push_back(
        {rate.units_per_micros, static_cast<size_t>(rate.burst_size)}
      );
    }
    Meter::MeterErrorCode error_code = switch_->meter_array_set_rates(
        cxt_id, meter_array_name, rates_);
    if(error_code != Meter::MeterErrorCode::SUCCESS) {
      InvalidMeterOperation imo;
      imo.code = (MeterOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_meter_set_rates(const int32_t cxt_id, const std::string& meter_array_name, const int32_t index, const std::vector<BmMeterRateConfig> & rates) {
    Logger::get()->trace("bm_meter_set_rates");
    std::vector<Meter::rate_config_t> rates_;
    rates_.reserve(rates.size());
    for(const auto &rate : rates) {
      rates_.push_back(
        {rate.units_per_micros, static_cast<size_t>(rate.burst_size)}
      );
    }
    Meter::MeterErrorCode error_code = switch_->meter_set_rates(
        cxt_id, meter_array_name, static_cast<size_t>(index), rates_
    );
    if(error_code != Meter::MeterErrorCode::SUCCESS) {
      InvalidMeterOperation imo;
      imo.code = (MeterOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  BmRegisterValue bm_register_read(const int32_t cxt_id, const std::string& register_array_name, const int32_t index) {
    Logger::get()->trace("bm_register_read");
    Data value; // make it thread_local ?
    Register::RegisterErrorCode error_code = switch_->register_read(
        cxt_id, register_array_name, static_cast<size_t>(index), &value);
    if(error_code != Register::RegisterErrorCode::SUCCESS) {
      InvalidRegisterOperation iro;
      iro.code = (RegisterOperationErrorCode::type) error_code;
      throw iro;
    }
    return value.get<int64_t>();
  }

  void bm_register_write(const int32_t cxt_id, const std::string& register_array_name, const int32_t index, const BmRegisterValue value) {
    Logger::get()->trace("bm_register_write");
    Register::RegisterErrorCode error_code = switch_->register_write(
        cxt_id, register_array_name, static_cast<size_t>(index), Data(value));
    if(error_code != Register::RegisterErrorCode::SUCCESS) {
      InvalidRegisterOperation iro;
      iro.code = (RegisterOperationErrorCode::type) error_code;
      throw iro;
    }
  }

  void bm_register_write_range(const int32_t cxt_id, const std::string& register_array_name, const int32_t start_index, const int32_t end_index, const BmRegisterValue value) {
    Logger::get()->trace("bm_register_write_range");
    Register::RegisterErrorCode error_code = switch_->register_write_range(
        cxt_id, register_array_name, static_cast<size_t>(start_index),
        static_cast<size_t>(end_index), Data(value));
    if(error_code != Register::RegisterErrorCode::SUCCESS) {
      InvalidRegisterOperation iro;
      iro.code = (RegisterOperationErrorCode::type) error_code;
      throw iro;
    }
  }

  void bm_register_reset(const int32_t cxt_id, const std::string& register_array_name) {
    Logger::get()->trace("bm_register_reset");
    Register::RegisterErrorCode error_code = switch_->register_reset(
        cxt_id, register_array_name);
    if(error_code != Register::RegisterErrorCode::SUCCESS) {
      InvalidRegisterOperation iro;
      iro.code = (RegisterOperationErrorCode::type) error_code;
      throw iro;
    }
  }

  void bm_dev_mgr_add_port(const std::string& iface_name, const int32_t port_num, const std::string& pcap_path) {
    Logger::get()->trace("bm_dev_mgr_add_port");
    const char *pcap = NULL;
    if(pcap_path != "") pcap = pcap_path.c_str();
    DevMgr::ReturnCode error_code;
    error_code = switch_->port_add(iface_name, port_num, pcap, pcap);
    if(error_code != DevMgr::ReturnCode::SUCCESS) {
      InvalidDevMgrOperation idmo;
      idmo.code = (DevMgrErrorCode::type) 1; // TODO
      throw idmo;
    }
  }

  void bm_dev_mgr_remove_port(const int32_t port_num) {
    Logger::get()->trace("bm_dev_mgr_remove_port");
    DevMgr::ReturnCode error_code;
    error_code = switch_->port_remove(port_num);
    if(error_code != DevMgr::ReturnCode::SUCCESS) {
      InvalidDevMgrOperation idmo;
      idmo.code = (DevMgrErrorCode::type) 1; // TODO
      throw idmo;
    }
  }

  void bm_dev_mgr_show_ports(std::vector<DevMgrPortInfo> & _return) {
    Logger::get()->trace("bm_dev_mgr_show_ports");
    _return.clear();
    for (auto &e : switch_->get_port_info()) {
      DevMgrIface::PortInfo &p_i = e.second;
      DevMgrPortInfo p_i_;
      p_i_.port_num = p_i.port_num;
      p_i_.iface_name = std::move(p_i.iface_name);
      p_i_.is_up = p_i.is_up;
      p_i_.extra = std::move(p_i.extra);
      _return.push_back(std::move(p_i_));
    }
  }

  void bm_dump_table(std::string& _return, const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("dump_table");
    std::ostringstream stream;
    switch_->dump_table(cxt_id, table_name, &stream);
    _return.append(stream.str());
  }

  void bm_reset_state() {
    Logger::get()->trace("bm_reset_state");
    switch_->reset_state();
  }

  void bm_get_config(std::string& _return) {
    Logger::get()->trace("bm_get_config");
    _return.append(switch_->get_config());
  }

  void bm_get_config_md5(std::string& _return) {
    Logger::get()->trace("bm_get_config_md5");
    _return.append(switch_->get_config_md5());
  }

private:
  SwitchWContexts *switch_;
};

} }
