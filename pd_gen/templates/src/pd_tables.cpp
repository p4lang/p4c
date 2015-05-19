#include <string>
#include <vector>

#include "pd/pd_types.h"
#include "pd/pd_static.h"
#include "pd_conn_mgr.h"

#define PD_DEBUG 1

#define HOST_BYTE_ORDER_CALLER 1

extern pd_conn_mgr_t *conn_mgr_state;
extern int *my_devices;

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   if not t.key: continue
static std::vector<BmMatchParam> build_key_${t_name} (
    ${pd_prefix}${t_name}_match_spec_t *match_spec
) {
  std::vector<BmMatchParam> key;
  key.reserve(${len(t.key)});

  BmMatchParam param;
  BmMatchParamExact param_exact; (void) param_exact;
  BmMatchParamLPM param_lpm; (void) param_lpm;
  BmMatchParamTernary param_ternary; (void) param_ternary;
  BmMatchParamValid param_valid; (void) param_valid;

  // TODO: 3-byte fields (mapped to uint32) are probably handled incorrectly

//::   for field_name, field_match_type, field_bw in t.key:
//::     field_name = get_c_name(field_name)
//::     width = bits_to_bytes(field_bw)
//::     if field_match_type == MatchType.EXACT:
  param_exact.key = std::string((char *) &(match_spec->${field_name}), ${width});
  param = BmMatchParam();
  param.type = BmMatchParamType::type::EXACT;
  param.__set_exact(param_exact); // does a copy of param_exact
  key.push_back(std::move(param));
//::     elif field_match_type == MatchType.LPM:
  param_lpm.key = std::string((char *) &(match_spec->${field_name}), ${width});
  param_lpm.prefix_length = match_spec->${field_name}_prefix_length;
  param = BmMatchParam();
  param.type = BmMatchParamType::type::LPM;
  param.__set_lpm(param_lpm); // does a copy of param_lpm
  key.push_back(std::move(param));
//::     elif field_match_type == MatchType.TERNARY:
  param_ternary.key = std::string((char *) &(match_spec->${field_name}), ${width});
  param_ternary.mask = std::string((char *) &(match_spec->${field_name}_mask), ${width});
  param = BmMatchParam();
  param.type = BmMatchParamType::type::TERNARY;
  param.__set_ternary(param_ternary); // does a copy of param_ternary
  key.push_back(std::move(param));
//::     elif field_match_type == MatchType.VALID:
  param_valid.key = (match_spec->${field_name} == 1);
  param = BmMatchParam();
  param.type = BmMatchParamType::type::VALID;
  param.__set_valid(param_valid); // does a copy of param_valid
  key.push_back(std::move(param));
//::     else:
//::       assert(0)
//::     #endif

//::   #endfor
  return key;
}

//:: #endfor
//::

//:: for a_name, a in actions.items():
//::   a_name = get_c_name(a_name)
//::   if not a.runtime_data: continue
//::   action_params = gen_action_params(a.runtime_data)
static std::vector<std::string> build_action_data_${a_name} (
    ${pd_prefix}${a_name}_action_spec_t *action_spec
) {
  std::vector<std::string> action_data;
//::   for name, width in action_params:
//::     name = get_c_name(name)
  action_data.push_back(std::string((char *) &(action_spec->${name}), ${width}));
//::   #endfor
  return action_data;
}

//:: #endfor

extern "C" {

/* ADD ENTRIES */

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   match_type = t.type_
//::   has_match_spec = len(t.key) > 0
//::   for a_name, a in t.actions.items():
//::     a_name = get_c_name(a_name)
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_match_spec:
//::       params += [pd_prefix + t_name + "_match_spec_t *match_spec"]
//::     #endif
//::     if match_type == MatchType.TERNARY:
//::       params += ["int priority"]
//::     #endif
//::     if has_action_spec:
//::       params += [pd_prefix + a_name + "_action_spec_t *action_spec"]
//::     #endif
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + t_name + "_table_add_with_" + a_name
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::     if not has_match_spec:
  std::vector<BmMatchParam> match_key;
//::     else:
  std::vector<BmMatchParam> match_key = build_key_${t_name}(match_spec);
//::     #endif
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a_name}(action_spec);
//::     #endif
  BmAddEntryOptions options;
//::     if match_type == MatchType.TERNARY:
  options.__set_priority(priority);
//::     #endif
  *entry_hdl = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_table_add_entry(
       "${t_name}",
       match_key,
       "${a_name}",
       action_data,
       options
  );
  return 0;
}

//::   #endfor
//:: #endfor

/* DELETE ENTRIES */

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   name = pd_prefix + t_name + "_table_delete"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t dev_id,
 p4_pd_entry_hdl_t entry_hdl
) {
  assert(my_devices[dev_id]);
  pd_conn_mgr_client(conn_mgr_state, dev_id)->bm_table_delete_entry("${t_name}", entry_hdl);
  return 0;
}

//:: #endfor

/* MODIFY ENTRIES */

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   for a_name, a in t.actions.items():
//::     a_name = get_c_name(a_name)
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t dev_id",
//::               "p4_pd_entry_hdl_t entry_hdl"]
//::     if has_action_spec:
//::       params += [pd_prefix + a_name + "_action_spec_t *action_spec"]
//::     #endif
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + t_name + "_table_modify_with_" + a_name
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a_name}(action_spec);
//::     #endif
  pd_conn_mgr_client(conn_mgr_state, dev_id)->bm_table_modify_entry(
      "${t_name}", entry_hdl,
      "${a_name}", action_data
  );
  return 0;
}

//::   #endfor
//:: #endfor


/* SET DEFAULT_ACTION */

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   for a_name, a in t.actions.items():
//::     a_name = get_c_name(a_name)
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += [pd_prefix + a_name + "_action_spec_t *action_spec"]
//::     #endif
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + t_name + "_set_default_action_" + a_name
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a_name}(action_spec);
//::     #endif
  pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_set_default_action(
      "${t_name}", "${a_name}",
      action_data
  );
  return 0;
}

//::   #endfor
//:: #endfor



/* DIRECT COUNTERS */

//:: for t_name, t in tables.items():
//::   if not t.with_counters: continue
//::   t_name = get_c_name(t_name)
//::   name = pd_prefix + t_name + "_read_counter"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_hdl,
 p4_pd_counter_value_t *counter_value
) {
  assert(my_devices[dev_tgt.device_id]);
  BmCounterValue value;
  // Thrift's weirdness ? even on client side, the return value becomes the
  // first argument and is passed by reference
  pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_table_read_counter(
      value,
      "${t_name}",
      entry_hdl
  );
  counter_value->bytes = (uint64_t) value.bytes;
  counter_value->packets = (uint64_t) value.packets;
  return 0;
}

//::   name = pd_prefix + t_name + "_reset_counters"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
) {
  assert(my_devices[dev_tgt.device_id]);
  pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_table_reset_counters(
      "${t_name}"
  );
  return 0;
}

//:: #endfor

}
