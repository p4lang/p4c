//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: api_prefix = p4_prefix + "_"

#include "p4_prefix.h"

#include <iostream>

#include <string.h>

#include "pd/pd.h"

#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>

using namespace  ::p4_pd_rpc;
using namespace  ::res_pd_rpc;

namespace {

void bytes_meter_spec_thrift_to_pd(
    const ${api_prefix}bytes_meter_spec_t &meter_spec,
    p4_pd_bytes_meter_spec_t *pd_meter_spec) {
  pd_meter_spec->cir_kbps = meter_spec.cir_kbps;
  pd_meter_spec->cburst_kbits = meter_spec.cburst_kbits;
  pd_meter_spec->pir_kbps = meter_spec.pir_kbps;
  pd_meter_spec->pburst_kbits = meter_spec.pburst_kbits;
  pd_meter_spec->meter_type = meter_spec.color_aware ?
      PD_METER_TYPE_COLOR_AWARE : PD_METER_TYPE_COLOR_UNAWARE;
}

void packets_meter_spec_thrift_to_pd(
    const ${api_prefix}packets_meter_spec_t &meter_spec,
    p4_pd_packets_meter_spec_t *pd_meter_spec) {
  pd_meter_spec->cir_pps = meter_spec.cir_pps;
  pd_meter_spec->cburst_pkts = meter_spec.cburst_pkts;
  pd_meter_spec->pir_pps = meter_spec.pir_pps;
  pd_meter_spec->pburst_pkts = meter_spec.pburst_pkts;
   pd_meter_spec->meter_type = meter_spec.color_aware ?
       PD_METER_TYPE_COLOR_AWARE : PD_METER_TYPE_COLOR_UNAWARE;
}

}  // namespace

//:: def get_direct_parameter_specs(d, t, api_prefix):
//::   for k in d:
//::     exec "%s=d[k]" % k
//::   #endfor
//::   specs = []
//::   if t.direct_meters:
//::     m_name = t.direct_meters
//::     m = meter_arrays[m_name]
//::     if m.type_ == MeterType.PACKETS:
//::       specs += ["const " + api_prefix + "packets_meter_spec_t &" + m.cname + "_spec"]
//::     else:
//::       specs += ["const " + api_prefix + "bytes_meter_spec_t &" + m.cname + "_spec"]
//::     #endif
//::   #endif
//::   return specs
//:: #enddef

class ${p4_prefix}Handler : virtual public ${p4_prefix}If {
private:
  class CbWrap {
    CbWrap() {}

    int wait() {
      std::unique_lock<std::mutex> lock(cb_mutex);
      while(cb_status == 0) {
        cb_condvar.wait(lock);
      }
      return 0;
    }

    void notify() {
      std::unique_lock<std::mutex> lock(cb_mutex);
      assert(cb_status == 0);
      cb_status = 1;
      cb_condvar.notify_one();
    }

    static void cb_fn(int device_id, void *cookie) {
      (void) device_id;
      CbWrap *inst = static_cast<CbWrap *>(cookie);
      inst->notify();
    }

    CbWrap(const CbWrap &other) = delete;
    CbWrap &operator=(const CbWrap &other) = delete;

    CbWrap(CbWrap &&other) = delete;
    CbWrap &operator=(CbWrap &&other) = delete;

   private:
    std::mutex cb_mutex{};
    std::condition_variable cb_condvar{};
    int cb_status{0};
  };

public:
    ${p4_prefix}Handler() {
//:: for lq_name, lq in learn_quantas.items():
      pthread_mutex_init(&${lq.cname}_mutex, NULL);
//:: #endfor
    }

    // Table entry add functions

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     if has_match_spec:
//::       params += ["const " + api_prefix + t.cname + "_match_spec_t &match_spec"]
//::     #endif
//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::       params += ["const int32_t priority"]
//::     #endif
//::     if has_action_spec:
//::       params += ["const " + api_prefix + a.cname + "_action_spec_t &action_spec"]
//::     #endif
//::     if t.support_timeout:
//::       params += ["const int32_t ttl"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t, api_prefix)
//::     param_str = ", ".join(params)
//::     name = t.cname + "_table_add_with_" + a.cname
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::     if has_match_spec:
        ${pd_prefix}${t.cname}_match_spec_t pd_match_spec;
//::       match_params = gen_match_params(t.key)
//::       for name, width in match_params:
//::         name = get_c_name(name)
//::         if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::         else:
	memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
//::     if has_action_spec:
        ${pd_prefix}${a.cname}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a.runtime_data)
