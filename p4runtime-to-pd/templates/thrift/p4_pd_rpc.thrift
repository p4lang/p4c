# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# P4 Thrift RPC Input

//:: api_prefix = p4_prefix + "_"

include "res.thrift"

namespace py p4_pd_rpc
namespace cpp p4_pd_rpc
namespace c_glib p4_pd_rpc

typedef i32 EntryHandle_t
typedef i32 MemberHandle_t
typedef i32 GroupHandle_t
typedef binary MacAddr_t
typedef binary IPv6_t

struct ${api_prefix}counter_value_t {
  1: required i64 packets;
  2: required i64 bytes;
}

struct ${api_prefix}counter_flags_t {
  1: required bool read_hw_sync;
}

struct ${api_prefix}packets_meter_spec_t {
  1: required i32 cir_pps;
  2: required i32 cburst_pkts;
  3: required i32 pir_pps;
  4: required i32 pburst_pkts;
  5: required bool color_aware;  // ignored for now
}

struct ${api_prefix}bytes_meter_spec_t {
  1: required i32 cir_kbps;
  2: required i32 cburst_kbits;
  3: required i32 pir_kbps;
  4: required i32 pburst_kbits;
  5: required bool color_aware;  // ignored for now
}

# Match structs

//:: for t_name, t in tables.items():
//::   if not t.key:
/* ${t_name} has no match fields */

//::     continue
//::   #endif
//::   match_params = gen_match_params(t.key)
struct ${api_prefix}${t.cname}_match_spec_t {
//::   id = 1
//::   for name, width in match_params:
//::     c_name = get_c_name(name)
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${c_name};
//::   id += 1
//::   #endfor
}

//:: #endfor


# Action structs

//:: for a_name, a in actions.items():
//::   if not a.runtime_data:
/* ${a_name} has no parameters */

//::     continue
//::   #endif
//::   action_params = gen_action_params(a.runtime_data)
struct ${api_prefix}${a.cname}_action_spec_t {
//::   id = 1
//::   for name, width in action_params:
//::     c_name = get_c_name(name)
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${name};
//::     id += 1
//::   #endfor
}

//:: #endfor


//:: for lq_name, lq in learn_quantas.items():
//::   rpc_msg_type = api_prefix + lq.cname + "_digest_msg_t"
//::   rpc_entry_type = api_prefix + lq.cname + "_digest_entry_t"
struct ${rpc_entry_type} {
//::   count = 1
//::   for name, bit_width in lq.fields:
//::     c_name = get_c_name(name)
//::     width = (bit_width + 7) / 8
//::     if width > 4:
//::       field_definition = "list<byte> %s" % c_name
//::     else:
//::       field_definition = "%s %s" % (get_thrift_type(width), c_name)
//::     #endif
  ${count}: required ${field_definition};
//::   count += 1
//::   #endfor
}

struct ${rpc_msg_type} {
  1: required res.DevTarget_t             dev_tgt;
  2: required list<${rpc_entry_type}> msg;
  3: required i64                     msg_ptr;
}
//:: #endfor

//:: def get_direct_parameter_specs(d, t, api_prefix):
//::   for k in d:
//::     exec "%s=d[k]" % k
//::   #endfor
//::   specs = []
//::   if t.direct_meters:
//::     m_name = t.direct_meters
//::     m = meter_arrays[m_name]
//::     if m.type_ == MeterType.PACKETS:
//::       specs += [api_prefix + "packets_meter_spec_t " + m.cname + "_spec"]
//::     else:
//::       specs += [api_prefix + "bytes_meter_spec_t " + m.cname + "_spec"]
//::     #endif
//::   #endif
//::   return specs
//:: #enddef

