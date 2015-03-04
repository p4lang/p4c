#ifndef _BM_DATA_H_
#define _BM_DATA_H_

#include <iostream>
#include <cstring>
#include <cassert>

#include "bignum.h"

using bignum::Bignum;

class Data
{  
public:
  Data() {}

  Data(int i)
    : value(i) {}

  Data(unsigned i)
    : value(i) {}

  Data(const char *bytes, int nbytes) {
    bignum::import_bytes(value, bytes, nbytes);
  }

  static char char2digit(char c) {
    if(c >= '0' && c <= '9')
      return (c - '0');
    if(c >= 'A' && c <= 'F')
      return (c - 'A' + 10);
    if(c >= 'a' && c <= 'f')
      return (c - 'a' + 10);
    assert(0);
    return 0;
  }

  Data(const std::string &hexstring) {
    std::vector<char> bytes;
    size_t idx = 0;
    bool neg = false;

    if(hexstring[idx] == '-') {
      neg = true;
      ++idx;
    }
    if(hexstring[idx] == '0' && hexstring[idx + 1] == 'x') {
      idx += 2;
    }
    size_t size = hexstring.size();
    assert((size - idx) > 0);

    if((size - idx) % 2 != 0) {
      char c = char2digit(hexstring[idx++]);
      bytes.push_back(c);
    }

    for(; idx < size; ) {
      char c = char2digit(hexstring[idx++]) << 4;
      c += char2digit(hexstring[idx++]);
      bytes.push_back(c);
    }

    bignum::import_bytes(value, bytes.data(), bytes.size());
    if(neg) value = -value;
  }

  void set(unsigned int i) {
    value = i;
  }

  void set(const char *bytes, int nbytes) {
    bignum::import_bytes(value, bytes, nbytes);
  }

  void set(const Data &data) {
    value = data.value;
  }

  unsigned int get_uint() const {
    assert(arith);
    // Bad ?
    return (unsigned) value;
  }

  int get_int() const {
    assert(arith);
    // Bad ?
    return (int) value;
  }

  // TODO: overload operators for those ?

  void add(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value + src2.value;
  }

  void bit_and(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value & src2.value;
  }

  void bit_or(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value | src2.value;
  }

  void bit_xor(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value ^ src2.value;
  }

  void bit_neg(const Data &src) {
    assert(src.arith);
    value = ~src.value;
  }

  friend bool operator==(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value == rhs.value;
  }

  friend bool operator!=(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return !(lhs == rhs);
  }

  friend bool operator>(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value > rhs.value;
  }

  friend bool operator>=(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value >= rhs.value;
  }

  friend bool operator<(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value < rhs.value;
  }

  friend bool operator<=(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value <= rhs.value;
  }

  friend std::ostream& operator<<( std::ostream &out, const Data &d ) {
    assert(d.arith);
    out << d.value;
    return out;
  }

protected:
  Bignum value;
  bool arith{true};
};

#endif