//::       for name, width in action_params:
//::         name = get_c_name(name)
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_entry_hdl_t pd_entry;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_match_spec:
//::       pd_params += ["&pd_match_spec"]
//::     #endif
//::     if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::       pd_params += ["priority"]
//::     #endif
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     if t.support_timeout:
//::       pd_params += ["(uint32_t)ttl"]
//::     #endif
//::     # direct parameter specs
//::     if t.direct_meters:
//::       m_name = t.direct_meters
//::       m = meter_arrays[m_name]
//::       type_name = MeterType.to_str(m.type_)
        p4_pd_${type_name}_meter_spec_t pd_${m.cname}_spec;
        ${type_name}_meter_spec_thrift_to_pd(${m.cname}_spec, &pd_${m.cname}_spec);
//::       pd_params += ["&pd_" + m.cname + "_spec"]
//::     #endif
//::     pd_params += ["&pd_entry"]
//::     pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_entry;
    }

//::   #endfor
//:: #endfor


    // Table entry modify functions

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const EntryHandle_t entry"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + a.cname + "_action_spec_t &action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t, api_prefix)
//::     param_str = ", ".join(params)
//::     name = t.cname + "_table_modify_with_" + a.cname
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

//::     if has_action_spec:
        ${pd_prefix}${a.cname}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a.runtime_data)
//::       for name, width in action_params:
//::         name = get_c_name(name)
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif

//::     pd_params = ["sess_hdl", "dev_id", "entry"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     # direct parameter specs
//::     if t.direct_meters:
//::       m_name = t.direct_meters
//::       m = meter_arrays[m_name]
//::       type_name = MeterType.to_str(m.type_)
        p4_pd_${type_name}_meter_spec_t pd_${m.cname}_spec;
        ${type_name}_meter_spec_thrift_to_pd(${m.cname}_spec, &pd_${m.cname}_spec);
//::       pd_params += ["&pd_" + m.cname + "_spec"]
//::     #endif
//::     pd_param_str = ", ".join(pd_params)
        return ${pd_name}(${pd_param_str});
    }

//::   #endfor
//:: #endfor


    // Table entry delete functions

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   name = t.cname + "_table_delete"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, entry);
    }

//:: #endfor

    // set default action

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   for a_name, a in t.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + a.cname + "_action_spec_t &action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(render_dict, t, api_prefix)
//::     param_str = ", ".join(params)
//::     name = t.cname + "_set_default_action_" + a.cname
//::     pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::     if has_action_spec:
        ${pd_prefix}${a.cname}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a.runtime_data)
//::       for name, width in action_params:
//::         name = get_c_name(name)
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_entry_hdl_t pd_entry;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     # direct parameter specs
//::     if t.direct_meters:
//::       m_name = t.direct_meters
//::       m = meter_arrays[m_name]
//::       type_name = MeterType.to_str(m.type_)
        p4_pd_${type_name}_meter_spec_t pd_${m.cname}_spec;
        ${type_name}_meter_spec_thrift_to_pd(${m.cname}_spec, &pd_${m.cname}_spec);
//::       pd_params += ["&pd_" + m.cname + "_spec"]
//::     #endif
//::     pd_params += ["&pd_entry"]
//::     pd_param_str = ", ".join(pd_params)
        return ${pd_name}(${pd_param_str});

        // return pd_entry;
    }

//::   #endfor
//:: #endfor

    // clear default action

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type != TableType.SIMPLE: continue
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
//::   name = t.cname + "_table_reset_default_entry"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

        ${pd_name}(sess_hdl, pd_dev_tgt);
    }

//:: #endfor


//:: name = "clean_all"
//:: pd_name = pd_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(sess_hdl, pd_dev_tgt);
  }

//:: name = "tables_clean_all"
//:: pd_name = pd_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      assert(0);

      return 0;
  }

    // INDIRECT ACTION DATA AND MATCH SELECT

