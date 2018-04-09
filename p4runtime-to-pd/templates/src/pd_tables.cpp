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

#include <bm/pdfixed/pd_common.h>
#include <bm/pdfixed/int/pd_helpers.h>

#include <string>
#include <vector>
#include <iostream>

#include "pd/pd_types.h"
#include "pd_client.h"

#define PD_DEBUG 1

// default is disabled
// #define HOST_BYTE_ORDER_CALLER

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
  tmp = htons(tmp);
#endif
  return std::string((char *) &tmp, 2);
}

template <>
std::string string_from_field<3>(char *field) {
  uint32_t tmp = *(uint32_t *) field;
#ifdef HOST_BYTE_ORDER_CALLER
  tmp = htonl(tmp);
#endif
  return std::string(((char *) &tmp) + 1, 3);
}

template <>
std::string string_from_field<4>(char *field) {
  uint32_t tmp = *(uint32_t *) field;
#ifdef HOST_BYTE_ORDER_CALLER
  tmp = htonl(tmp);
#endif
  return std::string((char *) &tmp, 4);
}

template <int L>
void string_to_field(const std::string &s, char *field) {
  assert(s.size() <= L);
  size_t offset = L - s.size();
  std::memset(field, 0, offset);
  std::memcpy(field + offset, s.data(), s.size());
}

template <>
void string_to_field<1>(const std::string &s, char *field) {
  assert(s.size() <= 1);
  if (s.size() == 1) *field = s[0];
}

template <>
void string_to_field<2>(const std::string &s, char *field) {
  uint16_t *tmp = (uint16_t *) field;
  *tmp = 0;
  assert(s.size() <= 2);
  size_t offset = 2 - s.size();
  std::memcpy(field, s.data(), s.size());
#ifdef HOST_BYTE_ORDER_CALLER
  *tmp = ntohs(*tmp);
#endif
}

template <>
void string_to_field<3>(const std::string &s, char *field) {
  uint32_t *tmp = (uint32_t *) field;
  *tmp = 0;
  assert(s.size() <= 3);
  size_t offset = 3 - s.size();
  std::memcpy(field, s.data(), s.size());
#ifdef HOST_BYTE_ORDER_CALLER
  *tmp = ntohl(*tmp);
#endif
}

template <>
void string_to_field<4>(const std::string &s, char *field) {
  uint32_t *tmp = (uint32_t *) field;
  *tmp = 0;
  assert(s.size() <= 4);
  size_t offset = 4 - s.size();
  std::memcpy(field, s.data(), s.size());
#ifdef HOST_BYTE_ORDER_CALLER
  *tmp = ntohl(*tmp);
#endif
}

//:: for t_name, t in tables.items():
//::   if not t.key: continue
std::vector<BmMatchParam> build_key_${t.cname} (
    ${pd_prefix}${t.cname}_match_spec_t *match_spec
) {
  std::vector<BmMatchParam> key;
  key.reserve(${len(t.key)});

  BmMatchParam param;
  BmMatchParamExact param_exact; (void) param_exact;
  BmMatchParamLPM param_lpm; (void) param_lpm;
  BmMatchParamTernary param_ternary; (void) param_ternary;
  BmMatchParamValid param_valid; (void) param_valid;
  BmMatchParamRange param_range; (void) param_range;

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
//::     elif field_match_type == MatchType.RANGE:
  param_range.start = string_from_field<${width}>((char *) &(match_spec->${field_name}_start));
  param_range.end_ = string_from_field<${width}>((char *) &(match_spec->${field_name}_end));
  param = BmMatchParam();
  param.type = BmMatchParamType::type::RANGE;
  param.__set_range(param_range); // does a copy of param_range
  key.push_back(std::move(param));
//::     else:
//::       assert(0)
//::     #endif

//::   #endfor
  return key;
}

