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

namespace cpp bm_runtime.standard
namespace py bm_runtime.standard

typedef i64 BmEntryHandle
typedef list<binary> BmActionData

typedef i32 BmMemberHandle
typedef i32 BmGroupHandle

typedef i32 BmLearningListId
typedef i64 BmLearningBufferId
typedef i32 BmLearningSampleId

enum BmMatchParamType {
  EXACT = 0,
  LPM = 1,
  TERNARY = 2,
  VALID = 3
}

struct BmMatchParamExact {
  1:binary key
}

struct BmMatchParamLPM {
  1:binary key,
  2:i32 prefix_length
}

struct BmMatchParamTernary {
  1:binary key,
  2:binary mask
}

struct BmMatchParamValid {
  1:bool key
}

# Thrift union sucks in C++, the following is much better
struct BmMatchParam {
  1:BmMatchParamType type,
  2:optional BmMatchParamExact exact,
  3:optional BmMatchParamLPM lpm,
  4:optional BmMatchParamTernary ternary,
  5:optional BmMatchParamValid valid
}

typedef list<BmMatchParam> BmMatchParams

struct BmAddEntryOptions {
  1:optional i32 priority
}

struct BmCounterValue {
  1:i64 bytes;
  2:i64 packets;
}

struct BmMeterRateConfig {
  1:double units_per_micros;
  2:i32 burst_size;
}

enum TableOperationErrorCode {
  TABLE_FULL = 1,
  INVALID_HANDLE = 2,
  EXPIRED_HANDLE = 3,
  COUNTERS_DISABLED = 4,
  AGEING_DISABLED = 5,
  INVALID_TABLE_NAME = 6,
  INVALID_ACTION_NAME = 7,
  WRONG_TABLE_TYPE = 8,
  INVALID_MBR_HANDLE = 9,
  MBR_STILL_USED = 10,
  MBR_ALREADY_IN_GRP = 11,
  MBR_NOT_IN_GRP = 12,
  INVALID_GRP_HANDLE = 13,
  GRP_STILL_USED = 14,
  EMPTY_GRP = 15,
  DUPLICATE_ENTRY = 16,
  BAD_MATCH_KEY = 17,
  ERROR = 18,
}

exception InvalidTableOperation {
  1:TableOperationErrorCode code
}

enum CounterOperationErrorCode {
  INVALID_COUNTER_NAME = 1,
  INVALID_INDEX = 2,
  ERROR = 3,
}

exception InvalidCounterOperation {
  1:CounterOperationErrorCode code
}

enum SwapOperationErrorCode {
  CONFIG_SWAP_DISABLED = 1,
  ONGOING_SWAP = 2,
  NO_ONGOING_SWAP = 3
}

exception InvalidSwapOperation {
  1:SwapOperationErrorCode code
}

enum MeterOperationErrorCode {
  INVALID_INDEX = 1,
  BAD_RATES_LIST = 2,
  INVALID_INFO_RATE_VALUE = 3,
  INVALID_BURST_SIZE_VALUE = 4,
  ERROR = 5
}

exception InvalidMeterOperation {
 1:MeterOperationErrorCode code
}

typedef i64 BmRegisterValue

enum RegisterOperationErrorCode {
  INVALID_INDEX = 1,
  ERROR = 2
}

exception InvalidRegisterOperation {
 1:RegisterOperationErrorCode code
}

// TODO
enum DevMgrErrorCode {
  ERROR = 1
}

exception InvalidDevMgrOperation {
 1:DevMgrErrorCode code
}

