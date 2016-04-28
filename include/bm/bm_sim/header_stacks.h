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

//! @file header_stacks.h

#ifndef BM_BM_SIM_HEADER_STACKS_H_
#define BM_BM_SIM_HEADER_STACKS_H_

#include <vector>
#include <utility>
#include <algorithm>
#include <string>

#include "named_p4object.h"
#include "headers.h"
#include "phv.h"

namespace bm {

typedef p4object_id_t header_stack_id_t;

//! HeaderStack is used to reprsent header stacks in P4. The HeaderStack class
//! does not store any header / field data itself, but stores references to
//! the Header instances which constitute the stack, as well as the stack
//! internal state (e.g. number of valid headers in the stack).
//!
//! A HeaderStack reference can be used in an action primitive. For example:
//! @code
//! class pop : public ActionPrimitive<HeaderStack &, const Data &> {
//!   void operator ()(HeaderStack &stack, const Data &num) {
//!     stack.pop_front(num.get_uint());
//!   }
//! };
//! @endcode
class HeaderStack : public NamedP4Object {
 public:
  friend class PHV;

 public:
  HeaderStack(const std::string &name, p4object_id_t id,
              const HeaderType &header_type)
    : NamedP4Object(name, id),
      header_type(header_type) { }

  //! Removes the first element of the stack. Returns the number of elements
  //! removed, which is `0` if the stack is empty and `1` otherwise. The second
  //! element of the stack becomes the first element, and so on...
  size_t pop_front() {
    if (next == 0) return 0u;
    next--;
    for (size_t i = 0; i < next; i++) {
      headers[i].get().swap_values(&headers[i + 1].get());
    }
    headers[next].get().mark_invalid();
    return 1u;
  }

  //! Removes the first \p num element of the stack. Returns the number of
  //! elements removed, which is `0` if the stack is empty. Calling this
  //! function is more efficient than calling pop_front() multiple times.
  size_t pop_front(size_t num) {
    if (num == 0) return 0;
    size_t popped = std::min(next, num);
    next -= popped;
    for (size_t i = 0; i < next; i++) {
      headers[i].get().swap_values(&headers[i + num].get());
    }
    for (size_t i = next; i < next + popped; i++) {
      headers[i].get().mark_invalid();
    }
    return popped;
  }

  //! Pushes an element to the front of the stack. If the stack is already full,
  //! the last header of the stack will be discarded. This function returns the
  //! number of elements pushed, which is guaranteed to be always 1. The first
  //! header of the stack is marked valid (note that it was already valid, if
  //! the stack was not empty).
  size_t push_front() {
    if (next < headers.size()) next++;
    for (size_t i = next - 1; i > 0; i--) {
      headers[i].get().swap_values(&headers[i - 1].get());
    }
    // TODO(antonin): do I want to reset the header as well?
    // this may be complicated given the design
    headers[0].get().mark_valid();
    return 1u;
  }

  //! Pushes \p num elements to the front of the stack. If the stack becomes
  //! full, the extra headers at the bottom of the stack will be discarded. This
  //! function returns the number of elements pushed, which is guaranteed to be
  //! min(get_depth(), \p num)`. All inserted headers are guaranteed to be
  //! marked valid. Calling this function is more efficient that calling
  //! push_front() multiple times.
  size_t push_front(size_t num) {
    if (num == 0) return 0;
    next = std::min(headers.size(), next + num);
    for (size_t i = next - 1; i > num - 1; i--) {
      headers[i].get().swap_values(&headers[i - num].get());
    }
    size_t pushed = std::min(headers.size(), num);
    for (size_t i = 0; i < pushed; i++) {
      headers[i].get().mark_valid();
    }
    return pushed;
  }

  // TODO(antonin): push_front(Header &h) where h is copied ?

  size_t pop_back() {
    if (next == 0) return 0u;
    next--;
    headers[next].get().mark_invalid();
    return 1u;
  }

  // basically, meant to be called by the parser
  size_t push_back() {
    if (next == headers.size()) return 0u;
    headers[next].get().mark_valid();
    next++;
    return 1u;
  }

  //! Returns the maximum capacity of the stack
  size_t get_depth() const { return headers.size(); }

  //! Returns the current occupancy of the stack
  size_t get_count() const { return next; }

  // TODO(antonin): do we really need those, or will the compiler do loop
  // unrolling, in which case we will just have an increase number of parse
  // states

  Header &get_last() {
    assert(next > 0 && "header stack empty");
    return headers[next - 1];
  }

  const Header &get_last() const {
    assert(next > 0 && "header stack empty");
    return headers[next - 1];
  }

  Header &get_next() {
    assert(next < headers.size() && "header stack full");
    return headers[next];
  }

  const Header &get_next() const {
    assert(next < headers.size() && "header stack full");
    return headers[next];
  }

  void reset() {
    next = 0;
  }

 private:
  // To be called by PHV class
  // This is a special case, as I want to store a reference
  // NOLINTNEXTLINE
  void set_next_header(Header &h) {
    headers.emplace_back(h);
  }

 private:
  typedef std::reference_wrapper<Header> HeaderRef;

 private:
  const HeaderType &header_type;
  std::vector<HeaderRef> headers{};
  // first empty index; if next == headers.size(), stack is full
  size_t next{0};
};

}  // namespace bm

#endif  // BM_BM_SIM_HEADER_STACKS_H_
