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

//! @file headers.h

#ifndef BM_BM_SIM_HEADERS_H_
#define BM_BM_SIM_HEADERS_H_

#include <bm/config.h>

#include <vector>
#include <string>
#include <set>

#include "fields.h"
#include "named_p4object.h"
#include "phv_forward.h"

namespace bm {

using header_type_id_t = p4object_id_t;

class VLHeaderExpression;
class ArithExpression;

class HeaderUnion;

class HeaderType : public NamedP4Object {
 public:
  // do not specify custome values for enum entries, the value is used directly
  // as an offset...
  enum class HiddenF {
    VALID
  };

  struct FInfo {
    std::string name;
    int bitwidth;
    bool is_signed;
    bool is_saturating;
    bool is_VL;
    bool is_hidden;
  };

  size_t get_hidden_offset(HiddenF hf) const {
    return fields_info.size() - 1 - static_cast<size_t>(hf);
  }

  HeaderType(const std::string &name, p4object_id_t id);

  // returns field offset
  int push_back_field(const std::string &field_name, int field_bit_width,
                      bool is_signed = false, bool is_saturating = false,
                      bool is_VL = false);

  // if field_length_expr is nullptr it means that the length will have to be
  // provided to extract
  int push_back_VL_field(
      const std::string &field_name,
      int max_header_bytes,
      std::unique_ptr<VLHeaderExpression> field_length_expr,
      bool is_signed = false,
      bool is_saturating = false);

  const FInfo &get_finfo(int field_offset) const {
    return fields_info.at(field_offset);
  }

  int get_bit_width(int field_offset) const {
    return fields_info.at(field_offset).bitwidth;
  }

  int get_bit_width() const {
    int bitwidth = 0;
    for (const auto &f_info : fields_info)
      bitwidth += f_info.bitwidth;
    return bitwidth;
  }

  const std::string &get_field_name(int field_offset) const {
    return fields_info.at(field_offset).name;
  }

  bool is_field_signed(int field_offset) const {
    return fields_info.at(field_offset).is_signed;
  }

  header_type_id_t get_type_id() const {
    return get_id();
  }

  int get_num_fields() const {
    return fields_info.size();
  }

  int get_field_offset(const std::string &field_name) const {
    for (size_t res = 0; res < fields_info.size(); res++) {
      if (field_name == fields_info[res].name) return res;
    }
    return -1;
  }

  bool is_VL_header() const {
    return (VL_offset >= 0);
  }

  bool has_VL_expr() const;

  std::unique_ptr<ArithExpression> resolve_VL_expr(header_id_t header_id) const;

  const std::vector<int> &get_VL_input_offsets() const;

  int get_VL_offset() const {
    return VL_offset;
  }

  int get_VL_max_header_bytes() const;

 private:
  std::vector<FInfo> fields_info;
  // used for VL headers only
  std::unique_ptr<VLHeaderExpression> VL_expr_raw;
  int VL_offset{-1};
  int VL_max_header_bytes{0};
};

//! Used to represent P4 header instances. It includes a vector of Field
//! objects.
class Header : public NamedP4Object {
 public:
  using iterator = std::vector<Field>::iterator;
  using const_iterator = std::vector<Field>::const_iterator;
  using reference = std::vector<Field>::reference;
  using const_reference = std::vector<Field>::const_reference;
  using size_type = size_t;

  friend class PHV;

 public:
  Header(const std::string &name, p4object_id_t id,
         const HeaderType &header_type, const std::set<int> &arith_offsets,
         const bool metadata = false);

  //! Returns the number of byte occupied by this header when it is deserialized
  //! in the packet
  int get_nbytes_packet() const {
    return nbytes_packet;
  }

  int recompute_nbytes_packet();

  //! Returns true if this header is marked valid
  bool is_valid() const {
    return (metadata || valid);
  }

  //! Returns true if this header represents metadata
  bool is_metadata() const {
    return metadata;
  }

  //! Marks the header as valid
  void mark_valid();

  //! Marks the header as not-valid
  void mark_invalid();