service Standard {
	
  // table operations

  BmEntryHandle bm_mt_add_entry(
    1:string table_name,
    2:BmMatchParams match_key,
    3:string action_name,
    4:BmActionData action_data,
    5:BmAddEntryOptions options
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_set_default_action(
    1:string table_name,
    2:string action_name,
    3:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_delete_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_modify_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle,
    3:string action_name,
    4:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_set_entry_ttl(
    1:string table_name
    2:BmEntryHandle entry_handle,
    3:i32 timeout_ms
  ) throws (1:InvalidTableOperation ouch),

  // indirect tables

  BmMemberHandle bm_mt_indirect_add_member(
    1:string table_name,
    2:string action_name,
    3:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_delete_member(
    1:string table_name,
    2:BmMemberHandle mbr_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_modify_member(
    1:string table_name,
    2:BmMemberHandle mbr_handle,
    3:string action_name,
    4:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  BmEntryHandle bm_mt_indirect_add_entry(
    1:string table_name,
    2:BmMatchParams match_key,
    3:BmMemberHandle mbr_handle,
    4:BmAddEntryOptions options
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_modify_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle,
    3:BmMemberHandle mbr_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_delete_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_set_entry_ttl(
    1:string table_name
    2:BmEntryHandle entry_handle,
    3:i32 timeout_ms
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_set_default_member(
    1:string table_name,
    2:BmMemberHandle mbr_handle
  ) throws (1:InvalidTableOperation ouch),

  // indirect tables with selector

  BmGroupHandle bm_mt_indirect_ws_create_group(
    1:string table_name
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_ws_delete_group(
    1:string table_name,
    2:BmGroupHandle grp_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_ws_add_member_to_group(
    1:string table_name,
    2:BmMemberHandle mbr_handle,
    3:BmGroupHandle grp_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_ws_remove_member_from_group(
    1:string table_name,
    2:BmMemberHandle mbr_handle,
    3:BmGroupHandle grp_handle
  ) throws (1:InvalidTableOperation ouch),

  BmEntryHandle bm_mt_indirect_ws_add_entry(
    1:string table_name,
    2:BmMatchParams match_key,
    3:BmGroupHandle grp_handle
    4:BmAddEntryOptions options
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_ws_modify_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle,
    3:BmGroupHandle grp_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_indirect_ws_set_default_group(
    1:string table_name,
    2:BmGroupHandle grp_handle
  ) throws (1:InvalidTableOperation ouch),

  BmCounterValue bm_mt_read_counter(
    1:string table_name,
    2:BmEntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_reset_counters(
    1:string table_name
  ) throws (1:InvalidTableOperation ouch),

  void bm_mt_write_counter(
    1:string table_name,
    2:BmEntryHandle entry_handle,
    3:BmCounterValue value,
  ) throws (1:InvalidTableOperation ouch),

  // indirect counters

  BmCounterValue bm_counter_read(
    1:string counter_name,
    2:i32 index
  ) throws (1:InvalidCounterOperation ouch),

  void bm_counter_reset_all(
    1:string counter_name
  ) throws (1:InvalidCounterOperation ouch),

  void bm_counter_write(
    1:string counter_name,
    2:i32 index,
    3:BmCounterValue value
  ) throws (1:InvalidCounterOperation ouch),

  // learning acks

  void bm_learning_ack(
    1:BmLearningListId list_id,
    2:BmLearningBufferId buffer_id,
    3:list<BmLearningSampleId> sample_ids
  ),

  void bm_learning_ack_buffer(
    1:BmLearningListId list_id,
    2:BmLearningBufferId buffer_id
  ),

  // swap configs

  void bm_load_new_config(
    1:string config_str
  ) throws (1:InvalidSwapOperation ouch),

  void bm_swap_configs() throws (1:InvalidSwapOperation ouch),

  
  // meters

  void bm_meter_array_set_rates(
    1:string meter_array_name,
    2:list<BmMeterRateConfig> rates
  ) throws (1:InvalidMeterOperation ouch)

  void bm_meter_set_rates(
    1:string meter_array_name,
    2:i32 index,
    3:list<BmMeterRateConfig> rates
  ) throws (1:InvalidMeterOperation ouch)


  // registers

  BmRegisterValue bm_register_read(
    1:string register_array_name,
    2:i32 idx
  ) throws (1:InvalidRegisterOperation ouch)

  void bm_register_write(
    1:string register_array_name,
    2:i32 index,
    3:BmRegisterValue value
  ) throws (1:InvalidRegisterOperation ouch)


  // device manager

  void bm_dev_mgr_add_port(
    1:string iface_name,
    2:i32 port_num,
    3:string pcap_path // optional
  ) throws (1:InvalidDevMgrOperation ouch)

  void bm_dev_mgr_remove_port(
    1:i32 port_num
  ) throws (1:InvalidDevMgrOperation ouch)

  // debug functions

  string bm_dump_table(
    1:string table_name
  )

  void bm_reset_state()
}
