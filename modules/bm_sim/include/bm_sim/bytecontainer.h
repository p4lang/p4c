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

//! @file bytecontainer.h

#ifndef BM_SIM_INCLUDE_BM_SIM_BYTECONTAINER_H_
#define BM_SIM_INCLUDE_BM_SIM_BYTECONTAINER_H_

#include <boost/functional/hash.hpp>

#include <vector>
#include <iterator>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace bm {

//! This class is used everytime a vector of bytes is needed in bmv2. It is most
//! notably used by the Field class (to store the byte representation of a
//! field) as well as to store match keys in tables.
class ByteContainer {
 public:
  typedef std::vector<char>::iterator iterator;
  typedef std::vector<char>::const_iterator const_iterator;
  // typedef std::reverse_iterator<iterator> reverse_iterator;
  // typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::vector<char>::reference reference;
  typedef std::vector<char>::const_reference const_reference;
  typedef size_t size_type;

 public:
  ByteContainer()
    : bytes() { }

  //! Constructs the container with \p nbytes copies of elements with value \p c
  explicit ByteContainer(const size_t nbytes, const char c = '\x00')
      : bytes(std::vector<char>(nbytes, c)) { }

  //! Constructs the container by copying the bytes in vector \p bytes
  explicit ByteContainer(const std::vector<char> &bytes)
    : bytes(bytes) { }

  //! Constructs the container by copying the bytes in this byte array
  ByteContainer(const char *bytes, size_t nbytes)
    : bytes(std::vector<char>(bytes, bytes + nbytes)) { }

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

  //! Constructs the container from a hexadecimal string. Parameter \p hexstring
  //! can optionally include the `0x` prefix.
  explicit ByteContainer(const std::string &hexstring)
    : bytes() {
    size_t idx = 0;

    assert(hexstring[idx] != '-');

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
  }

  //! Returns the number of bytes in the container
  size_type size() const noexcept { return bytes.size(); }

  //! Clears the contents of the container
  void clear() { return bytes.clear(); }

  // iterators

  //! NC
  iterator begin() { return bytes.begin(); }

  //! NC
  const_iterator begin() const { return bytes.begin(); }

  //! NC
  iterator end() { return bytes.end(); }

  //! NC
  const_iterator end() const { return bytes.end(); }

  // reverse_iterator rbegin() { return bytes.rbegin(); }

  // const_reverse_iterator rbegin() const { return bytes.rbegin(); }

  // reverse_iterator rend() { return bytes.rend(); }

  // const_reverse_iterator rend() const { return bytes.rend(); }

  // const_iterator cbegin() const { return bytes.cbegin(); }

  // const_iterator cend() const { return bytes.cend(); }

  // const_reverse_iterator crbegin() const { return bytes.crbegin(); }

  // const_reverse_iterator crend() const { return bytes.crend(); }

  //! Appends another ByteContainer to this container. \p other has to be
  //! different from `*this`.
  ByteContainer &append(const ByteContainer &other) {
    bytes.insert(end(), other.begin(), other.end());
    return *this;
  }

  //! Appends a byte array to this container
  ByteContainer &append(const char *byte_array, size_t nbytes) {
    bytes.insert(end(), byte_array, byte_array + nbytes);
    return *this;
  }

  //! Appends a binary string to this container
  ByteContainer &append(const std::string &other) {
    bytes.insert(end(), other.begin(), other.end());
    return *this;
  }

  //! Appends a character at the end of the container
  void push_back(char c) {
    bytes.push_back(c);
  }

  //! Access the character at position \p n. Will assert if n \p is greater or
  //! equal than the number of elements in the container.
  reference operator[](size_type n) {
    assert(n < size());
    return bytes[n];
  }

  //! @copydoc operator[]
  const_reference operator[](size_type n) const {
    assert(n < size());
    return bytes[n];
  }

  //! Access the last byte of the container. Undefined if the container is
  //! empty.
  reference back() {
    return bytes.back();
  }

  //! @copydoc back
  const_reference back() const {
    return bytes.back();
  }

  //! Access the first byte of the container. Undefined if the container is
  //! empty.
  reference front() {
    return bytes.front();
  }

  //! @copydoc front()
  const_reference front() const {
    return bytes.front();
  }

  //! Returns pointer to the underlying array serving as element storage. The
  //! pointer is such that range `[data(); data() + size())` is always a valid
  //! range, even if the container is empty.
  char* data() noexcept {
    return bytes.data();
  }

  //! @copydoc data()
  const char* data() const noexcept {
    return bytes.data();
  }

  //! Returns true is the contents of the containers are equal
  bool operator==(const ByteContainer& other) const {
    return bytes == other.bytes;
  }

  //! Returns true is the contents of the containers are not equal
  bool operator!=(const ByteContainer& other) const {
    return !(*this == other);
  }

  //! Increase the capacity of the container
  void reserve(size_t n) {
    bytes.reserve(n);
  }

  //! Resizes the container to contain \p bytes
  void resize(size_t n) {
    bytes.resize(n);
  }

  //! Perform a byte-by-byte masking of the container.
  //! Will assert if `size() != mask.size()`.
  void apply_mask(const ByteContainer &mask) {
    assert(size() == mask.size());
    for (size_t i = 0; i < size(); i++)
      bytes[i] &= mask[i];
  }

  //! Returns the hexadecimal representation of the byte container as a string
  std::string to_hex(bool upper_case = false) const {
    std::ostringstream ret;

    for (std::string::size_type i = 0; i < size(); i++) {
      ret << std::setw(2) << std::setfill('0') << std::hex
          << (upper_case ? std::uppercase : std::nouppercase)
          << static_cast<int>(bytes[i]);
    }

    return ret.str();
  }

 private:
  std::vector<char> bytes;
};

struct ByteContainerKeyHash {
  std::size_t operator()(const ByteContainer& b) const {
    // Murmur, boost::hash_range or Jenkins?
    return boost::hash_range(b.begin(), b.end());
  }
};

}  // namespace bm

#endif  // BM_SIM_INCLUDE_BM_SIM_BYTECONTAINER_H_
