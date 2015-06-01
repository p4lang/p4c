#ifndef _BM_CALCULATIONS_H_
#define _BM_CALCULATIONS_H_

#include "phv.h"
#include "packet.h"

namespace hash {

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T xxh64(const char *buf, size_t len);
  
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
class Calculation {
public:
  Calculation(const BufBuilder &builder)
    : builder(builder), nbytes(builder.get_nbytes_key()) { }

  void set_compute_fn(const CFn<T> &fn) { compute_fn = fn; }

  T output(const Packet &pkt) const {
    static thread_local ByteContainer key;
    key.resize(nbytes);
    key[nbytes - 1] = '\x00';
    builder(*pkt.get_phv(), key.data());
    return compute_fn(key.data(), nbytes);
  }

private:
  CFn<T> compute_fn{};
  BufBuilder builder;
  size_t nbytes;
};

#endif
