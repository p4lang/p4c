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

#ifndef _P4_PD_TYPES_H_
#define _P4_PD_TYPES_H_

#include <stdint.h>

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
