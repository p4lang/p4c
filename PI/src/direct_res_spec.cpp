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

#include <bm/bm_sim/meters.h>

#include <vector>

#include "direct_res_spec.h"

namespace pibmv2 {

void convert_from_counter_data(const pi_counter_data_t *from,
                               uint64_t *bytes, uint64_t *packets) {
  if (from->valid & PI_COUNTER_UNIT_BYTES)
    *bytes = static_cast<int64_t>(from->bytes);
  else
    *bytes = 0;
  if (from->valid & PI_COUNTER_UNIT_PACKETS)
    *packets = static_cast<int64_t>(from->packets);
  else
    *packets = 0;
}

void convert_to_counter_data(pi_counter_data_t *to,
                             uint64_t bytes, uint64_t packets) {
  // with bmv2, both are always valid
  to->valid = PI_COUNTER_UNIT_PACKETS | PI_COUNTER_UNIT_BYTES;
  to->bytes = static_cast<pi_counter_value_t>(bytes);
  to->packets = static_cast<pi_counter_value_t>(packets);
}

std::vector<bm::Meter::rate_config_t> convert_from_meter_spec(
    const pi_meter_spec_t *meter_spec) {
  std::vector<bm::Meter::rate_config_t> rates;
  auto conv_packets = [](uint64_t r, uint32_t b) {
    bm::Meter::rate_config_t new_rate;
    new_rate.info_rate = static_cast<double>(r) / 1000000.;
    new_rate.burst_size = b;
    return new_rate;
  };
  auto conv_bytes = [](uint64_t r, uint32_t b) {
    bm::Meter::rate_config_t new_rate;
    new_rate.info_rate = static_cast<double>(r) / 1000000.;
    new_rate.burst_size = b;
    return new_rate;
  };
  // guaranteed by PI common code
  assert(meter_spec->meter_unit != PI_METER_UNIT_DEFAULT);
  // choose appropriate conversion routine
  auto conv = (meter_spec->meter_unit == PI_METER_UNIT_PACKETS) ?
      conv_packets : conv_bytes;
  // perform conversion
  rates.push_back(conv(meter_spec->cir, meter_spec->cburst));
  rates.push_back(conv(meter_spec->pir, meter_spec->pburst));
  return rates;
}

void
convert_to_meter_spec(const pi_p4info_t *p4info, pi_p4_id_t m_id,
                      pi_meter_spec_t *meter_spec,
                      const std::vector<bm::Meter::rate_config_t> &rates) {
  auto conv_packets = [](const bm::Meter::rate_config_t &rate,
                         uint64_t *r, uint32_t *b) {
    *r = static_cast<uint64_t>(rate.info_rate * 1000000.);
    *b = rate.burst_size;
  };
  auto conv_bytes = [](const bm::Meter::rate_config_t &rate,
                       uint64_t *r, uint32_t *b) {
    *r = static_cast<uint64_t>(rate.info_rate * 1000000.);
    *b = rate.burst_size;
  };
  meter_spec->meter_unit = static_cast<pi_meter_unit_t>(
      pi_p4info_meter_get_unit(p4info, m_id));
  meter_spec->meter_type = static_cast<pi_meter_type_t>(
      pi_p4info_meter_get_type(p4info, m_id));
  assert(meter_spec->meter_unit != PI_METER_UNIT_DEFAULT);
  // choose appropriate conversion routine
  auto conv = (meter_spec->meter_unit == PI_METER_UNIT_PACKETS) ?
      conv_packets : conv_bytes;
  if (rates.empty()) {  // default meter config
    meter_spec->cir = static_cast<uint64_t>(-1);
    meter_spec->cburst = static_cast<uint32_t>(-1);
    meter_spec->pir = static_cast<uint64_t>(-1);
    meter_spec->pburst = static_cast<uint32_t>(-1);
  } else {
    conv(rates.at(0), &meter_spec->cir, &meter_spec->cburst);
    conv(rates.at(1), &meter_spec->pir, &meter_spec->pburst);
  }
}

}  // namespace pibmv2