service ${p4_prefix} {

    # Table entry add functions
//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     if has_match_spec:
//::       params += [api_prefix + t.cname + "_match_spec_t match_spec"]
//::     #endif
//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::       params += ["i32 priority"]
//::     #endif
//::     if has_action_spec:
//::       params += [api_prefix + a.cname + "_action_spec_t action_spec"]
//::     #endif
//::     if t.support_timeout:
//::       params += ["i32 ttl"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t, api_prefix)
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = t.cname + "_table_add_with_" + a.cname
    EntryHandle_t ${name}(${param_str});
//::   #endfor
//:: #endfor

    # Table entry modify functions
//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "EntryHandle_t entry"]
//::     if has_action_spec:
//::       params += [api_prefix + a.cname + "_action_spec_t action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t, api_prefix)
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = t.cname + "_table_modify_with_" + a.cname
    i32 ${name}(${param_str});
//::   #endfor
//:: #endfor

    # Table entry delete functions
//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   name = t.cname + "_table_delete"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str});
//:: #endfor

    # Table set default action functions
//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     if has_action_spec:
//::       params += [api_prefix + a.cname + "_action_spec_t action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t, api_prefix)
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = t.cname + "_set_default_action_" + a.cname
    EntryHandle_t ${name}(${param_str});
//::   #endfor
//:: #endfor

    # Table clear default action functions
//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = t.cname + "_table_reset_default_entry"
    void ${name}(${param_str});
//:: #endfor

    # INDIRECT ACTION DATA AND MATCH SELECT

//:: for ap_name, act_prof in action_profs.items():
//::   for a_name, a in act_prof.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     if has_action_spec:
//::       params += [api_prefix + a.cname + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof.cname + "_add_member_with_" + a.cname
    MemberHandle_t ${name}(${param_str});

//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::		   "MemberHandle_t mbr"]
//::     if has_action_spec:
//::       params += [api_prefix + a.cname + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof.cname + "_modify_member_with_" + a.cname
    i32 ${name}(${param_str});
//::   #endfor

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_del_member"
    i32 ${name}(${param_str});

//::   if not act_prof.with_selection: continue
//::
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "i16 max_grp_size"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_create_group"
    GroupHandle_t ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_del_group"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_add_member_to_group"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_del_member_from_group"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_deactivate_group_member"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof.cname + "_reactivate_group_member"
    i32 ${name}(${param_str});

//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   if has_match_spec:
//::     params += [api_prefix + t.cname + "_match_spec_t match_spec"]
//::   #endif
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::     params += ["i32 priority"]
//::   #endif
//::
//::   params_wo = params + ["MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::   param_str = ", ".join(param_list)
//::   name = t.cname + "_add_entry"
    EntryHandle_t ${name}(${param_str});
//::
//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_w = params + ["GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::   param_str = ", ".join(param_list)
//::   name = t.cname + "_add_entry_with_selector"
    EntryHandle_t ${name}(${param_str});
//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   params_wo = params + ["MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::   param_str = ", ".join(param_list)
//::   name = t.cname + "_set_default_entry"
    EntryHandle_t ${name}(${param_str});
//::
//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_w = params + ["GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::   param_str = ", ".join(param_list)
//::   name = t.cname + "_set_default_entry_with_selector"
    EntryHandle_t ${name}(${param_str});
//:: #endfor

//:: for t_name, t in tables.items():
//::   name = t.cname + "_get_entry_count"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str});
//:: #endfor


    # clean all state
//:: name = "clean_all"
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);

    # clean table state
//:: name = "tables_clean_all"
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);

    # counters

//:: for ca_name, ca in counter_arrays.items():
//::   if ca.is_direct == "direct":
//::     name = "counter_read_" + ca.cname
    ${api_prefix}counter_value_t ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:EntryHandle_t entry, 4:${api_prefix}counter_flags_t flags);
//::     name = "counter_write_" + ca.cname
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:EntryHandle_t entry, 4:${api_prefix}counter_value_t counter_value);

//::   else:
//::     name = "counter_read_" + ca.cname
    ${api_prefix}counter_value_t ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${api_prefix}counter_flags_t flags);
//::     name = "counter_write_" + ca.cname
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${api_prefix}counter_value_t counter_value);

//::   #endif
//:: #endfor

//:: for ca_name, ca in counter_arrays.items():
//::   name = "counter_hw_sync_" + ca.cname
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);
//:: #endfor

    # meters

//:: for ma_name, ma in meter_arrays.items():
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   if ma.is_direct:
//::     params += ["EntryHandle_t entry"]
//::   else:
//::     params += ["i32 index"]
//::   #endif
//::   if ma.type_ == MeterType.PACKETS:
//::     params += [api_prefix + "packets_meter_spec_t meter_spec"]
//::   else:
//::     params += [api_prefix + "bytes_meter_spec_t meter_spec"]
//::   #endif
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   
//::   name = "meter_set_" + ma.cname
    i32 ${name}(${param_str});

//:: #endfor

    # registers

//:: for ra_name, ra in register_arrays.items():
//::   name = "register_reset_" + ra.cname
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);

//:: #endfor

    void set_learning_timeout(1: res.SessionHandle_t sess_hdl, 2: byte dev_id, 3: i32 msecs);

//:: for lq_name, lq in learn_quantas.items():
//::   rpc_msg_type = api_prefix + lq.cname + "_digest_msg_t"
    void ${lq.cname}_register(1: res.SessionHandle_t sess_hdl, 2: byte dev_id);
    void ${lq.cname}_deregister(1: res.SessionHandle_t sess_hdl, 2: byte dev_id);
    ${rpc_msg_type} ${lq.cname}_get_digest(1: res.SessionHandle_t sess_hdl);
    void ${lq.cname}_digest_notify_ack(1: res.SessionHandle_t sess_hdl, 2: i64 msg_ptr);
//:: #endfor
}
