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

#include "boost/variant.hpp"

#include "phv.h"
#include "packet.h"

namespace hash {

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T xxh64(const char *buf, size_t len);

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T crc16(const char *buf, size_t len);

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T cksum16(const char *buf, size_t len);
  
}

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
using CFn = std::function<T(const char *, size_t)>;

/* Old BufBuilder, not sufficient for TCP checksum */
// struct BufBuilder
// {
//   std::vector<std::pair<header_id_t, int> > fields{};
//   size_t nbits_key{0};

//   void push_back_field(header_id_t header, int field_offset, size_t nbits) {
//     fields.push_back(std::pair<header_id_t, int>(header, field_offset));
//     nbits_key += nbits;
//   }

//   void operator()(const PHV &phv, char *buf) const
//   {
//     int offset = 0;
//     for(const auto &p : fields) {
//       const Field &field = phv.get_field(p.first, p.second);
//       // taken from headers.cpp::deparse
//       offset += field.deparse(buf, offset);
//       buf += offset / 8;
//       offset = offset % 8;
//     }
//   }

//   size_t get_nbytes_key() const { return (nbits_key + 7) / 8; }
// };

struct BufBuilder
{
  struct field_t {
    header_id_t header;
    int field_offset;
  };

  struct constant_t {
    ByteContainer v;
    size_t nbits;
  };

  struct header_t {
    header_id_t header;
  };

  std::vector<boost::variant<field_t, constant_t, header_t> > entries{};
  bool with_payload{false};

  void push_back_field(header_id_t header, int field_offset) {
    field_t f = {header, field_offset};
    entries.emplace_back(f);
  }

  void push_back_constant(const ByteContainer &v, size_t nbits) {
    // TODO: general case
    assert(nbits % 8 == 0);
    constant_t c = {v, nbits};
    entries.emplace_back(c);
  }

  void push_back_header(header_id_t header) {
    header_t h = {header};
    entries.emplace_back(h);
  }

  void append_payload() {
    with_payload = true;
  }

  struct Deparse : public boost::static_visitor<> {
    Deparse(const PHV &phv, ByteContainer *buf)
      : phv(phv), buf(buf) { }

    char *extend(int more_bits) {
      int nbits_ = nbits + more_bits;
      buf->resize((nbits_ + 7) / 8);
      char *ptr = buf->data() + (nbits / 8);
      nbits = nbits_;
      // needed ?
      // if(new_bytes > 0) buf->back() = '\x00';
      return ptr;
    }

    int get_offset() const {
      return nbits % 8;
    }
    
    void operator()(const field_t &f) {
      const Header &header = phv.get_header(f.header);
      if(!header.is_valid()) return;
      const Field &field = header.get_field(f.field_offset);
      // taken from headers.cpp::deparse
      field.deparse(extend(field.get_nbits()), get_offset());
    }

    void operator()(const constant_t &c) {
      assert(get_offset() == 0);
      std::copy(c.v.begin(), c.v.end(), extend(c.nbits));
    }

    void operator()(const header_t &h) {
      assert(get_offset() == 0);
      const Header &header = phv.get_header(h.header);
      if(header.is_valid()) {
	header.deparse(extend(header.get_nbytes_packet() * 8));
      }
    }

    Deparse(const Deparse &other) = delete;
    Deparse &operator=(const Deparse &other) = delete;

    Deparse(Deparse &&other) = delete;
    Deparse &operator=(Deparse &&other) = delete;

    const PHV &phv;
    ByteContainer *buf;
    int nbits{0};
  };

  void operator()(const Packet &pkt, ByteContainer *buf) const
  {
    buf->clear();
    const PHV *phv = pkt.get_phv();
    Deparse visitor(*phv, buf);
    std::for_each(entries.begin(), entries.end(), boost::apply_visitor(visitor));
    if(with_payload) {
      size_t curr = buf->size();
      size_t psize = pkt.get_data_size();
      buf->resize(curr + psize);
      std::copy(pkt.data(), pkt.data() + psize, buf->begin() + curr);
    }
  }
};


/* I don't know if using templates here is actually a good idea or if I am just
   making my life more complicated for nothing */

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
class Calculation_ {
public:
  Calculation_(const BufBuilder &builder)
    : builder(builder) { }

  void set_compute_fn(const CFn<T> &fn) { compute_fn = fn; }

  T output(const Packet &pkt) const {
    static thread_local ByteContainer key;
    builder(pkt, &key);
    return compute_fn(key.data(), key.size());
  }

protected:
  ~Calculation_() { }

private:
  CFn<T> compute_fn{};
  BufBuilder builder;
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

  uint64_t output(const Packet &pkt) const {
    return Calculation_<uint64_t>::output(pkt); 
  }
};

#endif
