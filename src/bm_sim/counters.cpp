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

#include <bm/bm_sim/counters.h>

#include <iostream>

namespace bm {

Counter::CounterErrorCode
Counter::query_counter(counter_value_t *bytes, counter_value_t *packets) const {
  *bytes = this->bytes;
  *packets = this->packets;
  return SUCCESS;
}

Counter::CounterErrorCode
Counter::reset_counter() {
  bytes = 0u;
  packets = 0u;
  return SUCCESS;
}

Counter::CounterErrorCode
Counter::write_counter(counter_value_t bytes, counter_value_t packets) {
  this->bytes = bytes;
  this->packets = packets;
  return SUCCESS;
}

void
Counter::serialize(std::ostream *out) const {
  (*out) << bytes << " " << packets << "\n";
}

void
Counter::deserialize(std::istream *in) {
  uint64_t b, p;
  (*in) >> b >> p;
  bytes = b;
  packets = p;
}

Counter::CounterErrorCode
CounterArray::reset_counters() {
  for (Counter &c : counters)
    c.reset_counter();
  return Counter::SUCCESS;
}

}  // namespace bm
