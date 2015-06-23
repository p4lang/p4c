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

namespace cpp bm_runtime

typedef i32 BmEntryHandle
typedef list<binary> BmActionData

typedef i32 BmMemberHandle
typedef i32 BmGroupHandle

typedef i32 BmLearningListId
typedef i64 BmLearningBufferId
typedef i32 BmLearningSampleId

typedef i32 BmMcMgrp
typedef i32 BmMcRid
typedef i32 BmMcMgrpHandle
typedef i32 BmMcL1Handle
typedef i32 BmMcL2Handle
typedef string BmMcPortMap // string of 0s and 1s

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
  COUNTERS_DISABLED = 3,
  INVALID_TABLE_NAME = 4,
  INVALID_ACTION_NAME = 5,
  WRONG_TABLE_TYPE = 6,
  INVALID_MBR_HANDLE = 7,
  MBR_STILL_USED = 8,
  MBR_ALREADY_IN_GRP = 9,
  MBR_NOT_IN_GRP = 10,
  INVALID_GRP_HANDLE = 11,
  GRP_STILL_USED = 12,
  EMPTY_GRP = 13,
  ERROR = 14,
}

exception InvalidTableOperation {
  1:TableOperationErrorCode what
}

enum SwapOperationErrorCode {
  CONFIG_SWAP_DISABLED = 1,
  ONGOING_SWAP = 2,
  NO_ONGOING_SWAP = 3
}

exception InvalidSwapOperation {
  1:SwapOperationErrorCode what
}

enum McOperationErrorCode {
  TABLE_FULL = 1,
  INVALID_HANDLE = 2,
  INVALID_MGID = 3,
  INVALID_L1_HANDLE = 4,
  INVALID_L2_HANLDE = 5,
  ERROR = 6
}

exception InvalidMcOperation {
  1:McOperationErrorCode what
}

enum MeterOperationErrorCode {
  INVALID_INDEX = 1,
  BAD_RATES_LIST = 2,
  INVALID_INFO_RATE_VALUE = 3,
  INVALID_BURST_SIZE_VALUE = 4,
  ERROR = 5
}

exception InvalidMeterOperation {
 1:MeterOperationErrorCode what
}

// TODO
enum DevMgrErrorCode {
  ERROR = 1
}

exception InvalidDevMgrOperation {
 1:DevMgrErrorCode what
}

service Runtime {
	
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


  BmCounterValue bm_table_read_counter(
    1:string table_name,
    2:BmEntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_table_reset_counters(
    1:string table_name
  ) throws (1:InvalidTableOperation ouch),

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

  // pre operations

  BmMcMgrpHandle bm_mc_mgrp_create(
    1:BmMcMgrp mgrp
  ) throws (1:InvalidMcOperation ouch),

  void bm_mc_mgrp_destroy(
    1:BmMcMgrpHandle mgrp_handle
  ) throws (1:InvalidMcOperation ouch),

  BmMcL1Handle bm_mc_l1_node_create(
    1:BmMcRid rid
  ) throws (1:InvalidMcOperation ouch),

  void bm_mc_l1_node_associate(
    1:BmMcMgrpHandle mgrp_handle,
    2:BmMcL1Handle l1_handle
  ) throws (1:InvalidMcOperation ouch),

  void bm_mc_l1_node_destroy(
    1:BmMcL1Handle l1_handle
  ) throws (1:InvalidMcOperation ouch),

  BmMcL2Handle bm_mc_l2_node_create(
    1:BmMcL1Handle l1_handle,
    2:BmMcPortMap port_map
  ) throws (1:InvalidMcOperation ouch),

  void bm_mc_l2_node_update(
    1:BmMcL2Handle l2_handle,
    2:BmMcPortMap port_map
  ) throws (1:InvalidMcOperation ouch),

  void bm_mc_l2_node_destroy(
    1:BmMcL2Handle l2_handle
  ) throws (1:InvalidMcOperation ouch)
  
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

  // device manager

  void bm_dev_mgr_add_port(
    1:string iface_name,
    2:i32 port_num,
    3:string pcap_path // optional
  ) throws (1:InvalidDevMgrOperation ouch)

  void bm_dev_mgr_remove_port(
    1:i32 port_num
  ) throws (1:InvalidDevMgrOperation ouch)
}