void unbuild_key_${t.cname} (
    const std::vector<BmMatchParam> &key,
    ${pd_prefix}${t.cname}_match_spec_t *match_spec
) {
  size_t i = 0;
//::   for field_name, field_match_type, field_bw in t.key:
  {
    const BmMatchParam &param = key.at(i++);
//::     field_name = get_c_name(field_name)
//::     width = bits_to_bytes(field_bw)
//::     if field_match_type == MatchType.EXACT:
    assert(param.type == BmMatchParamType::type::EXACT);
    string_to_field<${width}>(param.exact.key, (char *) &(match_spec->${field_name}));
//::     elif field_match_type == MatchType.LPM:
    assert(param.type == BmMatchParamType::type::LPM);
    string_to_field<${width}>(param.lpm.key, (char *) &(match_spec->${field_name}));
    match_spec->${field_name}_prefix_length = param.lpm.prefix_length;
//::     elif field_match_type == MatchType.TERNARY:
    assert(param.type == BmMatchParamType::type::TERNARY);
    string_to_field<${width}>(param.ternary.key, (char *) &(match_spec->${field_name}));
    string_to_field<${width}>(param.ternary.mask, (char *) &(match_spec->${field_name}_mask));
//::     elif field_match_type == MatchType.VALID:
    assert(param.type == BmMatchParamType::type::VALID);
    match_spec->${field_name} = (param.valid.key) ? 1 : 0;
//::     elif field_match_type == MatchType.RANGE:
    assert(param.type == BmMatchParamType::type::RANGE);
    string_to_field<${width}>(param.range.start, (char *) &(match_spec->${field_name}_start));
    string_to_field<${width}>(param.range.end_, (char *) &(match_spec->${field_name}_end));
//::     else:
//::       assert(0)
//::     #endif
  }

//::   #endfor
}

//:: #endfor
//::

//:: for a_name, a in actions.items():
//::   if not a.runtime_data: continue
//::   action_params = gen_action_params(a.runtime_data)
std::vector<std::string> build_action_data_${a.cname} (
    ${pd_prefix}${a.cname}_action_spec_t *action_spec
) {
  std::vector<std::string> action_data;
//::   for name, width in action_params:
//::     name = get_c_name(name)
  action_data.push_back(string_from_field<${width}>((char *) &(action_spec->${name})));
//::   #endfor
  return action_data;
}

void unbuild_action_data_${a.cname} (
    const std::vector<std::string> &action_data,
    ${pd_prefix}${a.cname}_action_spec_t *action_spec
) {
  size_t i = 0;
//::   for name, width in action_params:
//::     name = get_c_name(name)
  string_to_field<${width}>(action_data.at(i++), (char *) &(action_spec->${name}));
//::   #endfor
}
//:: #endfor

}

//:: def get_direct_parameter_specs(d, t):
//::   for k in d:
//::     exec "%s=d[k]" % k
//::   #endfor
//::   specs = []
//::   if t.direct_meters:
//::     m_name = t.direct_meters
//::     m = meter_arrays[m_name]
//::     if m.type_ == MeterType.PACKETS:
//::       specs += ["p4_pd_packets_meter_spec_t *" + m.cname + "_spec"]
//::     else:
//::       specs += ["p4_pd_bytes_meter_spec_t *" + m.cname + "_spec"]
//::     #endif
//::   #endif
//::   return specs
//:: #enddef

extern "C" {

/* ADD ENTRIES */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_match_spec:
//::       params += [pd_prefix + t.cname + "_match_spec_t *match_spec"]
//::     #endif
//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::       params += ["int priority"]
//::     #endif
//::     if has_action_spec:
//::       params += [pd_prefix + a.cname + "_action_spec_t *action_spec"]
//::     #endif
//::     if t.support_timeout:
//::       params += ["uint32_t ttl"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t)
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + t.cname + "_table_add_with_" + a.cname
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::     if not has_match_spec:
  std::vector<BmMatchParam> match_key;
//::     else:
  std::vector<BmMatchParam> match_key = build_key_${t.cname}(match_spec);
//::     #endif
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a.cname}(action_spec);
//::     #endif
  BmAddEntryOptions options;
//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
  options.__set_priority(priority);
