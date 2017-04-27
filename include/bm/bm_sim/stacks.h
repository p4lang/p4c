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

//! @file stacks.h

#ifndef BM_BM_SIM_STACKS_H_
#define BM_BM_SIM_STACKS_H_

#include <string>
#include <vector>

#include "headers.h"
#include "header_unions.h"
#include "named_p4object.h"

namespace bm {

using header_stack_id_t = p4object_id_t;

//! Interface to a P4 stack (of headers or header unions).
//! A StackIface reference can be used in an action primitive. For example:
//! @code
//! struct pop : public ActionPrimitive<StackIface &, const Data &> {
//!  void operator ()(StackIface &stack, const Data &num) {
//!    stack.pop_front(num.get<size_t>());
//! };
//! @endcode
class StackIface {
 public:
  virtual ~StackIface() { }

  //! Removes the first element of the stack. Returns the number of elements
  //! removed, which is `0` if the stack is empty and `1` otherwise. The second
  //! element of the stack becomes the first element, and so on...
  virtual size_t pop_front() = 0;

  //! Removes the first \p num element of the stack. Returns the number of
  //! elements removed, which is `0` if the stack is empty. Calling this
  //! function is more efficient than calling pop_front() multiple times.
  virtual size_t pop_front(size_t num) = 0;

  //! Pushes an element to the front of the stack. If the stack is already full,
  //! the last element of the stack will be discarded. This function returns the
  //! number of elements pushed, which is guaranteed to be always 1. The first
  //! element of the stack is marked valid (note that it was already valid if
  //! the stack was not empty).
  virtual size_t push_front() = 0;

  //! Pushes \p num elements to the front of the stack. If the stack becomes
  //! full, the extra elements at the bottom of the stack will be
  //! discarded. This function returns the number of elements pushed, which is
  //! guaranteed to be min(get_depth(), \p num)`. All inserted elements are
  //! guaranteed to be marked valid. Calling this function is more efficient
  //! that calling push_front() multiple times.
  virtual size_t push_front(size_t num) = 0;

  virtual size_t pop_back() = 0;

  // basically, meant to be called by the parser
  virtual size_t push_back() = 0;

  //! Returns the maximum capacity of the stack
  virtual size_t get_depth() const = 0;

  //! Returns the current occupancy of the stack
  virtual size_t get_count() const = 0;

  //! Returns true if the header stack is full
  virtual bool is_full() const = 0;

  virtual void reset() = 0;
};

//! Stack is used to represent header and union stacks in P4. The Stack class
//! itself does not store any union / header / field data itself, but stores
//! references to the HeaderUnion / Header instances which constitute the stack,
//! as well as the stack internal state (e.g. number of valid headers in the
//! stack).
template <typename T>
class Stack : public StackIface, public NamedP4Object {
 public:
  friend class PHV;

  Stack(const std::string &name, p4object_id_t id);

  size_t pop_front() override;
  size_t pop_front(size_t num) override;

  size_t push_front() override;
  size_t push_front(size_t num) override;

  size_t pop_back() override;

  size_t push_back() override;

  size_t get_depth() const override;
  size_t get_count() const override;
  bool is_full() const override;

  void reset() override;

  T &get_last();
  const T &get_last() const;

  T &get_next();
  const T &get_next() const;

  T &at(size_t idx);
  const T &at(size_t idx) const;

 private:
  using TRef = std::reference_wrapper<T>;

  // To be called by PHV class
  // This is a special case, as I want to store a reference
  // NOLINTNEXTLINE
  void set_next_element(T &e);

  std::vector<TRef> elements{};
  // first empty index; if next == headers.size(), stack is full
  size_t next{0};
};

using header_stack_id_t = p4object_id_t;
using header_union_stack_id_t = p4object_id_t;

//! Convenience alias for stacks of headers
//! A HeaderStack reference can be used in an action primitive. For example:
//! @code
//! struct my_primitive : public ActionPrimitive<HeaderStack &, const Data &> {
//!  void operator ()(HeaderStack &header_stack, const Data &num) {
//!    // ...
//! };
//! @endcode
using HeaderStack = Stack<Header>;
//! Convenience alias for stacks of header unions
//! A HeaderUnionStack reference can be used in an action primitive. For
//! example:
//! @code
//! struct my_primitive
//!   : public ActionPrimitive<HeaderUnionStack &, const Data &> {
//!  void operator ()(HeaderUnionStack &header_union_stack, const Data &num) {
//!    // ...
//! };
//! @endcode
using HeaderUnionStack = Stack<HeaderUnion>;

}  // namespace bm

#endif  // BM_BM_SIM_STACKS_H_
