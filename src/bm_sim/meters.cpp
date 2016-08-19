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

#include <algorithm>
#include <vector>

namespace bm {

typedef Meter::MeterErrorCode MeterErrorCode;

using ticks = std::chrono::microseconds;  // better with nanoseconds ?
using std::chrono::duration_cast;

namespace {

Meter::clock::time_point time_init = Meter::clock::now();

}  // namespace

MeterErrorCode
Meter::set_rate(size_t idx, const rate_config_t &config) {
  MeterRate &rate = rates[idx];
  rate.valid = true;
  rate.info_rate = config.info_rate;
  rate.burst_size = config.burst_size;
  rate.tokens = config.burst_size;
  rate.tokens_last = 0u;
  rate.color = (idx + 1);
  if (idx > 0) {
    MeterRate &prev_rate = rates[idx - 1];
    if (prev_rate.info_rate > rate.info_rate) return INVALID_INFO_RATE_VALUE;
  }
  return SUCCESS;
}

MeterErrorCode
Meter::reset_rates() {
  auto lock = unique_lock();
  for (MeterRate &rate : rates) {
    rate.valid = false;
  }
  configured = false;
  return SUCCESS;
}

Meter::color_t
Meter::execute(const Packet &pkt) {
  color_t packet_color = 0;

  if (!configured) return packet_color;

  clock::time_point now = clock::now();
  int64_t micros_since_init = duration_cast<ticks>(now - time_init).count();

  auto lock = unique_lock();

  /* I tried to make this as accurate as I could. Everything is computed
     compared to a single time point (init). I do not use the interval since
     last update, because it would require multiple consecutive
     approximations. Maybe this is an overkill or I am underestimating the code
     I wrote for BMv1.
     The only thing that could go wrong is if tokens_since_init grew too large,
     but I think it would take years even at high throughput */
  for (MeterRate &rate : rates) {
    uint64_t tokens_since_init =
      static_cast<uint64_t>(micros_since_init * rate.info_rate);
    assert(tokens_since_init >= rate.tokens_last);
    size_t new_tokens = tokens_since_init - rate.tokens_last;
    rate.tokens_last = tokens_since_init;
    rate.tokens = std::min(rate.tokens + new_tokens, rate.burst_size);

    size_t input = (type == MeterType::PACKETS) ? 1u : pkt.get_ingress_length();

    if (rate.tokens < input) {
      packet_color = rate.color;
      break;
    } else {
      rate.tokens -= input;
    }
  }

  return packet_color;
}

void
Meter::serialize(std::ostream *out) const {
  auto lock = unique_lock();
  (*out) << configured << "\n";
  if (configured) {
    for (const auto &rate : rates)
      (*out) << rate.info_rate << " " << rate.burst_size << "\n";
  }
}

void
Meter::deserialize(std::istream *in) {
  auto lock = unique_lock();
  (*in) >> configured;
  if (configured) {
    for (size_t i = 0; i < rates.size(); i++) {
      rate_config_t config;
      (*in) >> config.info_rate;
      (*in) >> config.burst_size;
      set_rate(i, config);
    }
  }
}

void
Meter::reset_global_clock() {
  time_init = Meter::clock::now();
}

std::vector<Meter::rate_config_t>
Meter::get_rates() const {
  std::vector<rate_config_t> configs;
  auto lock = unique_lock();
  if (!configured) return configs;
  // elegant but probably not the most efficient
  for (const MeterRate &rate : rates)
    configs.push_back(rate_config_t::make(rate.info_rate, rate.burst_size));
  std::reverse(configs.begin(), configs.end());
  return configs;
}


void
MeterArray::reset_state() {
  for (auto &m : meters) m.reset_rates();
}

void
MeterArray::serialize(std::ostream *out) const {
  for (const auto &m : meters) m.serialize(out);
}

void
MeterArray::deserialize(std::istream *in) {
  for (auto &m : meters) m.deserialize(in);
}

}  // namespace bm
