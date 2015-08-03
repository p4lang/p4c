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

#include <string>
#include <vector>

#include "pd/pd_types.h"
#include "pd/pd_static.h"
#include "pd_conn_mgr.h"

#define PD_DEBUG 1

// default is disbaled
// #define HOST_BYTE_ORDER_CALLER

extern pd_conn_mgr_t *conn_mgr_state;
extern int *my_devices;

namespace {

template <int L>
std::string string_from_field(char *field) {
  return std::string((char *) field, L);
}

template <>
std::string string_from_field<1>(char *field) {
  return std::string(field, 1);
}

template <>
std::string string_from_field<2>(char *field) {
  uint16_t tmp = *(uint16_t *) field;
#ifdef HOST_BYTE_ORDER_CALLER
  tmp = ntohs(tmp);
#endif
  return std::string((char *) &tmp, 2);
}

template <>
std::string string_from_field<3>(char *field) {
  uint32_t tmp = *(uint32_t *) field;
#ifdef HOST_BYTE_ORDER_CALLER
  tmp = ntohl(tmp);
#endif
  return std::string(((char *) &tmp) + 1, 3);
}

template <>
std::string string_from_field<4>(char *field) {
  uint32_t tmp = *(uint32_t *) field;
#ifdef HOST_BYTE_ORDER_CALLER
  tmp = ntohl(tmp);
#endif
  return std::string((char *) &tmp, 4);
}

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   if not t.key: continue
std::vector<BmMatchParam> build_key_${t_name} (
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
  param_exact.key = string_from_field<${width}>((char *) &(match_spec->${field_name}));
  param = BmMatchParam();
  param.type = BmMatchParamType::type::EXACT;
  param.__set_exact(param_exact); // does a copy of param_exact
  key.push_back(std::move(param));
//::     elif field_match_type == MatchType.LPM:
  param_lpm.key = string_from_field<${width}>((char *) &(match_spec->${field_name}));
  param_lpm.prefix_length = match_spec->${field_name}_prefix_length;
  param = BmMatchParam();
  param.type = BmMatchParamType::type::LPM;
  param.__set_lpm(param_lpm); // does a copy of param_lpm
  key.push_back(std::move(param));
//::     elif field_match_type == MatchType.TERNARY:
  param_ternary.key = string_from_field<${width}>((char *) &(match_spec->${field_name}));
  param_ternary.mask = string_from_field<${width}>((char *) &(match_spec->${field_name}_mask));
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
std::vector<std::string> build_action_data_${a_name} (
    ${pd_prefix}${a_name}_action_spec_t *action_spec
) {
  std::vector<std::string> action_data;
//::   for name, width in action_params:
//::     name = get_c_name(name)
  action_data.push_back(string_from_field<${width}>((char *) &(action_spec->${name})));
//::   #endfor
  return action_data;
}

//:: #endfor

}

extern "C" {

/* ADD ENTRIES */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   t_name = get_c_name(t_name)
//::   match_type = t.match_type
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
//::     if t.support_timeout:
//::       params += ["uint32_t ttl"]
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
  try {
    *entry_hdl = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_mt_add_entry(
      "${t_name}", match_key,
      "${a_name}", action_data,
      options
    );
//::     if t.support_timeout:

    // bmv2 takes a ttl in milliseconds
    pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_mt_set_entry_ttl(
      "${t_name}", *entry_hdl, ttl * 1000
    );
//::     #endif
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   #endfor
//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   t_name = get_c_name(t_name)
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if has_match_spec:
//::     params += [pd_prefix + t_name + "_match_spec_t *match_spec"]
//::   #endif
//::   if match_type == MatchType.TERNARY:
//::     params += ["int priority"]
//::   #endif
//::
//::   params_indirect = params + ["p4_pd_mbr_hdl_t mbr_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect)
//::   name = pd_prefix + t_name + "_add_entry"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::   if not has_match_spec:
  std::vector<BmMatchParam> match_key;
//::   else:
  std::vector<BmMatchParam> match_key = build_key_${t_name}(match_spec);
//::   #endif
  BmAddEntryOptions options;
//::   if match_type == MatchType.TERNARY:
  options.__set_priority(priority);
//::   #endif
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id);
  try {
    *entry_hdl = client->bm_mt_indirect_add_entry(
      "${t_name}", match_key, mbr_hdl, options
    );
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_indirect_ws = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect_ws)
//::   name = pd_prefix + t_name + "_add_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::   if not has_match_spec:
  std::vector<BmMatchParam> match_key;
//::   else:
  std::vector<BmMatchParam> match_key = build_key_${t_name}(match_spec);
//::   #endif
  BmAddEntryOptions options;
//::   if match_type == MatchType.TERNARY:
  options.__set_priority(priority);
//::   #endif
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id);
  try {
    *entry_hdl = client->bm_mt_indirect_ws_add_entry(
      "${t_name}", match_key, grp_hdl, options
    );
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//:: #endfor

/* DELETE ENTRIES */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
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
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
//::   if t_type == TableType.SIMPLE:
    client->bm_mt_delete_entry("${t_name}", entry_hdl);
//::   else:
    client->bm_mt_indirect_delete_entry("${t_name}", entry_hdl);
//::   #endif
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//:: #endfor

/* MODIFY ENTRIES */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
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
  try {
    pd_conn_mgr_client(conn_mgr_state, dev_id)->bm_mt_modify_entry(
      "${t_name}", entry_hdl, "${a_name}", action_data
    );
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   #endfor
//:: #endfor


/* SET DEFAULT_ACTION */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
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
  try {
    pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_mt_set_default_action(
      "${t_name}", "${a_name}", action_data
    );
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   #endfor
//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   t_name = get_c_name(t_name)
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::
//::   params_indirect = params + ["p4_pd_mbr_hdl_t mbr_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect)
//::   name = pd_prefix + t_name + "_set_default_entry"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id);
  try {
    client->bm_mt_indirect_set_default_member("${t_name}", mbr_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_indirect_ws = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect_ws)
//::   name = pd_prefix + t_name + "_set_default_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id);
  try {
    client->bm_mt_indirect_ws_set_default_group("${t_name}", grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   t_name = get_c_name(t_name)
//::   act_prof_name = get_c_name(t.act_prof)
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   for a_name, a in t.actions.items():
//::     a_name = get_c_name(a_name)
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += [pd_prefix + a_name + "_action_spec_t *action_spec"]
//::     #endif
//::     params += ["p4_pd_mbr_hdl_t *mbr_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + act_prof_name + "_add_member_with_" + a_name
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
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id);
  try {
    *mbr_hdl = client->bm_mt_indirect_add_member(
      "${t_name}", "${a_name}", action_data
    );
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t dev_id",
//::               "p4_pd_mbr_hdl_t mbr_hdl"]
//::     if has_action_spec:
//::       params += [pd_prefix + a_name + "_action_spec_t *action_spec"]
//::     #endif
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + act_prof_name + "_modify_member_with_" + a_name
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
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client->bm_mt_indirect_modify_member(
      "${t_name}", mbr_hdl, "${a_name}", action_data
    );
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   #endfor
//::
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof_name + "_del_member"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client->bm_mt_indirect_delete_member("${t_name}", mbr_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   if t.type_ != TableType.INDIRECT_WS: continue
//::
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::             "uint16_t max_grp_size",
//::             "p4_pd_grp_hdl_t *grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof_name + "_create_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  (void) max_grp_size;
  assert(my_devices[dev_tgt.device_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id);
  try {
    *grp_hdl = client->bm_mt_indirect_ws_create_group("${t_name}");
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof_name + "_del_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client->bm_mt_indirect_ws_delete_group("${t_name}", grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof_name + "_add_member_to_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client->bm_mt_indirect_ws_add_member_to_group("${t_name}", mbr_hdl, grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof_name + "_del_member_from_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  Client *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client->bm_mt_indirect_ws_remove_member_from_group("${t_name}", mbr_hdl, grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.what)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.what << "): " << what << std::endl;
    return ito.what;
  }
  return 0;
}

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

//:: for t_name, t in tables.items():
//:: if not t.support_timeout: continue
//::   p4_pd_enable_hit_state_scan = "_".join([pd_prefix[:-1], t_name, "enable_hit_state_scan"])
//::   p4_pd_get_hit_state = "_".join([pd_prefix[:-1], t_name, "get_hit_state"])
//::   p4_pd_set_entry_ttl = "_".join([pd_prefix[:-1], t_name, "set_entry_ttl"])
//::   p4_pd_enable_entry_timeout = "_".join([pd_prefix[:-1], t_name, "enable_entry_timeout"])
p4_pd_status_t
${p4_pd_enable_hit_state_scan}(p4_pd_sess_hdl_t sess_hdl, uint32_t scan_interval) {
  // This function is a no-op. Needed for real hardware.
  (void)sess_hdl;
  (void)scan_interval;
  return 0;
}

p4_pd_status_t
${p4_pd_get_hit_state}(p4_pd_sess_hdl_t sess_hdl, p4_pd_entry_hdl_t entry_hdl, p4_pd_hit_state_t *hit_state) {
  (void) sess_hdl; (void) entry_hdl;
  *hit_state = ENTRY_HIT; /* TODO */
  return 0;
}

p4_pd_status_t
${p4_pd_set_entry_ttl}(p4_pd_sess_hdl_t sess_hdl, p4_pd_entry_hdl_t entry_hdl, uint32_t ttl) {
  (void) sess_hdl; (void) entry_hdl; (void) ttl;
  return 0;
}

p4_pd_status_t ${pd_prefix}ageing_set_cb(int dev_id, int table_id,
					 p4_pd_notify_timeout_cb cb_fn,
					 void *cb_cookie);

p4_pd_status_t
${p4_pd_enable_entry_timeout}(p4_pd_sess_hdl_t sess_hdl,
			      p4_pd_notify_timeout_cb cb_fn,
			      uint32_t max_ttl,
			      void *client_data) {
  (void) sess_hdl; (void) max_ttl;
  // TODO: use max_ttl to set up sweep interval
  return ${pd_prefix}ageing_set_cb(0, ${t.id_}, cb_fn, client_data);
}
//:: #endfor

}
