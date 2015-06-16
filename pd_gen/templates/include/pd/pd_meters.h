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

// direct meters not supported yet

//:: for ma_name, ma in meter_arrays.items():
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   params += ["int index"]
//::   if ma.type_ == MeterType.PACKETS:
//::     params += ["uint32_t cir_pps", "uint32_t cburst_pkts",
//::                "uint32_t pir_pps", "uint32_t pburst_pkts"]
//::   else:
//::     params += ["uint32_t cir_kbps", "uint32_t cburst_kbits",
//::                "uint32_t pir_kbps", "uint32_t pburst_kbits"]
//::   #endif
//::   param_str = ",\n ".join(params)
//::   name = pd_prefix + "meter_configure_" + ma_name
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

