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

#include <vector>

#include "pd/pd_types.h"
#include "pd/pd_static.h"
#include "pd_conn_mgr.h"

extern pd_conn_mgr_t *conn_mgr_state;
extern int *my_devices;

extern "C" {

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
) {
  double info_rate;
  uint32_t burst_size;
  BmMeterRateConfig rate;

  std::vector<BmMeterRateConfig> rates;

//::     if ma.type_ == MeterType.PACKETS:
  info_rate = (double) cir_pps / 1000000.;
  burst_size = cburst_pkts;
//::     else:
  info_rate = (double) cir_kbps / 8000.; // bytes per microsecond
  burst_size = cburst_kbits * 1000 / 8;
//::     #endif

  rate.units_per_micros = info_rate; rate.burst_size = burst_size;
  rates.push_back(rate);

//::     if ma.type_ == MeterType.PACKETS:
  info_rate = (double) pir_pps / 1000000.;
  burst_size = pburst_pkts;
//::     else:
  info_rate = (double) pir_kbps / 8000.;
  burst_size = pburst_kbits * 1000 / 8;
//::     #endif

  rate.units_per_micros = info_rate; rate.burst_size = burst_size;
  rates.push_back(rate);

  assert(my_devices[dev_tgt.device_id]);
  pd_conn_mgr_client(conn_mgr_state, dev_tgt.device_id)->bm_meter_set_rates(
    "${ma_name}", index, rates
  );

  return 0;
}

//:: #endfor

}
