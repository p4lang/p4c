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

#ifndef _P4_PD_LEARNING_H_
#define _P4_PD_LEARNING_H_

#include "pd/pd_types.h"
#include "pd/pd_common.h"

#ifdef __cplusplus
extern "C" {
#endif

p4_pd_status_t ${pd_prefix}set_learning_timeout
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id,
 uint64_t usecs
);

//:: for lq_name, lq in learn_quantas.items():
//::   lq_name = get_c_name(lq_name)
typedef struct ${pd_prefix}${lq_name}_digest_entry {
//::   for name, bit_width in lq.fields:
//::     c_name = get_c_name(name)
//::     width = (bit_width + 7) / 8
//::     if width > 4:
  uint8_t ${c_name}[${width}];
//::     else:
//::       type_ = get_c_type(width)
  ${type_} ${c_name};
//::     #endif
//::   #endfor
} ${pd_prefix}${lq_name}_digest_entry_t;

typedef struct ${pd_prefix}${lq_name}_digest_msg {
  p4_pd_dev_target_t dev_tgt;
  uint16_t num_entries;
  ${pd_prefix}${lq_name}_digest_entry_t *entries;
  uint64_t buffer_id; // added by me, needed for BMv2 compatibility
} ${pd_prefix}${lq_name}_digest_msg_t;

typedef p4_pd_status_t
(*${pd_prefix}${lq_name}_digest_notify_cb)(p4_pd_sess_hdl_t,
					   ${pd_prefix}${lq_name}_digest_msg_t *,
					   void *cookie);

p4_pd_status_t ${pd_prefix}${lq_name}_register
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id,
 ${pd_prefix}${lq_name}_digest_notify_cb cb_fn,
 void *cb_fn_cookie
);

p4_pd_status_t ${pd_prefix}${lq_name}_deregister
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id
);

p4_pd_status_t ${pd_prefix}${lq_name}_notify_ack
(
 p4_pd_sess_hdl_t sess_hdl,
 ${pd_prefix}${lq_name}_digest_msg_t *msg
);

//:: #endfor

#ifdef __cplusplus
}
#endif

#endif
