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

//! @file header_unions.h

#ifndef BM_BM_SIM_HEADER_UNIONS_H_
#define BM_BM_SIM_HEADER_UNIONS_H_

#include <algorithm>  // std::swap
#include <vector>
#include <string>
#include <functional>  // std::reference_wrapper

#include "named_p4object.h"
#include "headers.h"

namespace bm {

using header_union_id_t = p4object_id_t;

//! Used to represent P4 header union instances. It stores references to the
//! Header instances included in the union and ensures that at most one of these
//! instances is valid at any given time.
//!
//! A HeaderUnion reference can be used in an action primitive. For example:
//! @code
//! class my_primitive : public ActionPrimitive<HeaderUnion &, const Data &> {
//!   void operator ()(HeaderUnion &header_union) {
//!     // ...
//!   }
//! };
//! @endcode
class HeaderUnion : public NamedP4Object {
 public:
  friend class PHV;

  HeaderUnion(const std::string &name, p4object_id_t id)
      : NamedP4Object(name, id) { }

  void make_header_valid(size_t new_valid_idx) {
    if (valid && valid_header_idx != new_valid_idx)
      headers[valid_header_idx].get().mark_invalid();
    valid_header_idx = new_valid_idx;
    valid = true;
    // the caller is responsible for making the new header valid
  }

  void make_header_invalid(size_t idx) {
    if (valid && valid_header_idx == idx) valid = false;
  }

  // used for stacks of header unions
  void swap_values(HeaderUnion *other) {
    assert(headers.size() == other->headers.size());
    std::swap(valid, other->valid);
    std::swap(valid_header_idx, other->valid_header_idx);
    // this is probably too conservative, I think we only need to swap valid
    // headers, but this is a good place to start
    for (size_t i = 0; i < headers.size(); i++)
      headers[i].get().swap_values(&other->headers[i].get());
  }

  //! Returns a pointer to the valid Header instance in the union, or nullptr if
  //! no header is valid.
  Header *get_valid_header() const {
    return valid ? &headers[valid_header_idx].get() : nullptr;
  }

  //! Returns the number of headers included in the union.
  size_t get_num_headers() const { return headers.size(); }

  //! Access the Header at index \p idx in the union. If the index is invalid,
  //! an std::out_of_range exception will be thrown.
  Header &at(size_t idx) { return headers.at(idx); }
  //! @copydoc at(size_t idx)
  const Header &at(size_t idx) const { return headers.at(idx); }

  // compare to another union instance; returns true iff both unions are valid,
  // and the valid headers are the same in both unions.
  bool cmp(const HeaderUnion &other) const;

  //! Returns true if one of the headers in the union is valid.
  bool is_valid() const { return valid; }

  // no-ops on purpose
  void mark_invalid() { }
  void mark_valid() { }

 private:
  // To be called by PHV class
  // This is a special case, as I want to store a reference
  // NOLINTNEXTLINE
  void set_next_header(Header &h) {
    headers.emplace_back(h);
  }

  using HeaderRef = std::reference_wrapper<Header>;

  std::vector<HeaderRef> headers{};
  bool valid{false};
  size_t valid_header_idx{0};
};

}  // namespace bm

#endif  // BM_BM_SIM_HEADER_UNIONS_H_
