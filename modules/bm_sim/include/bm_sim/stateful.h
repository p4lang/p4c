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

#ifndef _BM_STATEFUL_H_
#define _BM_STATEFUL_H_

#include <array>

#include "data.h"
#include "bignum.h"

class Register : public Data
{
public:
  enum RegisterErrorCode {
    SUCCESS = 0,
    INVALID_INDEX,
    ERROR
  };

public:
  Register(int nbits)
    : nbits(nbits) {
    mask <<= nbits; mask -= 1;
  }

  void export_bytes() {
    value &= mask;
  }

private:
  typedef std::unique_lock<std::mutex> UniqueLock;
  UniqueLock unique_lock() const { return UniqueLock(*m_mutex); }
  void unlock(UniqueLock &lock) const { lock.unlock(); }

private:
  int nbits;
  Bignum mask{1};
  // mutexes are not movable, extra indirection bad?
  std::unique_ptr<std::mutex> m_mutex{new std::mutex()};
};

typedef p4object_id_t register_array_id_t;

class RegisterArray : public NamedP4Object {
public:
  typedef vector<Register>::iterator iterator;
  typedef vector<Register>::const_iterator const_iterator;

  RegisterArray(const std::string &name, p4object_id_t id,
		size_t size, int bitwidth)
    : NamedP4Object(name, id) {
    registers.reserve(size);
    for(size_t i = 0; i < size; i++)
      registers.emplace_back(bitwidth);
  }

  Register &operator[](size_t idx) {
    assert(idx < size());
    return registers[idx];
  }

  const Register &operator[](size_t idx) const {
    assert(idx < size());
    return registers[idx];
  }

  // iterators
  iterator begin() { return registers.begin(); }

  const_iterator begin() const { return registers.begin(); }

  iterator end() { return registers.end(); }

  const_iterator end() const { return registers.end(); }

  size_t size() const { return registers.size(); }

private:
    std::vector<Register> registers;
};

#endif
