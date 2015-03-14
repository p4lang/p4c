namespace cpp bm_runtime

typedef i32 EntryHandle
typedef list<binary> ActionData

exception InvalidTableOperation {
  1: i32 what
}

service Runtime {
  EntryHandle bm_table_add_exact_match_entry(
    1:string table_name,
    2:string action_name,
    3:binary key,
    4:ActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  EntryHandle bm_table_add_lpm_entry(
    1:string table_name,
    2:string action_name,
    3:binary key,
    4:i32 prefix_length,
    5:ActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  EntryHandle bm_table_add_ternary_match_entry(
    1:string table_name,
    2:string action_name,
    3:binary key,
    4:binary mask,
    5:i32 priority,
    6:ActionData action_data
  ) throws (1:InvalidTableOperation ouch),

  void bm_set_default_action(
    1:string table_name,
    2:string action_name,
    3:ActionData action_data
  ) throws (1:InvalidTableOperation ouch)

  void bm_table_delete_entry(
    1:string table_name,
    2:EntryHandle entry_handle
  ) throws (1:InvalidTableOperation ouch)
}
