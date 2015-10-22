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

#ifndef _BM_BYTECONTAINER_H_
#define _BM_BYTECONTAINER_H_

#include <vector>
#include <iterator>
#include <string>
#include <iostream>
#include <sstream> 
#include <iomanip>

#include <boost/functional/hash.hpp>

using std::vector;

class ByteContainer
{
public:
  typedef vector<char>::iterator iterator;
  typedef vector<char>::const_iterator const_iterator;
  // typedef std::reverse_iterator<iterator> reverse_iterator;
  // typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef vector<char>::reference reference;
  typedef vector<char>::const_reference const_reference;
  typedef size_t size_type;

public:
  ByteContainer()
    : bytes() {}

  ByteContainer(int nbytes)
    : bytes(vector<char>(nbytes)) {}

  ByteContainer(const vector<char> &bytes)
    : bytes(bytes) {}

  ByteContainer(const char *bytes, size_t nbytes)
    : bytes(vector<char>(bytes, bytes + nbytes)) {}

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

  ByteContainer(const std::string &hexstring)
    : bytes() {
    size_t idx = 0;

    assert(hexstring[idx] != '-');

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
  }

  size_type size() const noexcept { return bytes.size(); }

  void clear() { return bytes.clear(); }

  // iterators
  iterator begin() { return bytes.begin(); }

  const_iterator begin() const { return bytes.begin(); }

  iterator end() { return bytes.end(); }

  const_iterator end() const { return bytes.end(); }

  // reverse_iterator rbegin() { return bytes.rbegin(); }

  // const_reverse_iterator rbegin() const { return bytes.rbegin(); }

  // reverse_iterator rend() { return bytes.rend(); }

  // const_reverse_iterator rend() const { return bytes.rend(); }

  // const_iterator cbegin() const { return bytes.cbegin(); }

  // const_iterator cend() const { return bytes.cend(); }

  // const_reverse_iterator crbegin() const { return bytes.crbegin(); }

  // const_reverse_iterator crend() const { return bytes.crend(); }

  ByteContainer &append(const ByteContainer &other) {
    bytes.insert(end(), other.begin(), other.end());
    return *this;
  }

  ByteContainer &append(const char *byte_array, size_t nbytes) {
    bytes.insert(end(), byte_array, byte_array + nbytes);
    return *this;
  }

  ByteContainer &append(const std::string &other) {
    bytes.insert(end(), other.begin(), other.end());
    return *this;
  }

  void push_back(char c) {
    bytes.push_back(c);
  }

  reference operator[](size_type n) {
    assert(n < size());
    return bytes[n];
  }

  const_reference operator[](size_type n) const {
    assert(n < size());
    return bytes[n];
  }

  reference back() {
    return bytes.back();
  }

  const_reference back() const {
    return bytes.back();
  }

  reference front() {
    return bytes.front();
  }

  const_reference front() const {
    return bytes.front();
  }

  char* data() noexcept {
    return bytes.data();
  }

  const char* data() const noexcept {
    return bytes.data();
  }

  bool operator==(const ByteContainer& other) const {
    return bytes == other.bytes;
  }

  bool operator!=(const ByteContainer& other) const {
    return !(*this == other);
  }

  void reserve(size_t n) {
    bytes.reserve(n);
  }

  void resize(size_t n) {
    bytes.resize(n);
  }

  std::string to_hex(bool upper_case = false) const {
    std::ostringstream ret;

    for (std::string::size_type i = 0; i < size(); i++) {
      ret << std::setw(2) << std::setfill('0') << std::hex
	  << (upper_case ? std::uppercase : std::nouppercase)
	  << (int) static_cast<unsigned char>(bytes[i]);
    }

    return ret.str();
  }

private:
  vector<char> bytes;
};

struct ByteContainerKeyHash {
  std::size_t operator()(const ByteContainer& b) const {
    // Murmur, boost::hash_range or Jenkins?
    return boost::hash_range(b.begin(), b.end());
  }
};

#endif
