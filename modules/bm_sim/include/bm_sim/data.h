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

#ifndef _BM_DATA_H_
#define _BM_DATA_H_

#include <iostream>
#include <cstring>
#include <cassert>
#include <type_traits>

#include "bignum.h"
#include "bytecontainer.h"

using bignum::Bignum;

class Data
{  
public:
  Data() {}

  template<typename T,
	   typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  Data(T i)
    : value(i) {}

  Data(const char *bytes, int nbytes) {
    bignum::import_bytes(value, bytes, nbytes);
  }
  
  virtual ~Data() { };

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
    set(hexstring);
  }

  virtual void export_bytes() {}

  // need to figure out what to do with signed values
  template<typename T,
	   typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  void set(T i) {
    value = i;
    export_bytes();
  }

  template<typename T,
	   typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
  void set(T i) {
    value = (int) i;
    export_bytes();
  }
  
  void set(const char *bytes, int nbytes) {
    bignum::import_bytes(value, bytes, nbytes);
    export_bytes();
  }

  void set(const Data &data) {
    value = data.value;
    export_bytes();
  }

  void set(Data &&data) {
    value = std::move(data.value);
    export_bytes();
  }

  void set(const ByteContainer &bc) {
    bignum::import_bytes(value, bc.data(), bc.size());
    export_bytes();
  }

  void set(const std::string &hexstring) {
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
    export_bytes(); // not very efficient for fields, we import then export...
  }

  template<typename T,
	   typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  T get() const {
    assert(arith);
    return static_cast<T>(value);
  }

  unsigned int get_uint() const {
    assert(arith);
    // Bad ?
    return (unsigned) value;
  }

  uint64_t get_uint64() const {
    assert(arith);
    // Bad ?
    return (uint64_t) value;
  }

  int get_int() const {
    assert(arith);
    // Bad ?
    return (int) value;
  }

  bool get_arith() const { return arith; }

  // TODO: overload operators for those ?

  void add(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value + src2.value;
    export_bytes();
  }

  void sub(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value - src2.value;
    export_bytes();
  }

  void mod(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value % src2.value;
    export_bytes();
  }

  void multiply(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value * src2.value;
    export_bytes();
  }

  void shift_left(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    assert(src2.value >= 0);
    value = src1.value << (src2.get_uint());
    export_bytes();
  }

  void shift_right(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    assert(src2.value >= 0);
    value = src1.value >> (src2.get_uint());
    export_bytes();
  }

  void shift_left(const Data &src1, unsigned int src2) {
    assert(src1.arith);
    value = src1.value << src2;
    export_bytes();
  }

  void shift_right(const Data &src1, unsigned int src2) {
    assert(src1.arith);
    value = src1.value >> src2;
    export_bytes();
  }

  void bit_and(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value & src2.value;
    export_bytes();
  }

  void bit_or(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value | src2.value;
    export_bytes();
  }

  void bit_xor(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value ^ src2.value;
    export_bytes();
  }

  void bit_neg(const Data &src) {
    assert(src.arith);
    value = ~src.value;
    export_bytes();
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

  /* Copy constructor
     I know it is considered a style exception by many to define this manually.
     However, I need to be able to check whether the bignum value needs to be
     copied */
  Data(const Data &other)
    : arith(other.arith) {
    if(other.arith) value = other.value;
  }

  /* Copy assignment operator */
  Data &operator=(const Data &other) {
    Data tmp(other); // re-use copy-constructor
    *this = std::move(tmp); // re-use move-assignment
    return *this;
  }

  Data(Data &&other) = default;

  Data &operator=(Data &&other) = default;

protected:
  Bignum value{0};
  bool arith{true};
};

#endif