  //! Sets all the fields in the header to value `0`.
  void reset();

  void reset_VL_header();

  //! Set the written_to flag maintained by each field. This flag can be queried
  //! at any time by the target, using the Field interface, and can be used to
  //! check whether the field has been modified since written_to was last set to
  //! `false`.
  void set_written_to(bool written_to_value);

  //! Returns a reference to the Field at the specified offset, with bounds
  //! checking. If pos not within the range of the container, an exception of
  //! type std::out_of_range is thrown.
  Field &get_field(int field_offset) {
    return fields.at(field_offset);
  }

  //! @copydoc get_field
  const Field &get_field(int field_offset) const {
    return fields.at(field_offset);
  }

  const HeaderType &get_header_type() const { return header_type; }

  //! Returns an integral id which represents the header type of the
  //! header. This id can be used to compare whether 2 Header instances have the
  //! same header type.
  header_type_id_t get_header_type_id() const {
    return header_type.get_type_id();
  }

  //! Returns true if this header is an instance of a Variable-Length header
  //! type, i.e. contains a field whose length is determined when the header is
  //! extracted from the packet data.
  bool is_VL_header() const { return header_type.is_VL_header(); }

  // phv needed for variable length extraction
  void extract(const char *data, const PHV &phv);

  // extract a VL header for which the bitwidth of the VL field is already known
  void extract_VL(const char *data, int VL_nbits);

  void deparse(char *data) const;

  //! Returns the number of fields in the header
  size_type size() const noexcept { return fields.size(); }

  // iterators

  //! NC
  iterator begin() { return fields.begin(); }

  //! NC
  const_iterator begin() const { return fields.begin(); }

  //! NC
  iterator end() { return fields.end(); }

  //! NC
  const_iterator end() const { return fields.end(); }

  //! Access the character at position \p n. Will assert if \p n is greater or
  //! equal than the number of fields in the header.
  reference operator[](size_type n) {
    assert(n < fields.size());
    return fields[n];
  }

  //! @copydoc operator[]
  const_reference operator[](size_type n) const {
    assert(n < fields.size());
    return fields[n];
  }

  // useful for header stacks
  void swap_values(Header *other);

  void copy_fields(const Header &src);

  // compare to another header instance; returns true iff headers have the same
  // type, are both valid (irrelevant for metadata) and all the fields have the
  // same value.
  bool cmp(const Header &other) const;

#ifdef BM_DEBUG_ON
  void set_packet_id(const Debugger::PacketId *id);
#else
  void set_packet_id(const Debugger::PacketId *) { }
#endif

  //! Returns a reference to the name of the field at the given offset.
  const std::string &get_field_name(int field_offset) const;

  //! Returns the full name of the field at the given offset as a new
  //! string. The name is of the form <hdr_name>.<f_name>.
  const std::string get_field_full_name(int field_offset) const;

  Header(const Header &other) = delete;
  Header &operator=(const Header &other) = delete;

  Header(Header &&other) = default;
  // because of reference member, this is not possible
  Header &operator=(Header &&other) = delete;

 private:
  void extract_VL(const char *data, const PHV &phv);
  template <typename Fn>
  void extract_VL_common(const char *data, const Fn &VL_fn);

  // called by the PHV class
  void set_union_membership(HeaderUnion *header_union, size_t idx);

 private:
  struct UnionMembership {
    UnionMembership(HeaderUnion *header_union, size_t idx);

    void make_valid();
    void make_invalid();

    HeaderUnion *header_union;
    size_t idx;
  };

  const HeaderType &header_type;
  std::vector<Field> fields{};
  bool valid{false};
  // is caching this pointer here really useful?
  Field *valid_field{nullptr};
  bool metadata{false};
  int nbytes_packet{0};
  std::unique_ptr<ArithExpression> VL_expr;
  std::unique_ptr<UnionMembership> union_membership{nullptr};
#ifdef BM_DEBUG_ON
  const Debugger::PacketId *packet_id{&Debugger::dummy_PacketId};
#endif
};

}  // namespace bm

#endif  // BM_BM_SIM_HEADERS_H_