//:: for ap_name, act_prof in action_profs.items():
//::   for a_name, a in act_prof.actions.items():
//::     has_action_spec = len(a.runtime_data) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + a.cname + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = act_prof.cname + "_add_member_with_" + a.cname
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::     if has_action_spec:
        ${pd_prefix}${a.cname}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a.runtime_data)
//::       for name, width in action_params:
//::         name = get_c_name(name)
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_mbr_hdl_t pd_mbr_hdl;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_params += ["&pd_mbr_hdl"]
//::     pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_mbr_hdl;
    }

//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const MemberHandle_t mbr"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + a.cname + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = act_prof.cname + "_modify_member_with_" + a.cname
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

//::     if has_action_spec:
        ${pd_prefix}${a.cname}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a.runtime_data)
//::       for name, width in action_params:
//::         name = get_c_name(name)
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif

//::     pd_params = ["sess_hdl", "dev_id", "mbr"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_param_str = ", ".join(pd_params)
        return ${pd_name}(${pd_param_str});
    }

//::   #endfor

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_del_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, mbr);
    }

//::   if not act_prof.with_selection: continue
//::
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const int16_t max_grp_size"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_create_group"
//::   pd_name = pd_prefix + name
    GroupHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

	p4_pd_grp_hdl_t pd_grp_hdl;

        ${pd_name}(sess_hdl, pd_dev_tgt, max_grp_size, &pd_grp_hdl);
	return pd_grp_hdl;
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_del_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_add_member_to_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp, mbr);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_del_member_from_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp, mbr);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_deactivate_group_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return 0;
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof.cname + "_reactivate_group_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return 0;
    }

//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   match_type = t.match_type
//::   has_match_spec = len(t.key) > 0
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   if has_match_spec:
//::     params += ["const " + api_prefix + t.cname + "_match_spec_t &match_spec"]
//::   #endif
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::     params += ["const int32_t priority"]
//::   #endif
//::   params_wo = params + ["const MemberHandle_t mbr"]
//::   param_str = ", ".join(params_wo)
//::   name = t.cname + "_add_entry"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::   if has_match_spec:
        ${pd_prefix}${t.cname}_match_spec_t pd_match_spec;
//::     match_params = gen_match_params(t.key)
//::     for name, width in match_params:
//::       name = get_c_name(name)
//::       if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::       else:
	memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::       #endif
//::     #endfor

//::   #endif
        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if has_match_spec:
//::     pd_params += ["&pd_match_spec"]
//::   #endif
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["mbr", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_entry;
    }

//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_w = params + ["const GroupHandle_t grp"]
//::   param_str = ", ".join(params_w)
//::   name = t.cname + "_add_entry_with_selector"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::   if has_match_spec:
        ${pd_prefix}${t.cname}_match_spec_t pd_match_spec;
//::     match_params = gen_match_params(t.key)
//::     for name, width in match_params:
//::       name = get_c_name(name)
//::       if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::       else:
	memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::       #endif
//::     #endfor

//::   #endif
        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if has_match_spec:
//::     pd_params += ["&pd_match_spec"]
//::   #endif
//::   if match_type in {MatchType.TERNARY, MatchType.RANGE}:
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["grp", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_entry;
    }

//:: #endfor

//:: for t_name, t in tables.items():
//::   t_type = t.type_
//::   if t_type == TableType.SIMPLE: continue
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   params_wo = params + ["const MemberHandle_t mbr"]
//::   param_str = ", ".join(params_wo)
//::   name = t.cname + "_set_default_entry"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   pd_params += ["mbr", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});

        return pd_entry;
    }

//::   if t_type != TableType.INDIRECT_WS: continue
//::   params_w = params + ["const GroupHandle_t grp"]
//::   param_str = ", ".join(params_w)
//::   name = t.cname + "_set_default_entry_with_selector"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   pd_params += ["grp", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});

        return pd_entry;
    }
//:: #endfor

//:: for t_name, t in tables.items():
//::   name = t.cname + "_get_entry_count"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        uint32_t count = 0;

        int status = ${pd_name}(sess_hdl, dev_id, &count);
        if(status != 0) return -1;
        return static_cast<int32_t>(count);
    }

