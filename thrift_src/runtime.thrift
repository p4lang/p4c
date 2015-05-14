namespace cpp bm_runtime

typedef i32 BmEntryHandle
typedef list<binary> BmActionData
typedef list<binary> BmMatchKey

typedef i32 BmLearningListId
typedef i64 BmLearningBufferId
typedef i32 BmLearningSampleId

typedef i32 BmMcMgrp
typedef i32 BmMcRid
typedef i32 BmMcMgrpHandle
typedef i32 BmMcL1Handle
typedef i32 BmMcL2Handle
typedef string BmMcPortMap // string of 0s and 1s

struct BmCounterValue {
  1:i64 bytes;
  2:i64 packets;
}

enum TableOperationErrorCode {
  TABLE_FULL = 1,
  INVALID_HANDLE = 2,
  ERROR = 3
}

exception InvalidTableOperation {
  1:TableOperationErrorCode what
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

service Runtime {
	
  // table operations

  BmEntryHandle bm_table_add_exact_match_entry(
    1:string table_name,
    2:string action_name,
    3:BmMatchKey match_key,
    4:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  BmEntryHandle bm_table_add_lpm_entry(
    1:string table_name,
    2:string action_name,
    3:BmMatchKey match_key,
    4:i32 prefix_length,
    5:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  BmEntryHandle bm_table_add_ternary_match_entry(
    1:string table_name,
    2:string action_name,
    3:BmMatchKey match_key,
    4:BmMatchKey match_mask,
    5:i32 priority,
    6:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  void bm_set_default_action(
    1:string table_name,
    2:string action_name,
    3:BmActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  void bm_table_delete_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch),

  void bm_table_modify_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle,
    3:string action_name,
    4:BmActionData action_data
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
  
}
