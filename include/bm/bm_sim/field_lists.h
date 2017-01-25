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

//! @file field_lists.h

#ifndef BM_BM_SIM_FIELD_LISTS_H_
#define BM_BM_SIM_FIELD_LISTS_H_

#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>

#include <utility>  // for pair<>
#include <vector>
#include <unordered_set>
#include <string>

#include "phv_forward.h"
#include "bytecontainer.h"

namespace bm {

//! Corresponds to a `field_list` object in P4 v1.0.2. Some targets -this is the
//! case for the simple switch target- need to access FieldList instances (using
//! Context::get_field_list() or Switch::get_field_list()). The simple switch
//! target uses this to implement metadata carry when cloning a packet.
class FieldList {
 public:
  struct field_t {
    header_id_t header;
    int offset;

    bool operator==(const field_t& other) const {
      return header == other.header && offset == other.offset;
    }
    bool operator!=(const field_t& other) const {
      return !(*this == other);
    }
  };

  struct constant_t {
    ByteContainer value;
    size_t bitwidth;

    bool operator==(const constant_t& other) const {
      return value == other.value;
    }
    bool operator!=(const constant_t& other) const {
      return !(*this == other);
    }
  };

  struct FieldListVisitor : boost::static_visitor<> {
    void operator()(const field_t &) { }
    void operator()(const constant_t &) { }
  };

 public:
  void push_back_field(header_id_t header, int field_offset) {
    field_t f = {header, field_offset};
    fields.push_back(field_list_member_t(f));
    fields_set.insert(f);
  }

  void push_back_constant(const std::string &hexstr, size_t bitwidth) {
    constant_t c = {ByteContainer(hexstr), bitwidth};
    fields.push_back(field_list_member_t(c));
  }

  //! Returns true if the FieldList contains the given field, identified by the
  //! header id and the offset of the field in the header
  bool contains_field(header_id_t header, int field_offset) const {
    auto it = fields_set.find({header, field_offset});
    return it != fields_set.end();
  }

  template <typename T>
  // NOLINTNEXTLINE(runtime/references)
  void visit(T &visitor) {
    static_assert(std::is_base_of<FieldListVisitor, T>::value,
                  "Invalid visitor, must inherit from from FieldListVisitor");
    std::for_each(fields.begin(), fields.end(), boost::apply_visitor(visitor));
  }

  void copy_fields_between_phvs(PHV *dst, const PHV *src) {
    struct CopyFieldsVisitor : FieldListVisitor {
      CopyFieldsVisitor(PHV *dst, const PHV *src)
          : dst(dst), src(src) {}
      PHV *dst;
      const PHV *src;

      void operator()(const field_t &f) {
        dst->get_field(f.header, f.offset)
            .set(src->get_field(f.header, f.offset));
      }
      void operator()(const constant_t &) { }
    };

    CopyFieldsVisitor v(dst, src);
    visit(v);
  }

 private:
  using field_list_member_t = boost::variant<field_t, constant_t>;

  struct FieldKeyHash {
    std::size_t operator()(const field_t& f) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, f.header);
      boost::hash_combine(seed, f.offset);
      return seed;
    }
  };

 private:
  std::vector<field_list_member_t> fields{};
  std::unordered_set<field_t, FieldKeyHash> fields_set{};
};

}  // namespace bm

#endif  // BM_BM_SIM_FIELD_LISTS_H_
