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

#include <bm/Standard.h>

#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/logger.h>

#include <functional>

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
    case MatchErrorCode::DEFAULT_ACTION_IS_CONST:
      return TableOperationErrorCode::DEFAULT_ACTION_IS_CONST;
    case MatchErrorCode::DEFAULT_ENTRY_IS_CONST:
      return TableOperationErrorCode::DEFAULT_ENTRY_IS_CONST;
      case MatchErrorCode::NO_DEFAULT_ENTRY:
      return TableOperationErrorCode::NO_DEFAULT_ENTRY;
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
      case BmMatchParamType::type::RANGE:
        params.emplace_back(MatchKeyParam::Type::RANGE,
                            bm_param.range.start, bm_param.range.end_);
        break;
      default:
        assert(0 && "wrong type");
      }
    }
  }

  static void retrieve_match_key(BmMatchParams& match_key,
                                 const std::vector<MatchKeyParam> &params) {
    match_key.reserve(params.size()); // the number of elements will be the same
    for(const MatchKeyParam &param : params) {
      switch(param.type) {
        case MatchKeyParam::Type::EXACT:
          {
            BmMatchParamExact bm_param_exact;
            bm_param_exact.key = param.key;
            BmMatchParam bm_param;
            bm_param.type = BmMatchParamType::type::EXACT;
            bm_param.__set_exact(bm_param_exact);
            match_key.push_back(std::move(bm_param));
            break;
          }
        case MatchKeyParam::Type::LPM:
          {
            BmMatchParamLPM bm_param_lpm;
            bm_param_lpm.key = param.key;
            bm_param_lpm.prefix_length = param.prefix_length;
            BmMatchParam bm_param;
            bm_param.type = BmMatchParamType::type::LPM;
            bm_param.__set_lpm(bm_param_lpm);
            match_key.push_back(std::move(bm_param));
            break;
          }
        case MatchKeyParam::Type::TERNARY:
          {
            BmMatchParamTernary bm_param_ternary;
            bm_param_ternary.key = param.key;
            bm_param_ternary.mask = param.mask;
            BmMatchParam bm_param;
            bm_param.type = BmMatchParamType::type::TERNARY;
            bm_param.__set_ternary(bm_param_ternary);
            match_key.push_back(std::move(bm_param));
            break;
          }
        case MatchKeyParam::Type::VALID:
          {
            BmMatchParamValid bm_param_valid;
            bm_param_valid.key = (param.key == std::string("\x01", 1));
            BmMatchParam bm_param;
            bm_param.type = BmMatchParamType::type::VALID;
            bm_param.__set_valid(bm_param_valid);
            match_key.push_back(std::move(bm_param));
            break;
          }
        case MatchKeyParam::Type::RANGE:
          {
            BmMatchParamRange bm_param_range;
            bm_param_range.start = param.key;
            bm_param_range.end_ = param.mask;
            BmMatchParam bm_param;
            bm_param.type = BmMatchParamType::type::RANGE;
            bm_param.__set_range(bm_param_range);
            match_key.push_back(std::move(bm_param));
            break;
          }
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

  void bm_mt_get_meter_rates(std::vector<BmMeterRateConfig> & _return, const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle) {
    Logger::get()->trace("bm_mt_get_meter_rates");
    std::vector<Meter::rate_config_t> rates;
    MatchErrorCode error_code = switch_->mt_get_meter_rates(
        cxt_id, table_name, entry_handle, &rates);
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    }
    _return.resize(rates.size());
    for (size_t i = 0; i < rates.size(); i++) {
      _return[i].units_per_micros = rates[i].info_rate;
      _return[i].burst_size = rates[i].burst_size;
    }
  }

  template <typename E>
  void copy_match_part_entry(BmMtEntry *e, const E &entry) {
    retrieve_match_key(e->match_key, entry.match_key);
    e->entry_handle = entry.handle;
    BmAddEntryOptions options;
    options.__set_priority(entry.priority);
    e->options = options;
  }

  template <typename E>
  void build_action_entry(BmActionEntry *action_e, const E &entry);

  template <typename M,
            typename std::vector<typename M::Entry> (RuntimeInterface::*GetFn)(
                size_t, const std::string &) const>
  void get_entries_common(size_t cxt_id, const std::string &table_name,
                          std::vector<BmMtEntry> &_return) {
    const auto entries = std::bind(GetFn, switch_, cxt_id, table_name)();
    for (const auto &entry : entries) {
      BmMtEntry e;
      copy_match_part_entry(&e, entry);
      build_action_entry(&e.action_entry, entry);
      _return.push_back(std::move(e));
    }
  }

  template <typename M,
            MatchErrorCode (RuntimeInterface::*GetFn)(
                size_t, const std::string &, entry_handle_t, typename M::Entry *) const>
  void get_entry_common(size_t cxt_id, const std::string &table_name,
                        BmEntryHandle handle, BmMtEntry &e) {
    typename M::Entry entry;
    auto rc = std::bind(GetFn, switch_, cxt_id, table_name, handle, &entry)();
    if(rc != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(rc);
      throw ito;
    }
    copy_match_part_entry(&e, entry);
    build_action_entry(&e.action_entry, entry);
  }

  void bm_mt_get_entries(std::vector<BmMtEntry> & _return, const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("bm_mt_get_entries");
    switch (switch_->mt_get_type(cxt_id, table_name)) {
      case MatchTableType::NONE:
        return;
      case MatchTableType::SIMPLE:
        get_entries_common<MatchTable, &RuntimeInterface::mt_get_entries>(
            cxt_id, table_name, _return);
        break;
      case MatchTableType::INDIRECT:
        get_entries_common<MatchTableIndirect,
                           &RuntimeInterface::mt_indirect_get_entries>(
                               cxt_id, table_name, _return);
        break;
      case MatchTableType::INDIRECT_WS:
        get_entries_common<MatchTableIndirectWS,
                           &RuntimeInterface::mt_indirect_ws_get_entries>(
                               cxt_id, table_name, _return);
        break;
    }
  }

  void bm_mt_get_entry(BmMtEntry& _return, const int32_t cxt_id, const std::string& table_name, const BmEntryHandle entry_handle) {
    Logger::get()->trace("bm_mt_get_entry");
    switch (switch_->mt_get_type(cxt_id, table_name)) {
      case MatchTableType::NONE:
        return;
      case MatchTableType::SIMPLE:
        get_entry_common<MatchTable, &RuntimeInterface::mt_get_entry>(
            cxt_id, table_name, entry_handle,  _return);
        break;
      case MatchTableType::INDIRECT:
        get_entry_common<MatchTableIndirect,
                           &RuntimeInterface::mt_indirect_get_entry>(
                               cxt_id, table_name, entry_handle, _return);
        break;
      case MatchTableType::INDIRECT_WS:
        get_entry_common<MatchTableIndirectWS,
                           &RuntimeInterface::mt_indirect_ws_get_entry>(
                               cxt_id, table_name, entry_handle, _return);
        break;
    }
  }

  template <typename M,
            MatchErrorCode (RuntimeInterface::*GetFn)(
                size_t, const std::string &, typename M::Entry *) const>
  typename M::Entry get_default_entry_common(
      size_t cxt_id, const std::string &table_name, BmActionEntry &e) {
    typename M::Entry entry;
    auto error_code = std::bind(GetFn, switch_, cxt_id, table_name, &entry)();
    if (error_code == MatchErrorCode::NO_DEFAULT_ENTRY) {
      e.action_type = BmActionEntryType::NONE;
    } else if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(error_code);
      throw ito;
    } else {
      build_action_entry(&e, entry);
    }
    return entry;
  }

  void bm_mt_get_default_entry(BmActionEntry& _return, const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("bm_mt_get_default_entry");
    BmActionEntry e;
    switch (switch_->mt_get_type(cxt_id, table_name)) {
      case MatchTableType::NONE:
        return;
      case MatchTableType::SIMPLE:
        get_default_entry_common<
          MatchTable, &RuntimeInterface::mt_get_default_entry>(
              cxt_id, table_name, _return);
        break;
      case MatchTableType::INDIRECT:
        get_default_entry_common<
          MatchTableIndirect, &RuntimeInterface::mt_indirect_get_default_entry>(
              cxt_id, table_name, _return);
        break;
      case MatchTableType::INDIRECT_WS:
        get_default_entry_common<
          MatchTableIndirectWS,
          &RuntimeInterface::mt_indirect_ws_get_default_entry>(
              cxt_id, table_name, _return);
        break;
    }
  }

  void copy_one_member(BmMtIndirectMember *m,
                       const MatchTableIndirect::Member &from) const;

  void bm_mt_indirect_get_members(std::vector<BmMtIndirectMember> & _return, const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("bm_mt_indirect_get_members");
    const auto members = switch_->mt_indirect_get_members(cxt_id, table_name);
    for (const auto &member : members) {
      BmMtIndirectMember m;
      copy_one_member(&m, member);
      _return.push_back(std::move(m));
    }
  }

  void bm_mt_indirect_get_member(BmMtIndirectMember& _return, const int32_t cxt_id, const std::string& table_name, const BmMemberHandle mbr_handle) {
    Logger::get()->trace("bm_mt_indirect_get_member");
    MatchTableIndirect::Member member;
    auto rc = switch_->mt_indirect_get_member(cxt_id, table_name, mbr_handle,
                                              &member);
    if(rc != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(rc);
      throw ito;
    }
    copy_one_member(&_return, member);
  }

  void copy_one_group(BmMtIndirectWsGroup *g,
                      const MatchTableIndirectWS::Group &from) const;

  void bm_mt_indirect_ws_get_groups(std::vector<BmMtIndirectWsGroup> & _return, const int32_t cxt_id, const std::string& table_name) {
    Logger::get()->trace("bm_mt_indirect_ws_get_groups");
    const auto groups = switch_->mt_indirect_ws_get_groups(cxt_id, table_name);
    for (const auto &group : groups) {
      BmMtIndirectWsGroup g;
      copy_one_group(&g, group);
      _return.push_back(std::move(g));
    }
  }

  void bm_mt_indirect_ws_get_group(BmMtIndirectWsGroup& _return, const int32_t cxt_id, const std::string& table_name, const BmGroupHandle grp_handle) {
    Logger::get()->trace("bm_mt_indirect_ws_get_group");
    MatchTableIndirectWS::Group group;
    auto rc = switch_->mt_indirect_ws_get_group(cxt_id, table_name, grp_handle,
                                                &group);
    if(rc != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.code = get_exception_code(rc);
      throw ito;
    }
    copy_one_group(&_return, group);
  }

  void bm_counter_read(BmCounterValue& _return, const int32_t cxt_id, const std::string& counter_name, const int32_t index) {
    Logger::get()->trace("bm_counter_read");
    MatchTable::counter_value_t bytes;  // unsigned
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

  void bm_meter_get_rates(std::vector<BmMeterRateConfig> & _return, const int32_t cxt_id, const std::string& meter_array_name, const int32_t index) {
    Logger::get()->trace("bm_meter_get_rates");
    std::vector<Meter::rate_config_t> rates;
    Meter::MeterErrorCode error_code = switch_->meter_get_rates(
        cxt_id, meter_array_name, index, &rates);
    if(error_code != Meter::MeterErrorCode::SUCCESS) {
      InvalidMeterOperation imo;
      imo.code = (MeterOperationErrorCode::type) error_code;
      throw imo;
    }
    _return.resize(rates.size());
    for (size_t i = 0; i < rates.size(); i++) {
      _return[i].units_per_micros = rates[i].info_rate;
      _return[i].burst_size = rates[i].burst_size;
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

  void bm_parse_vset_add(const int32_t cxt_id, const std::string& parse_vset_name, const BmParseVSetValue& value) {
    Logger::get()->trace("bm_parse_vset_add");
    ParseVSet::ErrorCode error_code = switch_->parse_vset_add(
        cxt_id, parse_vset_name, ByteContainer(value.data(), value.size()));
    if(error_code != ParseVSet::ErrorCode::SUCCESS) {
      InvalidParseVSetOperation ipo;
      ipo.code = (ParseVSetOperationErrorCode::type) error_code;
      throw ipo;
    }
  }

  void bm_parse_vset_remove(const int32_t cxt_id, const std::string& parse_vset_name, const BmParseVSetValue& value) {
    Logger::get()->trace("bm_parse_vset_remove");
    ParseVSet::ErrorCode error_code = switch_->parse_vset_remove(
        cxt_id, parse_vset_name, ByteContainer(value.data(), value.size()));
    if(error_code != ParseVSet::ErrorCode::SUCCESS) {
      InvalidParseVSetOperation ipo;
      ipo.code = (ParseVSetOperationErrorCode::type) error_code;
      throw ipo;
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

  void bm_mgmt_get_info(BmConfig& _return) {
    Logger::get()->trace("bm_mgmt_get_info");
    _return.device_id = switch_->get_device_id();
    _return.thrift_port = switch_->get_runtime_port();
    _return.__set_notifications_socket(switch_->get_notifications_addr());
    {
      std::string s = switch_->get_event_logger_addr();
      if (s != "") _return.__set_elogger_socket(s);
    }
    {
      std::string s = switch_->get_debugger_addr();
      if (s != "") _return.__set_debugger_socket(s);
    }
  }

  void bm_set_crc16_custom_parameters(const int32_t cxt_id, const std::string& calc_name, const BmCrc16Config& crc16_config) {
    Logger::get()->trace("bm_set_crc16_custom_parameters");
    CustomCrcMgr<uint16_t>::crc_config_t c;
    c.polynomial = static_cast<uint16_t>(crc16_config.polynomial);
    c.initial_remainder = static_cast<uint16_t>(crc16_config.initial_remainder);
    c.final_xor_value = static_cast<uint16_t>(crc16_config.final_xor_value);
    c.data_reflected = crc16_config.data_reflected;
    c.remainder_reflected = crc16_config.remainder_reflected;
    auto rc = switch_->set_crc16_custom_parameters(cxt_id, calc_name, c);
    if(rc != CustomCrcErrorCode::SUCCESS) {
      InvalidCrcOperation ico;
      ico.code = static_cast<CrcErrorCode::type>(rc); // TODO
      throw ico;
    }
  }

  void bm_set_crc32_custom_parameters(const int32_t cxt_id, const std::string& calc_name, const BmCrc32Config& crc32_config) {
    Logger::get()->trace("bm_set_crc32_custom_parameters");
    CustomCrcMgr<uint32_t>::crc_config_t c;
    c.polynomial = static_cast<uint32_t>(crc32_config.polynomial);
    c.initial_remainder = static_cast<uint32_t>(crc32_config.initial_remainder);
    c.final_xor_value = static_cast<uint32_t>(crc32_config.final_xor_value);
    c.data_reflected = crc32_config.data_reflected;
    c.remainder_reflected = crc32_config.remainder_reflected;
    auto rc = switch_->set_crc32_custom_parameters(cxt_id, calc_name, c);
    if(rc != CustomCrcErrorCode::SUCCESS) {
      InvalidCrcOperation ico;
      ico.code = static_cast<CrcErrorCode::type>(rc); // TODO
      throw ico;
    }
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

  void bm_serialize_state(std::string& _return) {
    Logger::get()->trace("bm_serialize_state");
    std::ostringstream stream;
    switch_->serialize(&stream);
    _return.append(stream.str());
  }

private:
  SwitchWContexts *switch_;
};

template <>
void
StandardHandler::build_action_entry<MatchTable::Entry>(
    BmActionEntry *action_e, const MatchTable::Entry &entry) {
  BmActionData action_data;
  for (const Data &d : entry.action_data.action_data) {
    action_data.push_back(d.get_string());
  }
  action_e->action_type = BmActionEntryType::ACTION_DATA;
  action_e->__set_action_name(entry.action_fn->get_name());
  action_e->__set_action_data(std::move(action_data));
}

template <>
void
StandardHandler::build_action_entry<MatchTableIndirect::Entry>(
    BmActionEntry *action_e, const MatchTableIndirect::Entry &entry) {
  action_e->action_type = BmActionEntryType::MBR_HANDLE;
  action_e->__set_mbr_handle(entry.mbr);
}

template <>
void
StandardHandler::build_action_entry<MatchTableIndirectWS::Entry>(
    BmActionEntry *action_e, const MatchTableIndirectWS::Entry &entry) {
  if (entry.mbr < entry.grp) {
    action_e->action_type = BmActionEntryType::MBR_HANDLE;
    action_e->__set_mbr_handle(entry.mbr);
  } else {
    action_e->action_type = BmActionEntryType::GRP_HANDLE;
    action_e->__set_grp_handle(entry.grp);
  }
}

void StandardHandler::copy_one_member(
    BmMtIndirectMember *m, const MatchTableIndirect::Member &from) const{
  m->mbr_handle = from.mbr;
  m->action_name = from.action_fn->get_name();
  BmActionData action_data;
  for (const Data &d : from.action_data.action_data) {
    action_data.push_back(d.get_string());
  }
  m->action_data = std::move(action_data);
}

void StandardHandler::copy_one_group(
    BmMtIndirectWsGroup *g, const MatchTableIndirectWS::Group &from) const {
  g->grp_handle = from.grp;
  g->mbr_handles.reserve(from.mbr_handles.size());
  for (const auto h : from.mbr_handles) g->mbr_handles.push_back(h);
}

boost::shared_ptr<StandardIf> get_handler(SwitchWContexts *switch_) {
  return boost::shared_ptr<StandardHandler>(new StandardHandler(switch_));
}

}  // namespace standard
}  // namespace bm_runtime
