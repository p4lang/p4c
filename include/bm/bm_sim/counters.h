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

//! @file counters.h
//! Includes both the Counter and CounterArray classes. CounterArray is exposed
//! in P4 v1.0.2 as the `counter` object. Action primitives can take a
//! CounterArray reference as a parameter (but not a Counter though). Here is a
//! possible implementation of the `count()` P4 primitive:
//! @code
//! class count : public ActionPrimitive<CounterArray &, const Data &> {
//!   void operator ()(CounterArray &counter_array, const Data &idx) {
//!     counter_array.get_counter(idx.get_uint()).increment_counter(
//!       get_packet());
//!   }
//! };
//! @endcode

#ifndef BM_BM_SIM_COUNTERS_H_
#define BM_BM_SIM_COUNTERS_H_

#include <vector>
#include <atomic>
#include <string>

#include "named_p4object.h"
#include "packet.h"

namespace bm {

//! Very basic counter implementation. Every Counter instance counts both bytes
//! and packets. The data plane is in charge of incrementing the counters
//! (e.g. through an action primitive), the control plane can query or write
//! a given value to the counters.
class Counter {
 public:
  //! A counter value (measuring bytes or packets) is a `uint64_t`.
  typedef uint64_t counter_value_t;

  enum CounterErrorCode {
    SUCCESS = 0,
    INVALID_COUNTER_NAME,
    INVALID_INDEX,
    ERROR
  };

  //! Increments both counter values (bytes and packets)
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

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in);

 private:
  std::atomic<std::uint_fast64_t> bytes{0u};
  std::atomic<std::uint_fast64_t> packets{0u};
};

typedef p4object_id_t meter_array_id_t;

//! CounterArray corresponds to the `counter` standard P4 v1.02 object. A
//! CounterArray reference can be used as a P4 primitive parameter. For example:
//! @code
//! class count : public ActionPrimitive<CounterArray &, const Data &> {
//!   void operator ()(CounterArray &counter_array, const Data &idx) {
//!     counter_array.get_counter(idx.get_uint()).increment_counter(
//!       get_packet());
//!   }
//! };
//! @endcode
class CounterArray : public NamedP4Object {
 public:
  typedef Counter::CounterErrorCode CounterErrorCode;

  typedef std::vector<Counter>::iterator iterator;
  typedef std::vector<Counter>::const_iterator const_iterator;

 public:
  CounterArray(const std::string &name, p4object_id_t id, size_t size)
    : NamedP4Object(name, id), counters(size) { }

  CounterErrorCode reset_counters();

  //! Access the counter at position \p idx, asserts if bad \p idx
  Counter &get_counter(size_t idx) {
    return counters[idx];
  }

  //! @copydoc get_counter
  const Counter &get_counter(size_t idx) const {
    return counters[idx];
  }

  //! Access the counter at position \p idx, asserts if bad \p idx
  Counter &operator[](size_t idx) {
    assert(idx < size());
    return counters[idx];
  }

  //! @copydoc operator[]
  const Counter &operator[](size_t idx) const {
    assert(idx < size());
    return counters[idx];
  }

  // iterators

  //! NC
  iterator begin() { return counters.begin(); }

  //! NC
  const_iterator begin() const { return counters.begin(); }

  //! NC
  iterator end() { return counters.end(); }

  //! NC
  const_iterator end() const { return counters.end(); }

  //! Return the size of the CounterArray (i.e. number of counters it includes)
  size_t size() const { return counters.size(); }

  void reset_state() { reset_counters(); }

 private:
    std::vector<Counter> counters;
};

}  // namespace bm

#endif  // BM_BM_SIM_COUNTERS_H_