//:: #endfor

    // COUNTERS

//:: for ca_name, ca in counter_arrays.items():
//::   if ca.is_direct == "direct":
//::     name = "counter_read_" + ca.cname
//::     pd_name = pd_prefix + name
    void ${name}(${api_prefix}counter_value_t &counter_value, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}counter_flags_t &flags) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= COUNTER_READ_HW_SYNC;

      p4_pd_counter_value_t value = ${pd_name}(sess_hdl, pd_dev_tgt, entry, pd_flags);
      counter_value.packets = value.packets;
      counter_value.bytes = value.bytes;
    }

//::     name = "counter_write_" + ca.cname
//::     pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}counter_value_t &counter_value) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      p4_pd_counter_value_t value;
      value.packets = counter_value.packets;
      value.bytes = counter_value.bytes;

      return ${pd_name}(sess_hdl, pd_dev_tgt, entry, value);
    }

//::   else:
//::     name = "counter_read_" + ca.cname
//::     pd_name = pd_prefix + name
    void ${name}(${api_prefix}counter_value_t &counter_value, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}counter_flags_t &flags) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= COUNTER_READ_HW_SYNC;

      p4_pd_counter_value_t value = ${pd_name}(sess_hdl, pd_dev_tgt, index, pd_flags);
      counter_value.packets = value.packets;
      counter_value.bytes = value.bytes;
    }

//::     name = "counter_write_" + ca.cname
//::     pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}counter_value_t &counter_value) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      p4_pd_counter_value_t value;
      value.packets = counter_value.packets;
      value.bytes = counter_value.bytes;

      return ${pd_name}(sess_hdl, pd_dev_tgt, index, value);
    }

//::   #endif
//:: #endfor

//:: for ca_name, ca in counter_arrays.items():
//::   name = "counter_hw_sync_" + ca.cname
//::   pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      return 0;
    }

//:: #endfor

  // METERS

//:: for ma_name, ma in meter_arrays.items():
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if ma.is_direct:
//::     params += ["const EntryHandle_t entry"]
//::     pd_params += ["entry"]
//::   else:
//::     params += ["const int32_t index"]
//::     pd_params += ["index"]
//::   #endif
//::   if ma.type_ == MeterType.PACKETS:
//::     params += ["const " + api_prefix + "packets_meter_spec_t &meter_spec"]
//::   else:
//::     params += ["const " + api_prefix + "bytes_meter_spec_t &meter_spec"]
//::   #endif
//::   pd_params += ["&pd_meter_spec"]
//::   param_str = ", ".join(params)
//::
//::   pd_param_str = ", ".join(pd_params)
//::
//::   name = "meter_set_" + ma.cname
//::   pd_name = pd_prefix + name
  int32_t ${name}(${param_str}) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::   if ma.type_ == MeterType.PACKETS:
      p4_pd_packets_meter_spec_t pd_meter_spec;
      packets_meter_spec_thrift_to_pd(meter_spec, &pd_meter_spec);
//::   else:
      p4_pd_bytes_meter_spec_t pd_meter_spec;
      bytes_meter_spec_thrift_to_pd(meter_spec, &pd_meter_spec);
//::   #endif

      return ${pd_name}(${pd_param_str});
  }

//:: #endfor

  // REGISTERS

//:: for ra_name, ra in register_arrays.items():
//::   name = "register_reset_" + ra.cname
//::   pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(sess_hdl, pd_dev_tgt);
    }

//:: #endfor

  void set_learning_timeout(const SessionHandle_t sess_hdl, const int8_t dev_id, const int32_t msecs) {
      ${pd_prefix}set_learning_timeout(sess_hdl, (const uint8_t)dev_id, msecs);
  }