//::     #endif
  try {
    *entry_hdl = pd_client(dev_tgt.device_id).c->bm_mt_add_entry(
        0, "${t_name}", match_key, "${a_name}", action_data, options);
//::     if t.support_timeout:

    // bmv2 takes a ttl in milliseconds
    pd_client(dev_tgt.device_id).c->bm_mt_set_entry_ttl(
        0, "${t_name}", *entry_hdl, ttl);
//::     #endif
//::     if t.direct_meters:

//::       m_name = t.direct_meters
//::       m = meter_arrays[m_name]
//::       type_name = MeterType.to_str(meter_arrays[m_name].type_)
    pd_client(dev_tgt.device_id).c->bm_mt_set_meter_rates(
        0, "${t_name}", *entry_hdl,
        pd_${type_name}_meter_spec_to_rates(${m.cname}_spec));
//::     #endif
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   #endfor
//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if has_match_spec:
//::     params += [pd_prefix + t.cname + "_match_spec_t *match_spec"]
//::   #endif
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::     params += ["int priority"]
//::   #endif
//::
//::   params_indirect = params + ["p4_pd_mbr_hdl_t mbr_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect)
//::   name = pd_prefix + t.cname + "_add_entry"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::   if not has_match_spec:
  std::vector<BmMatchParam> match_key;
//::   else:
  std::vector<BmMatchParam> match_key = build_key_${t.cname}(match_spec);
//::   #endif
  BmAddEntryOptions options;
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
  options.__set_priority(priority);
//::   #endif
  try {
    *entry_hdl = pd_client(dev_tgt.device_id).c->bm_mt_indirect_add_entry(
        0, "${t_name}", match_key, mbr_hdl, options);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_indirect_ws = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect_ws)
//::   name = pd_prefix + t.cname + "_add_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::   if not has_match_spec:
  std::vector<BmMatchParam> match_key;
//::   else:
  std::vector<BmMatchParam> match_key = build_key_${t.cname}(match_spec);
//::   #endif
  BmAddEntryOptions options;
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
  options.__set_priority(priority);
//::   #endif
  try {
    *entry_hdl = pd_client(dev_tgt.device_id).c->bm_mt_indirect_ws_add_entry(
        0, "${t_name}", match_key, grp_hdl, options);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//:: #endfor

/* DELETE ENTRIES */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   name = pd_prefix + t.cname + "_table_delete"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t dev_id,
 p4_pd_entry_hdl_t entry_hdl
) {
  assert(my_devices[dev_id]);
  auto client = pd_client(dev_id);
  try {
//::   if t_type == TableType.SIMPLE:
    client.c->bm_mt_delete_entry(0, "${t_name}", entry_hdl);
//::   else:
    client.c->bm_mt_indirect_delete_entry(0, "${t_name}", entry_hdl);
//::   #endif
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//:: #endfor

/* MODIFY ENTRIES */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t dev_id",
//::               "p4_pd_entry_hdl_t entry_hdl"]
//::     if has_action_spec:
//::       params += [pd_prefix + a.cname + "_action_spec_t *action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t)
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + t.cname + "_table_modify_with_" + a.cname
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a.cname}(action_spec);
//::     #endif
  try {
    pd_client(dev_id).c->bm_mt_modify_entry(
        0, "${t_name}", entry_hdl, "${a_name}", action_data);
//::     if t.direct_meters:

//::       m_name = t.direct_meters
//::       m = meter_arrays[m_name]
//::       type_name = MeterType.to_str(meter_arrays[m_name].type_)
    pd_client(dev_id).c->bm_mt_set_meter_rates(
        0, "${t_name}", entry_hdl,
        pd_${type_name}_meter_spec_to_rates(${m.cname}_spec));
//::     #endif
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   #endfor
//:: #endfor


/* SET DEFAULT_ACTION */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += [pd_prefix + a.cname + "_action_spec_t *action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t)
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + t.cname + "_set_default_action_" + a.cname
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a.cname}(action_spec);
//::     #endif
  try {
    pd_client(dev_tgt.device_id).c->bm_mt_set_default_action(
        0, "${t_name}", "${a_name}", action_data);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   #endfor
//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::
//::   params_indirect = params + ["p4_pd_mbr_hdl_t mbr_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect)
//::   name = pd_prefix + t.cname + "_set_default_entry"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
  auto client = pd_client(dev_tgt.device_id);
  try {
    client.c->bm_mt_indirect_set_default_member(0, "${t_name}", mbr_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_indirect_ws = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_indirect_ws)
//::   name = pd_prefix + t.cname + "_set_default_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
  auto client = pd_client(dev_tgt.device_id);
  try {
    client.c->bm_mt_indirect_ws_set_default_group(0, "${t_name}", grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//:: #endfor


/* CLEAR DEFAULT_ACTION */

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   name = pd_prefix + t.cname + "_table_reset_default_entry"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
)
{
  (void)sess_hdl;
  (void)dev_tgt;
  return 0;
}
//:: #endfor


//:: for ap_name, act_prof in action_profs.items():
//::   for a_name, a in act_prof.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += [pd_prefix + a.cname + "_action_spec_t *action_spec"]
//::     #endif
//::     params += ["p4_pd_mbr_hdl_t *mbr_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + act_prof.cname + "_add_member_with_" + a.cname
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_tgt.device_id]);
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a.cname}(action_spec);
//::     #endif
  auto client = pd_client(dev_tgt.device_id);
  try {
    *mbr_hdl = client.c->bm_mt_act_prof_add_member(
        0, "${ap_name}", "${a_name}", action_data);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t dev_id",
//::               "p4_pd_mbr_hdl_t mbr_hdl"]
//::     if has_action_spec:
//::       params += [pd_prefix + a.cname + "_action_spec_t *action_spec"]
//::     #endif
//::     param_str = ",\n ".join(params)
//::     name = pd_prefix + act_prof.cname + "_modify_member_with_" + a.cname
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
//::     if not has_action_spec:
  std::vector<std::string> action_data;
//::     else:
  std::vector<std::string> action_data = build_action_data_${a.cname}(action_spec);
//::     #endif
  auto client = pd_client(dev_id);
  try {
    client.c->bm_mt_act_prof_modify_member(
        0, "${ap_name}", mbr_hdl, "${a_name}", action_data);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   #endfor
//::
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof.cname + "_del_member"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  auto client = pd_client(dev_id);
  try {
    client.c->bm_mt_act_prof_delete_member(0, "${ap_name}", mbr_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   if not act_prof.with_selection: continue
//::
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::             "uint16_t max_grp_size",
//::             "p4_pd_grp_hdl_t *grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof.cname + "_create_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  (void) max_grp_size;
  assert(my_devices[dev_tgt.device_id]);
  auto client = pd_client(dev_tgt.device_id);
  try {
    *grp_hdl = client.c->bm_mt_act_prof_create_group(0, "${ap_name}");
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof.cname + "_del_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  auto client = pd_client(dev_id);
  try {
    client.c->bm_mt_act_prof_delete_group(0, "${ap_name}", grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof.cname + "_add_member_to_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  auto client = pd_client(dev_id);
  try {
    client.c->bm_mt_act_prof_add_member_to_group(
        0, "${ap_name}", mbr_hdl, grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof.cname + "_del_member_from_group"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  auto client = pd_client(dev_id);
  try {
    client.c->bm_mt_act_prof_remove_member_from_group(
        0, "${ap_name}", mbr_hdl, grp_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//:: #endfor

/* ENTRY RETRIEVAL */

//:: for t_name, t in tables.items():
//::   match_type = t.match_type
//::   name = pd_prefix + t.cname + "_get_entry"
//::   common_params = ["p4_pd_sess_hdl_t sess_hdl", "uint8_t dev_id",
//::                    "p4_pd_entry_hdl_t entry_hdl", "bool read_from_hw"]
//::   has_match_spec = len(t.key) > 0
//::   if has_match_spec:
//::     common_params += [pd_prefix + t.cname + "_match_spec_t *match_spec"]
//::   #endif
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::     common_params += ["int *priority"]
//::   #endif
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE:
//::     params = common_params + ["char **action_name", "uint8_t *action_data",
//::                               "int *num_action_bytes"] # action_data has to be large enough
//::     param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  BmMtEntry entry;
  try {
    pd_client(dev_id).c->bm_mt_get_entry(entry, 0, "${t_name}", entry_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
//::     if has_match_spec:
  unbuild_key_${t.cname}(entry.match_key, match_spec);
//::     #endif

  const BmActionEntry &action_entry = entry.action_entry;
  assert(action_entry.action_type == BmActionEntryType::ACTION_DATA);
  *num_action_bytes = 0;
  // not efficient, but who cares
//::     for a_name, a in t.actions.items():
//::       has_action_spec = len(a.runtime_data) > 0
//::       if not has_action_spec: continue
  if (action_entry.action_name == "${a_name}") {
    unbuild_action_data_${a.cname}(
        action_entry.action_data,
        (${pd_prefix}${a.cname}_action_spec_t *) action_data);
    *num_action_bytes = sizeof(${pd_prefix}${a.cname}_action_spec_t);
    // not valid in C++, hence the cast, but I have no choice (can't change the
    // signature of the method)
    *action_name = (char *) "${a_name}";
  }
//::     #endfor

//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
  *priority = entry.options.priority;
//::     #endif

  return 0;
}
//::   else:
//::     # indirect tables
//::     params = common_params + ["p4_pd_mbr_hdl_t *mbr_hdls",
//::                               "int *num_mbrs"] # has to be large enough
//::     param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  BmMtEntry entry;
  try {
    pd_client(dev_id).c->bm_mt_get_entry(entry, 0, "${t_name}", entry_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
//::     if has_match_spec:
  unbuild_key_${t.cname}(entry.match_key, match_spec);
//::     #endif

//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
  *priority = entry.options.priority;
//::     #endif

  const BmActionEntry &action_entry = entry.action_entry;
//::     if t_type == TableType.INDIRECT:
  assert(action_entry.action_type == BmActionEntryType::MBR_HANDLE);
//::     #endif

  if (action_entry.action_type == BmActionEntryType::MBR_HANDLE) {
    *num_mbrs = 1;
    *mbr_hdls = action_entry.mbr_handle;
  } else {
    assert(action_entry.action_type == BmActionEntryType::GRP_HANDLE);
    BmMtActProfGroup group;
    try {
//::     ap_name = t.action_prof.name
      pd_client(dev_id).c->bm_mt_act_prof_get_group(
          group, 0, "${ap_name}", action_entry.grp_handle);
    } catch (InvalidTableOperation &ito) {
      const char *what =
          _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
      std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
                << ito.code << "): " << what << std::endl;
      return ito.code;
    }
    for (const auto mbr: group.mbr_handles) {
      mbr_hdls[*num_mbrs] = mbr;
      (*num_mbrs)++;
    }
  }

  return 0;
}
//::   #endif

//:: #endfor

//:: for ap_name, act_prof in action_profs.items():
//::   params = ["p4_pd_sess_hdl_t sess_hdl", "uint8_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl", "bool read_from_hw",
//::             "char **action_name",
//::             "uint8_t *action_data", # has to be large enough
//::             "int *num_action_bytes"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + act_prof.cname + "_get_member"
p4_pd_status_t
${name}
(
 ${param_str}
) {
  assert(my_devices[dev_id]);
  BmMtActProfMember member;
  try {
    pd_client(dev_id).c->bm_mt_act_prof_get_member(
        member, 0, "${ap_name}", mbr_hdl);
  } catch (InvalidTableOperation &ito) {
    const char *what =
        _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid action profile (" << "${ap_name}" << ") operation ("
              << ito.code << "): " << what << std::endl;
    return ito.code;
  }

  *num_action_bytes = 0;
  // not efficient, but who cares
//::   for a_name, a in act_prof.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     if not has_action_spec: continue
  if (member.action_name == "${a_name}") {
    unbuild_action_data_${a.cname}(
        member.action_data,
        (${pd_prefix}${a.cname}_action_spec_t *) action_data);
    *num_action_bytes = sizeof(${pd_prefix}${a.cname}_action_spec_t);
    // not valid in C++, hence the cast, but I have no choice (can't change the
    // signature of the method)
    *action_name = (char *) "${a_name}";
  }
//::     #endfor

  return 0;
}

//:: #endfor

//:: for t_name, t in tables.items():
//::   name = pd_prefix + t.cname + "_get_entry_count"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t dev_id,
 uint32_t *count
) {
  assert(my_devices[dev_id]);
  auto client = pd_client(dev_id);
  *count = 0;
  try {
    *count = static_cast<uint32_t>(
        client.c->bm_mt_get_num_entries(0, "${t_name}"));
  } catch (InvalidTableOperation &ito) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(ito.code)->second;
    std::cout << "Invalid table (" << "${t_name}" << ") operation ("
	      << ito.code << "): " << what << std::endl;
    return ito.code;
  }
  return 0;
}

//:: #endfor

/* DIRECT COUNTERS */

/* legacy code, to be removed at some point */

//:: for t_name, t in tables.items():
//::   if not t.with_counters: continue
//::   name = pd_prefix + t.cname + "_read_counter"
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
  pd_client(dev_tgt.device_id).c->bm_mt_read_counter(
      value, 0, "${t_name}", entry_hdl);
  counter_value->bytes = (uint64_t) value.bytes;
  counter_value->packets = (uint64_t) value.packets;
  return 0;
}

//::   name = pd_prefix + t.cname + "_reset_counters"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
) {
  assert(my_devices[dev_tgt.device_id]);
  pd_client(dev_tgt.device_id).c->bm_mt_reset_counters(
      0, "${t_name}");
  return 0;
}

//:: #endfor

//:: for t_name, t in tables.items():
//::   if not t.support_timeout: continue
//::   p4_pd_enable_hit_state_scan = "_".join([pd_prefix[:-1], t.cname, "enable_hit_state_scan"])
//::   p4_pd_get_hit_state = "_".join([pd_prefix[:-1], t.cname, "get_hit_state"])
//::   p4_pd_set_entry_ttl = "_".join([pd_prefix[:-1], t.cname, "set_entry_ttl"])
//::   p4_pd_enable_entry_timeout = "_".join([pd_prefix[:-1], t.cname, "enable_entry_timeout"])
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

/* Clean all state */
//:: name = pd_prefix + "clean_all"
p4_pd_status_t
${name}(p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt) {
  assert(my_devices[dev_tgt.device_id]);
  pd_client(dev_tgt.device_id).c->bm_reset_state();
  return 0;
}

}
