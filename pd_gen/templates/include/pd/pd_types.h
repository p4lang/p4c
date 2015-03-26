#ifndef _P4_PD_TYPES_H_
#define _P4_PD_TYPES_H_

/* MATCH STRUCTS */

//:: for t_name, t in tables.items():
//::   t_name = get_c_name(t_name)
//::   if not t.key:
/* ${t_name} has no match fields */

//::     continue
//::   #endif
//::   match_params = gen_match_params(t.key)
typedef struct ${pd_prefix}${t_name}_match_spec {
//::   for name, width in match_params:
//::     c_name = get_c_name(name)
//::     if width > 4:
  uint8_t ${c_name}[${width}];
//::     else:
//::       type_ = get_c_type(width)
  ${type_} ${c_name};
//::     #endif
//::   #endfor
} ${pd_prefix}${t_name}_match_spec_t;

//:: #endfor


/* ACTION STRUCTS */

//:: for a_name, a in actions.items():
//::   a_name = get_c_name(a_name)
//::   if not a.runtime_data:
/* ${a_name} has no parameters */

//::     continue
//::   #endif
//::   action_params = gen_action_params(a.runtime_data)
typedef struct ${pd_prefix}${a_name}_action_spec {
//::   for name, width in action_params:
//::     c_name = get_c_name(name)
//::     if width > 4:
  uint8_t ${c_name}[${width}];
//::     else:
//::       type_ = get_c_type(width)
  ${type_} ${c_name};
//::     #endif
//::   #endfor
} ${pd_prefix}${a_name}_action_spec_t;

//:: #endfor

#endif
