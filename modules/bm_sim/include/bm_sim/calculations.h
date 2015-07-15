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

#ifndef _BM_CALCULATIONS_H_
#define _BM_CALCULATIONS_H_

#include "phv.h"
#include "packet.h"

namespace hash {

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T xxh64(const char *buf, size_t len);

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T crc16(const char *buf, size_t len);
  
}

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
using CFn = std::function<T(const char *, size_t)>;

struct BufBuilder
{
  std::vector<std::pair<header_id_t, int> > fields{};
  size_t nbits_key{0};

  void push_back_field(header_id_t header, int field_offset, size_t nbits) {
    fields.push_back(std::pair<header_id_t, int>(header, field_offset));
    nbits_key += nbits;
  }

  void operator()(const PHV &phv, char *buf) const
  {
    int offset = 0;
    for(const auto &p : fields) {
      const Field &field = phv.get_field(p.first, p.second);
      // taken from headers.cpp::deparse
      offset += field.deparse(buf, offset);
      buf += offset / 8;
      offset = offset % 8;
    }
  }

  size_t get_nbytes_key() const { return (nbits_key + 7) / 8; }
};


/* I don't know if using templates here is actually a good idea or if I am just
   making my life more complicated for nothing */

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
class Calculation_ {
public:
  Calculation_(const BufBuilder &builder)
    : builder(builder), nbytes(builder.get_nbytes_key()) { }

  void set_compute_fn(const CFn<T> &fn) { compute_fn = fn; }

  T output(const PHV &phv) const {
    static thread_local ByteContainer key;
    key.resize(nbytes);
    key[nbytes - 1] = '\x00';
    builder(phv, key.data());
    return compute_fn(key.data(), nbytes);
  }

  T output(const Packet &pkt) const {
    return output(*pkt.get_phv());
  }

protected:
  ~Calculation_() { }

private:
  CFn<T> compute_fn{};
  BufBuilder builder;
  size_t nbytes;
};

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
class Calculation : public Calculation_<T> {
public:
  Calculation(const BufBuilder &builder)
    : Calculation_<T>(builder) { }
};

// quick and dirty and hopefully temporary
// added for support of old primitive modify_field_with_hash_based_offset()
class NamedCalculation : public NamedP4Object, public Calculation_<uint64_t>
{
public:
  NamedCalculation(const std::string &name, p4object_id_t id,
		   const BufBuilder &builder)
    : NamedP4Object(name, id), Calculation_<uint64_t>(builder) {
    set_compute_fn(hash::xxh64<uint64_t>);
  }

  uint64_t output(const PHV &phv) const {
    return Calculation_<uint64_t>::output(phv); 
  }

  uint64_t output(const Packet &pkt) const {
    return Calculation_<uint64_t>::output(pkt); 
  }
};

#endif
