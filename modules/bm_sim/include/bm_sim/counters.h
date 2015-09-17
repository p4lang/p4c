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

#ifndef _BM_COUNTERS_H_
#define _BM_COUNTERS_H_

#include <vector>
#include <atomic>

#include "named_p4object.h"
#include "packet.h"

/* This is a very basic counter for now: data plane can only increment the
   counters, control plane only can query values and reset */
class Counter
{
public:
  typedef uint64_t counter_value_t;

  enum CounterErrorCode {
    SUCCESS = 0,
    INVALID_INDEX,
    ERROR
  };

  void increment_counter(const Packet &pkt) {
    bytes += pkt.get_ingress_length();
    packets += 1;
  }
  
  CounterErrorCode query_counter(counter_value_t *bytes,
				 counter_value_t *packets) const;

  CounterErrorCode reset_counter();

  // in case something more general than reset is needed
  CounterErrorCode write_counter(counter_value_t bytes,
				 counter_value_t packets);

private:
  std::atomic<std::uint_fast64_t> bytes{0u};
  std::atomic<std::uint_fast64_t> packets{0u};
};

typedef p4object_id_t meter_array_id_t;

class CounterArray : public NamedP4Object
{
public:
  typedef Counter::CounterErrorCode CounterErrorCode;

  typedef vector<Counter>::iterator iterator;
  typedef vector<Counter>::const_iterator const_iterator;

public:
  CounterArray(const std::string &name, p4object_id_t id, size_t size)
    : NamedP4Object(name, id), counters(size) { }

  CounterErrorCode reset_counters();

  Counter &get_counter(size_t idx) {
    return counters[idx];
  }

  const Counter &get_counter(size_t idx) const {
    return counters[idx];
  }

  Counter &operator[](size_t idx) {
    assert(idx < size());
    return counters[idx];
  }

  const Counter &operator[](size_t idx) const {
    assert(idx < size());
    return counters[idx];
  }

  // iterators
  iterator begin() { return counters.begin(); }

  const_iterator begin() const { return counters.begin(); }

  iterator end() { return counters.end(); }

  const_iterator end() const { return counters.end(); }

  size_t size() const { return counters.size(); }

private:
    std::vector<Counter> counters;
};

#endif
