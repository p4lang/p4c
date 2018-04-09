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

#ifndef _P4_PD_METERS_H_
#define _P4_PD_METERS_H_

#ifdef __cplusplus
extern "C" {
#endif

//:: for ma_name, ma in meter_arrays.items():
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if ma.is_direct:
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     params += ["int index"]
//::   #endif
//::   if ma.type_ == MeterType.PACKETS:
//::     params += ["p4_pd_packets_meter_spec_t *meter_spec"]
//::   else:
//::     params += ["p4_pd_bytes_meter_spec_t *meter_spec"]
//::   #endif
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + "meter_set_" + ma.cname
p4_pd_status_t
${name}
(
${param_str}
);

//:: #endfor

#ifdef __cplusplus
}
#endif

#endif
