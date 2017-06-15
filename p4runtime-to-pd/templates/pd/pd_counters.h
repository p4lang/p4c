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

#ifndef _P4_PD_COUNTERS_H_
#define _P4_PD_COUNTERS_H_

#include <bm/pdfixed/pd_common.h>

#ifdef __cplusplus
extern "C" {
#endif

//:: for ca_name, ca in counter_arrays.items():
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if ca.is_direct:
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     params += ["int index"]
//::   #endif
//::   params += ["int flags"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + "counter_read_" + ca.cname
p4_pd_counter_value_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if ca.is_direct:
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     params += ["int index"]
//::   #endif
//::   params += ["p4_pd_counter_value_t counter_value"]
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + "counter_write_" + ca.cname
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   name = pd_prefix + "counter_hw_sync_" + ca.cname
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_stat_sync_cb cb_fn,
 void *cb_cookie
);

//:: #endfor

#ifdef __cplusplus
}
#endif

#endif
