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

//! @file meters.h
//! Includes both the Meter and MeterArray classes. MeterArray is exposed
//! in P4 v1.0.2 as the `meter` object. Action primitives can take a
//! MeterArray reference as a parameter (but not a Meter though). Here is a
//! possible implementation of the `execute_meter()` P4 primitive:
//! @code
//! class execute_meter
//!   : public ActionPrimitive<MeterArray &, const Data &, Field &> {
//!   void operator ()(MeterArray &meter_array, const Data &idx, Field &dst) {
//!     dst.set(meter_array.execute_meter(get_packet(), idx.get_uint()));
//!   }
//! };
//! @endcode

#ifndef BM_BM_SIM_METERS_H_
#define BM_BM_SIM_METERS_H_

#include <vector>
#include <mutex>
#include <memory>
#include <string>

#include "named_p4object.h"
#include "packet.h"
#include "logger.h"

namespace bm {

// I initially implemented this with template values: meter type and rate
// count. I thought it would potentially speed up operations. However, it meant
// I also had to use a virtual interface (e.g. to store in p4 objects / use in
// action primitives). After some separate benchmarking, I decided against using
// templates (not really worth it). The only time I observed a real speed up was
// without using virtual functions (which is not really possible here anyway)
// and for a certain rate count (for which the compiler was doing loop
// unrolling, probably was a more cache friendly value too). I think I can live
// with the extra overhead of having a vector (vs an array) and having to test
// for the meter type.  Maybe I will change this later, but meters are not used
// that much so for now I am going for simplicity.

//! Implementation of a general non-color aware `N` rate `N+1` color marker. For
//! P4 we will use 2 rate 3 color meters, as per
//! <a href="https://tools.ietf.org/html/rfc2698">this rfc</a>.
//!
//! Note that a Meter operates on either bytes or packets (not both, unlike a
//! Counter).
class Meter {
 public:
  typedef unsigned int color_t;
  typedef size_t rate_idx_t;
  struct rate_config_t {
    double info_rate;
    size_t burst_size;

    static rate_config_t make(double info_rate, size_t burst_size) {
      return {info_rate, burst_size};
    }
  };
  typedef std::chrono::steady_clock clock;

 public:
  enum class MeterType {
    BYTES,
    PACKETS
  };

  enum MeterErrorCode {
    SUCCESS = 0,
    INVALID_METER_NAME,
    INVALID_INDEX,
    BAD_RATES_LIST,
    INVALID_INFO_RATE_VALUE,
    INVALID_BURST_SIZE_VALUE,
    ERROR
  };

 public:
  Meter(MeterType type, size_t rate_count)
    : type(type), rates(rate_count) { }

  // the rate configs must be sorted from smaller rate to higher rate
  // in the 2 rate meter case: {CIR, PIR}

  // this is probably a total overkill, but I am entitled to my fun. set_rates
  // accepts a vector, an initializer list, or any Random Accces Iterator pair.
  template<typename RAIt>
  MeterErrorCode set_rates(const RAIt first, const RAIt last) {
    // I think using static asserts is cleaner than using SFINAE / enable_if
    static_assert(
        std::is_same<std::random_access_iterator_tag,
        typename std::iterator_traits<RAIt>::iterator_category>::value,
        "wrong iterator category");
    static_assert(
        std::is_same<rate_config_t,
        typename std::iterator_traits<RAIt>::value_type>::value,
        "wrong iterator value type");
    typename std::iterator_traits<RAIt>::difference_type n =
      std::distance(first, last);
    assert(n >= 0);
    auto lock = unique_lock();
    if (static_cast<size_t>(n) != rates.size()) return BAD_RATES_LIST;
    size_t idx = 0;
    for (auto it = first; it < last; ++it) {
      MeterErrorCode rc = set_rate(idx++, *it);
      if (rc != SUCCESS) return rc;
    }
    std::reverse(rates.begin(), rates.end());
    configured = true;
    return SUCCESS;
  }

  MeterErrorCode set_rates(const std::vector<rate_config_t> &configs) {
    return set_rates(configs.begin(), configs.end());
  }

  MeterErrorCode set_rates(
      const std::initializer_list<rate_config_t> &configs) {
    return set_rates(configs.begin(), configs.end());
  }

  // returns an empty vector if meter not configured
  std::vector<rate_config_t> get_rates() const;

  MeterErrorCode reset_rates();

  //! Executes the meter on the given packet. Returns an integral value in the
  //! range [0, N] (where N is the number of rates for this meter). A higher
  //! value means that the meter is "busier". For a classic trTCM:
  //!   - `0 <-> GREEN`
  //!   - `1 <-> YELLOW`
  //!   - `2 <-> RED`
  color_t execute(const Packet &pkt);

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in);