//:: for lq_name, lq in learn_quantas.items():
//::   rpc_msg_type = api_prefix + lq.cname + "_digest_msg_t"
//::   rpc_entry_type = api_prefix + lq.cname + "_digest_entry_t"
  std::map<SessionHandle_t, std::list<${rpc_msg_type}> > ${lq.cname}_digest_queues;
  pthread_mutex_t ${lq.cname}_mutex;

  p4_pd_status_t
  ${lq.cname}_receive(const SessionHandle_t sess_hdl,
                        const ${rpc_msg_type} &msg) {
    pthread_mutex_lock(&${lq.cname}_mutex);
    assert(${lq.cname}_digest_queues.find(sess_hdl) != ${lq.cname}_digest_queues.end());
    std::map<SessionHandle_t, std::list<${rpc_msg_type}> >::iterator digest_queue = ${lq.cname}_digest_queues.find(sess_hdl);
    digest_queue->second.push_back(msg);
    pthread_mutex_unlock(&${lq.cname}_mutex);

    return 0;
  }

  static p4_pd_status_t
  ${p4_prefix}_${lq.cname}_cb(p4_pd_sess_hdl_t sess_hdl,
                              ${pd_prefix}${lq.cname}_digest_msg_t *msg,
                             void *cookie) {
    ${pd_prefix}${lq.cname}_digest_msg_t *msg_ = new ${pd_prefix}${lq.cname}_digest_msg_t();
    *msg_ = *msg;
    ${rpc_msg_type} rpc_msg;
    rpc_msg.msg_ptr = (int64_t)msg_;
    rpc_msg.dev_tgt.dev_id = msg->dev_tgt.device_id;
    rpc_msg.dev_tgt.dev_pipe_id = msg->dev_tgt.dev_pipe_id;
    for (int i = 0; (msg != NULL) && (i < msg->num_entries); ++i) {
      ${rpc_entry_type} entry;
//::   for name, bit_width in lq.fields:
//::     c_name = get_c_name(name)
//::     width = (bit_width + 7) / 8
//::     if width > 4:
      entry.${c_name}.insert(entry.${c_name}.end(), msg->entries[i].${c_name}, msg->entries[i].${c_name} + ${width});
//::     else:
      entry.${c_name} = msg->entries[i].${c_name};
//::     #endif
//::   #endfor
      rpc_msg.msg.push_back(entry);
    }
    return ((${p4_prefix}Handler*)cookie)->${lq.cname}_receive((SessionHandle_t)sess_hdl, rpc_msg);
  }

  void ${lq.cname}_register( const SessionHandle_t sess_hdl, const int8_t dev_id) {
    ${pd_prefix}${lq.cname}_register(sess_hdl, dev_id, ${p4_prefix}_${lq.cname}_cb, this);
    pthread_mutex_lock(&${lq.cname}_mutex);
    ${lq.cname}_digest_queues.insert(std::pair<SessionHandle_t, std::list<${rpc_msg_type}> >(sess_hdl, std::list<${rpc_msg_type}>()));
    pthread_mutex_unlock(&${lq.cname}_mutex);
  }

  void ${lq.cname}_deregister(const SessionHandle_t sess_hdl, const int8_t dev_id) {
    ${pd_prefix}${lq.cname}_deregister(sess_hdl, dev_id);
    pthread_mutex_lock(&${lq.cname}_mutex);
    ${lq.cname}_digest_queues.erase(sess_hdl);
    pthread_mutex_unlock(&${lq.cname}_mutex);
  }

  void ${lq.cname}_get_digest(${rpc_msg_type} &msg, const SessionHandle_t sess_hdl) {
    msg.msg_ptr = 0;
    msg.msg.clear();

    pthread_mutex_lock(&${lq.cname}_mutex);
    std::map<SessionHandle_t, std::list<${rpc_msg_type}> >::iterator digest_queue = ${lq.cname}_digest_queues.find(sess_hdl);
    if (digest_queue != ${lq.cname}_digest_queues.end()) {
      if (digest_queue->second.size() > 0) {
        msg = digest_queue->second.front();
        digest_queue->second.pop_front();
      }
    }

    pthread_mutex_unlock(&${lq.cname}_mutex);
  }

  void ${lq.cname}_digest_notify_ack(const SessionHandle_t sess_hdl, const int64_t msg_ptr) {
    ${pd_prefix}${lq.cname}_digest_msg_t *msg = (${pd_prefix}${lq.cname}_digest_msg_t *) msg_ptr;
    ${pd_prefix}${lq.cname}_notify_ack((p4_pd_sess_hdl_t)sess_hdl, msg);
    delete msg;
  }
//:: #endfor
};
