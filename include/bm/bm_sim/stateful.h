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

//! @file stateful.h
//! Includes both the Register and RegisterArray classes. RegisterArray is
//! exposed in P4 v1.0.2 as the `register` object. Action primitives can take a
//! RegisterArray reference as a parameter (but not a Register though). Here is
//! a possible implementation of the `register_write()` P4 primitive:
//! @code
//! class register_write
//!   : public ActionPrimitive<RegisterArray &, const Data &, const Data &> {
//!   void operator ()(RegisterArray &dst, const Data &idx, const Data &src) {
//!     dst[idx.get_uint()].set(src);
//!   }
//! };
//! @endcode

#ifndef BM_BM_SIM_STATEFUL_H_
#define BM_BM_SIM_STATEFUL_H_

#include <boost/thread/locks.hpp>  // for boost::lock

#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>

#include "data.h"
#include "bignum.h"
#include "named_p4object.h"
#include "short_alloc.h"

namespace bm {

//! A Register object is essentially just a Data object, meant to live in a
//! RegisterArray. Use the Data class methods to read and write to a Register.
class Register : public Data {
 public:
  enum RegisterErrorCode {
    SUCCESS = 0,
    INVALID_REGISTER_NAME,
    INVALID_INDEX,
    ERROR
  };

 public:
  explicit Register(int nbits)
    : nbits(nbits) {
    mask <<= nbits; mask -= 1;
  }

  void export_bytes() {
    value &= mask;
  }

 private:
  int nbits;
  Bignum mask{1};
};

typedef p4object_id_t register_array_id_t;

//! RegisterArray corresponds to the `register` standard P4 v1.02 object. A
//! RegisterArray reference can be used as a P4 primitive parameter. For
//! example:
//! @code
//! class register_write
//!   : public ActionPrimitive<RegisterArray &, const Data &, const Data &> {
//!   void operator ()(RegisterArray &dst, const Data &idx, const Data &src) {
//!     dst[idx.get_uint()].set(src);
//!   }
//! };
//! @endcode
class RegisterArray : public NamedP4Object {
  friend class RegisterSync;

 public:
  typedef std::vector<Register>::iterator iterator;
  typedef std::vector<Register>::const_iterator const_iterator;

  typedef std::unique_lock<std::mutex> UniqueLock;

  RegisterArray(const std::string &name, p4object_id_t id,
                size_t size, int bitwidth)
      : NamedP4Object(name, id), bitwidth(bitwidth) {
    registers.reserve(size);
    for (size_t i = 0; i < size; i++)
      registers.emplace_back(bitwidth);
  }

  //! Access the register at position \p idx, asserts if bad \p idx
  Register &operator[](size_t idx) {
    assert(idx < size());
    return registers[idx];
  }

  //! @copydoc operator[]
  const Register &operator[](size_t idx) const {
    assert(idx < size());
    return registers[idx];
  }

  //! Access the register at position \p idx, throws a std::out_of_range
  //! exception if \p idx is invalid
  Register &at(size_t idx) {
    return registers.at(idx);
  }

  //! @copydoc at
  const Register &at(size_t idx) const {
    return registers.at(idx);
  }

  // iterators

  //! NC
  iterator begin() { return registers.begin(); }

  //! NC
  const_iterator begin() const { return registers.begin(); }

  //! NC
  iterator end() { return registers.end(); }

  //! NC
  const_iterator end() const { return registers.end(); }

  //! Return the size of the RegisterArray (i.e. number of registers it
  //! includes)
  size_t size() const { return registers.size(); }

  void reset_state();

  //! Request exclusive access to this register array. This method needs to be
  //! called when the target needs to read or write a register. Note that it is
  //! never necessary to call this method in a primitive action, since when an
  //! action is executed, it is guaranteed exclusive access to all the register
  //! arrays it reads or writes.
  UniqueLock unique_lock() const { return UniqueLock(m_mutex); }
  // NOLINTNEXTLINE(runtime/references)
  void unlock(UniqueLock &lock) const { lock.unlock(); }

 private:
  std::vector<Register> registers{};
  mutable std::mutex m_mutex{};
  int bitwidth{};
};


// This class was added to provide some measure of concurrency support for
// register accesses. Every time an action is executed, this action is given
// exclusive access to all the registers it is referring to. Same thing for a
// parse state.
class RegisterSync {
 public:
  using Lock = RegisterArray::UniqueLock;

  template <size_t NumLocks = 4>
  using LockVector = std::vector<
    Lock, detail::short_alloc<Lock, NumLocks * sizeof(Lock), alignof(Lock)> >;

  struct RegisterLocks {
    LockVector<>::allocator_type::arena_type a;
    LockVector<> v{a};
  };

  void add_register_array(RegisterArray *register_array);

  // tried NRVO, but RegisterLocks not movable
  void lock(RegisterLocks *RL) const {
    for (auto m : mutexes) RL->v.emplace_back(*m, std::defer_lock);
    boost::lock(RL->v.begin(), RL->v.end());
  }

 private:
  mutable std::vector<std::mutex *> mutexes{};
  std::unordered_set<const RegisterArray *> register_arrays{};
};

}  // namespace bm

#endif  // BM_BM_SIM_STATEFUL_H_
