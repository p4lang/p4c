#include <string>
#include <vector>

#include "pd/pd_types.h"
#include "pd/pd_static.h"
#include "pd_conn_mgr.h"

#define PD_DEBUG 1

#define HOST_BYTE_ORDER_CALLER 1

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   if not t.key: continue
static std::vector<std::string> build_key_${t_name} (
    ${pd_prefix}${t_name}_match_spec_t *match_spec
) {
  std::vector<std::string> key;
//::   for field_name, field_match_type, field_bw in t.key:
//::     field_name = get_c_name(field_name)
//::     width = bits_to_bytes(field_bw)
  key.push_back(std::string((char *) &(match_spec->${field_name}), ${width}));
//::   #endfor
  return key;
}

//:: #endfor
//::
//::
//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   if not t.key: continue
//::   if t.type_ != MatchType.TERNARY: continue
static std::vector<std::string> build_mask_${t_name} (
    ${pd_prefix}${t_name}_match_spec_t *match_spec
) {
  std::vector<std::string> mask;
//::   for field_name, field_match_type, field_bw in t.key:
//::     field_name = get_c_name(field_name)
//::     width = bits_to_bytes(field_bw)
//::     if field_match_type == MatchType.EXACT:
  mask.push_back(std::string((char) 0xFF, ${width}));
//::     elif field_match_type == MatchType.TERNARY:
  mask.push_back(std::string((char *) &(match_spec->${field_name}_mask), ${width}));
//::     elif field_match_type == MatchType.LPM:
  int pref_length = match_spec->${field_name}_prefix_length;
  std::string pref((char) 0x00, ${width});
  std::fill(pref.begin(), pref.begin() + (pref_length / 8), (char) 0xFF);
  if(pref_length % 8 != 0) {
    pref[pref_length / 8] = (char) 0xFF << (8 - (pref_length % 8));
  }
  mask.push_back(std::move(pref));
//::     #endif
//::   #endfor
  return mask;
}

//:: #endfor

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
//::     if not has_match_spec:
  std::vector<std::string> match_key;
//::     else:
  std::vector<std::string> match_key = build_key_${t_name}(match_spec);
//::     #endif
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a_name}(action_spec);
//::     #endif
//::     if match_type == MatchType.EXACT:
  *entry_hdl = pd_conn_mgr_client(dev_tgt.device_id)->bm_table_add_exact_match_entry(
       "${t_name}", "${a_name}",
       match_key,
       action_data
  );
//::     elif match_type == MatchType.LPM:
//::       prefix_length = 0
//::       lpm_field = None
//::       for field_name, field_match_type, field_bw in t.key:
//::         if field_match_type == MatchType.EXACT:
//::           prefix_length += bits_to_bytes(field_bw) * 8
//::         elif field_match_type == MatchType.LPM:
//::           assert(not lpm_field)
//::           lpm_field = field_name
//::         else:
//::           assert(False)
//::         #endif
//::       #endfor
  int prefix_length = ${prefix_length} + match_spec->${get_c_name(field_name)}_prefix_length;
  *entry_hdl = pd_conn_mgr_client(dev_tgt.device_id)->bm_table_add_lpm_entry(
       "${t_name}", "${a_name}",
       match_key,
       prefix_length,
       action_data
  );
//::     elif match_type == MatchType.TERNARY:
  *entry_hdl = pd_conn_mgr_client(dev_tgt.device_id)->bm_table_add_ternary_match_entry(
       "${t_name}", "${a_name}",
       match_key,
       build_mask_${t_name}(match_spec),
       priority,
       action_data
  );
//::     #endif
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
  pd_conn_mgr_client(dev_id)->bm_table_delete_entry("${t_name}", entry_hdl);
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
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a_name}(action_spec);
//::     #endif
  pd_conn_mgr_client(dev_id)->bm_table_modify_entry(
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
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a_name}(action_spec);
//::     #endif
  pd_conn_mgr_client(dev_tgt.device_id)->bm_set_default_action(
      "${t_name}", "${a_name}",
      action_data
  );
  return 0;
}

//::   #endfor
//:: #endfor

}
