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

#include <bm/pdfixed/int/pd_helpers.h>
#include <bm/pdfixed/int/pd_conn_mgr.h>

#include <vector>

std::vector<BmMeterRateConfig>
pd_bytes_meter_spec_to_rates(p4_pd_bytes_meter_spec_t *meter_spec) {
  double info_rate;
  uint32_t burst_size;
  BmMeterRateConfig rate;

  std::vector<BmMeterRateConfig> rates;

  // bytes per microsecond
  info_rate = static_cast<double>(meter_spec->cir_kbps) / 8000.;
  burst_size = meter_spec->cburst_kbits * 1000 / 8;
  rate.units_per_micros = info_rate; rate.burst_size = burst_size;
  rates.push_back(rate);

  info_rate = static_cast<double>(meter_spec->pir_kbps) / 8000.;
  burst_size = meter_spec->pburst_kbits * 1000 / 8;
  rate.units_per_micros = info_rate; rate.burst_size = burst_size;
  rates.push_back(rate);

  return rates;
}

std::vector<BmMeterRateConfig>
pd_packets_meter_spec_to_rates(p4_pd_packets_meter_spec_t *meter_spec) {
  double info_rate;
  uint32_t burst_size;
  BmMeterRateConfig rate;

  std::vector<BmMeterRateConfig> rates;

  info_rate = static_cast<double>(meter_spec->cir_pps) / 1000000.;
  burst_size = meter_spec->cburst_pkts;
  rate.units_per_micros = info_rate; rate.burst_size = burst_size;
  rates.push_back(rate);

  info_rate = static_cast<double>(meter_spec->pir_pps) / 1000000.;
  burst_size = meter_spec->pburst_pkts;
  rate.units_per_micros = info_rate; rate.burst_size = burst_size;
  rates.push_back(rate);

  return rates;
}
