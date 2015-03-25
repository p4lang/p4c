namespace cpp bm_runtime

typedef i32 BmEntryHandle
typedef list<binary> BmActionData
typedef list<binary> BmMatchKey

enum TableOperationErrorCode {
  TABLE_FULL = 1,
  INVALID_HANDLE = 2,
  ERROR = 3
}

exception InvalidTableOperation {
  1: TableOperationErrorCode what
}

service Runtime {
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
  ) throws (1:InvalidTableOperation ouch)

  void bm_table_delete_entry(
    1:string table_name,
    2:BmEntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch)
}