 public:
  /* This is for testing purposes only, for more accurate tests */
  static void reset_global_clock();

 private:
  typedef std::unique_lock<std::mutex> UniqueLock;
  UniqueLock unique_lock() const { return UniqueLock(*m_mutex); }
  void unlock(UniqueLock &lock) const { lock.unlock(); }  // NOLINT

 private:
  struct MeterRate {
    bool valid{};  // TODO(antonin): get rid of this?
    double info_rate{};  // in bytes / packets per microsecond
    size_t burst_size{};
    size_t tokens{};
    uint64_t tokens_last{};
    color_t color{};
  };

 private:
  MeterErrorCode set_rate(size_t idx, const rate_config_t &config);

 private:
  MeterType type;
  // I decided to take the easy route and wrap the m_mutex into a
  // unique_ptr. Mutexes are not movable and that would be a problem with the
  // MeterArray implementation. I don't think this will incur a performance hit
  std::unique_ptr<std::mutex> m_mutex{new std::mutex()};
  // mutable std::mutex m_mutex;
  std::vector<MeterRate> rates;
  bool configured{false};
};

typedef p4object_id_t meter_array_id_t;

//! MeterArray corresponds to the `meter` standard P4 v1.02 object. A
//! MeterArray reference can be used as a P4 primitive parameter. For example:
//! @code
//! class execute_meter
//!   : public ActionPrimitive<MeterArray &, const Data &, Field &> {
//!   void operator ()(MeterArray &meter_array, const Data &idx, Field &dst) {
//!     dst.set(meter_array.execute_meter(get_packet(), idx.get_uint()));
//!   }
//! };
//! @endcode
class MeterArray : public NamedP4Object {
 public:
  typedef Meter::MeterErrorCode MeterErrorCode;
  typedef Meter::color_t color_t;
  typedef Meter::MeterType MeterType;
  typedef Meter::rate_config_t rate_config_t;

  typedef std::vector<Meter>::iterator iterator;
  typedef std::vector<Meter>::const_iterator const_iterator;

 public:
  MeterArray(const std::string &name, p4object_id_t id,
             MeterType type, size_t rate_count, size_t size)
    : NamedP4Object(name, id) {
    meters.reserve(size);
    for (size_t i = 0; i < size; i++)
      meters.emplace_back(type, rate_count);
  }

  //! Executes the meter at index \p idx on the given packet and returns the
  //! correct integral color value.
  //! See Meter::execute() for more information.
  color_t execute_meter(const Packet &pkt, size_t idx) {
    BMLOG_DEBUG_PKT(pkt, "Executing meter {}[{}]", get_name(), idx);
    return meters[idx].execute(pkt);
  }

  template<class RAIt>
  MeterErrorCode set_rates(const RAIt first, const RAIt last) {
    // check validity of rates here?
    MeterErrorCode rc;
    for (Meter &m : meters) {
      rc = m.set_rates(first, last);
      if (rc != MeterErrorCode::SUCCESS) return rc;
    }
    return MeterErrorCode::SUCCESS;
  }

  MeterErrorCode set_rates(const std::vector<rate_config_t> &configs) {
    return set_rates(configs.begin(), configs.end());
  }

  MeterErrorCode set_rates(
      const std::initializer_list<rate_config_t> &configs) {
    return set_rates(configs.begin(), configs.end());
  }

  //! Access the meter at position \p idx, asserts if bad \p idx
  Meter &get_meter(size_t idx) {
    return meters[idx];
  }

  //! @copydoc get_meter
  const Meter &get_meter(size_t idx) const {
    return meters[idx];
  }

  //! Access the meter at position \p idx, with bound-checking. If pos not
  //! within the range of the array, an exception of type std::out_of_range is
  //! thrown.
  Meter &at(size_t idx) {
    return meters.at(idx);
  }

  //! @copydoc at
  const Meter &at(size_t idx) const {
    return meters.at(idx);
  }

  //! Access the meter at position \p idx, asserts if bad \p idx
  Meter &operator[](size_t idx) {
    assert(idx < size());
    return meters[idx];
  }

  //! @copydoc operator[]
  const Meter &operator[](size_t idx) const {
    assert(idx < size());
    return meters[idx];
  }

  // iterators

  //! NC
  iterator begin() { return meters.begin(); }

  //! NC
  const_iterator begin() const { return meters.begin(); }

  //! NC
  iterator end() { return meters.end(); }

  //! NC
  const_iterator end() const { return meters.end(); }

  //! Returns the size of the MeterArray (i.e. number of meters it includes)
  size_t size() const { return meters.size(); }

  void reset_state();

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in);

 private:
  std::vector<Meter> meters{};
};

}  // namespace bm

#endif  // BM_BM_SIM_METERS_H_
