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

//! @file deparser.h

#ifndef BM_BM_SIM_DATA_H_
#define BM_BM_SIM_DATA_H_

#include <type_traits>
#include <string>
#include <vector>

#include <cstring>
#include <cassert>

#include "bignum.h"
#include "bytecontainer.h"

namespace bm {

using bignum::Bignum;

//! Data is a very important class in bmv2. It can be used to represent an
//! arbitrarily large number. It is also the base class for the Field and
//! Register classes. Arithmetic expressions are evaluated using Data as
//! temporaries and the result is stored in a Data instance.
//!
//! All base operations are supported, although not (yet?) by operator
//! overloading.
//! For example:
//! @code
//! Data d1(6574);
//! Data d2("0xabcd");
//! d1.add(d1, d2);  // d1 = d1 + d2
//! @endcode
//!
//! Note that Data includes a Bignum (for arbitrary arithmetic). Therefore,
//! operations involving Data can be rather costly. In particular, constructing
//! a Data instance involves a heap memory allocation.
class Data {
 public:
  Data() {}

  //! Constructs a Data instance from any integral type
  template<typename T,
           typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  explicit Data(T i)
    : value(i) {}

  //! Constructs a Data instance from a byte array. There is no sign support.
  Data(const char *bytes, int nbytes) {
    bignum::import_bytes(&value, bytes, nbytes);
  }

  virtual ~Data() { }

  static char char2digit(char c) {
    if (c >= '0' && c <= '9')
      return (c - '0');
    if (c >= 'A' && c <= 'F')
      return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f')
      return (c - 'a' + 10);
    assert(0);
    return 0;
  }

  //! Constructs a Data instance from a hexadecimal string. The string can start
  //! with a "-", for negative values, and can optionally include the `0x`
  //! prefix. `Data("0xab")` is equivalent to `Data("ab")`. For negative values,
  //! the sign needs to come before the prefix (if present).
  explicit Data(const std::string &hexstring) {
    set(hexstring);
  }

  virtual void export_bytes() {}

  // TODO(Antonin): need to figure out what to do with signed values
  //! Set the value of Data from any integral type
  template<typename T,
           typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  void set(T i) {
    value = i;
    export_bytes();
  }

  //! Set the value of Data from an enum member
  template<typename T,
           typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
  void set(T i) {
    value = static_cast<int>(i);
    export_bytes();
  }

  //! Set the value of Data from a byte array
  void set(const char *bytes, int nbytes) {
    bignum::import_bytes(&value, bytes, nbytes);
    export_bytes();
  }

  //! Set the value of Data from another data instance
  void set(const Data &data) {
    value = data.value;
    export_bytes();
  }

  void set(Data &&data) {
    value = std::move(data.value);
    export_bytes();
  }

  void set(const ByteContainer &bc) {
    bignum::import_bytes(&value, bc.data(), bc.size());
    export_bytes();
  }

  //! Set the value of Data from a hexadecimal string. See
  //! Data(const std::string &hexstring) for more information on \p hexstring
  void
#if __GNUC__ == 5 && __GNUC_MINOR__ <= 2
  // With g++-5.2, and when compiling with -O2, I sometimes get a segfault in
  // this function (after calling mpz_import). I do not see anything wrong with
  // this code and g++-4.8/9, as well as g++5.3, do not have this issue. I was
  // not able to reproduce this bug with a simpler code sample (one without the
  // bignum) and the assembly is too difficult for me to look at, so I am just
  // going to blame it on the compiler and move on...
  __attribute__((optimize("O0")))
#endif
  set(const std::string &hexstring) {
    std::vector<char> bytes;
    size_t idx = 0;
    bool neg = false;

    if (hexstring[idx] == '-') {
      neg = true;
      ++idx;
    }
    if (hexstring[idx] == '0' && hexstring[idx + 1] == 'x') {
      idx += 2;
    }
    size_t size = hexstring.size();
    assert((size - idx) > 0);

    if ((size - idx) % 2 != 0) {
      char c = char2digit(hexstring[idx++]);
      bytes.push_back(c);
    }

    for (; idx < size; ) {
      char c = char2digit(hexstring[idx++]) << 4;
      c += char2digit(hexstring[idx++]);
      bytes.push_back(c);
    }

    bignum::import_bytes(&value, bytes.data(), bytes.size());
    if (neg) value = -value;
    export_bytes();  // not very efficient for fields, we import then export...
  }

  //! Convert the value of Data to any inegral type
  template<typename T,
           typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  T get() const {
    assert(arith);
    return static_cast<T>(value);
  }

  //! Get the value of Data has an unsigned integer
  unsigned int get_uint() const {
    assert(arith);
    // Bad ?
    return static_cast<unsigned int>(value);
  }

  //! Get the value of Data has a `uint64_t`
  uint64_t get_uint64() const {
    assert(arith);
    // Bad ?
    return static_cast<uint64_t>(value);
  }

  //! get the value of Data has an integer
  int get_int() const {
    assert(arith);
    // Bad ?
    return static_cast<int>(value);
  }

  //! get the binary representation of Data has a string. There is no sign
  //! support.
  std::string get_string() const {
    assert(arith);
    const size_t export_size = bignum::export_size_in_bytes(value);
    std::string s(export_size, '\x00');
    // this is not technically correct, but works for all compilers
    bignum::export_bytes(&s[0], export_size, value);
    return s;
  }

  bool get_arith() const { return arith; }

  // TODO(antonin): overload operators for those ?

  //! NC
  void add(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value + src2.value;
    export_bytes();
  }

  //! NC
  void sub(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value - src2.value;
    export_bytes();
  }

  //! Performs a modulo operation. The following needs to be true: \p src1 >= 0
  //! and \p src2 > 0.
  void mod(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    assert(src1.value >= 0 && src2.value > 0);
    value = src1.value % src2.value;
    export_bytes();
  }

  //! Performs a division operation. The following needs to be true: \p src1 >=
  //! 0 and \p src2 > 0.
  void divide(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    assert(src1.value >= 0 && src2.value > 0);
    value = src1.value / src2.value;
    export_bytes();
  }

  //! NC
  void multiply(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value * src2.value;
    export_bytes();
  }

  //! NC
  void shift_left(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    assert(src2.value >= 0);
    value = src1.value << (src2.get_uint());
    export_bytes();
  }

  //! NC
  void shift_right(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    assert(src2.value >= 0);
    value = src1.value >> (src2.get_uint());
    export_bytes();
  }

  //! NC
  void shift_left(const Data &src1, unsigned int src2) {
    assert(src1.arith);
    value = src1.value << src2;
    export_bytes();
  }

  //! NC
  void shift_right(const Data &src1, unsigned int src2) {
    assert(src1.arith);
    value = src1.value >> src2;
    export_bytes();
  }

  //! NC
  void bit_and(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value & src2.value;
    export_bytes();
  }

  //! NC
  void bit_or(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value | src2.value;
    export_bytes();
  }

  //! NC
  void bit_xor(const Data &src1, const Data &src2) {
    assert(src1.arith && src2.arith);
    value = src1.value ^ src2.value;
    export_bytes();
  }

  //! NC
  void bit_neg(const Data &src) {
    assert(src.arith);
    value = ~src.value;
    export_bytes();
  }

  //! NC
  void two_comp_mod(const Data &src, const Data &width) {
    static Bignum one(1);
    unsigned int uwidth = width.get_uint();
    Bignum mask = (one << uwidth) - 1;
    Bignum max = (one << (uwidth - 1)) - 1;
    Bignum min = -(one << (uwidth - 1));
    if (src.value < min || src.value > max) {
      value = src.value & mask;
      if (value > max)
        value -= (one << uwidth);
    } else {
      value = src.value;
    }
  }

  //! NC
  template<typename T,
           typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  bool test_eq(T i) const {
    return (value == i);
  }

  //! NC
  friend bool operator==(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value == rhs.value;
  }

  //! NC
  friend bool operator!=(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return !(lhs == rhs);
  }

  //! NC
  friend bool operator>(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value > rhs.value;
  }

  //! NC
  friend bool operator>=(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value >= rhs.value;
  }

  //! NC
  friend bool operator<(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value < rhs.value;
  }

  //! NC
  friend bool operator<=(const Data &lhs, const Data &rhs) {
    assert(lhs.arith && rhs.arith);
    return lhs.value <= rhs.value;
  }

  //! NC
  friend std::ostream& operator<<(std::ostream &out, const Data &d) {
    assert(d.arith);
    out << d.value;
    return out;
  }

  // Copy constructor
  //! NC
  Data(const Data &other)
    : arith(other.arith) {
    if (other.arith) value = other.value;
  }

  // Copy assignment operator
  //! NC
  Data &operator=(const Data &other) {
    Data tmp(other);  // re-use copy-constructor
    *this = std::move(tmp);  // re-use move-assignment
    return *this;
  }

  //! NC
  Data(Data &&other) = default;

  //! NC
  Data &operator=(Data &&other) = default;

 protected:
  Bignum value{0};
  bool arith{true};
};

}  // namespace bm

#endif  // BM_BM_SIM_DATA_H_
