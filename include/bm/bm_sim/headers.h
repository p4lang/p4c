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

#include <algorithm>  // for std::swap
#include <vector>
#include <string>
#include <set>

#include "fields.h"
#include "named_p4object.h"
#include "expressions.h"

namespace bm {

typedef p4object_id_t header_type_id_t;

class PHV;

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
    bool is_hidden;
  };

  size_t get_hidden_offset(HiddenF hf) const {
    return fields_info.size() - 1 - static_cast<size_t>(hf);
  }

  HeaderType(const std::string &name, p4object_id_t id);

  // returns field offset
  int push_back_field(const std::string &field_name, int field_bit_width,
                      bool is_signed = false);

  int push_back_VL_field(
      const std::string &field_name,
      std::unique_ptr<VLHeaderExpression> field_length_expr,
      bool is_signed = false);

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
    return (VL_expr_raw != nullptr);
  }

  std::unique_ptr<ArithExpression> resolve_VL_expr(
      header_id_t header_id) const {
    if (!is_VL_header()) return nullptr;
    std::unique_ptr<ArithExpression> expr(new ArithExpression());
    *expr = VL_expr_raw->resolve(header_id);
    return expr;
  }

  const std::vector<int> &get_VL_input_offsets() const {
    return VL_expr_raw->get_input_offsets();
  }

  int get_VL_offset() const {
    return VL_offset;
  }

 private:
  std::vector<FInfo> fields_info;
  // used for VL headers only
  std::unique_ptr<VLHeaderExpression> VL_expr_raw{nullptr};
  int VL_offset{-1};
};

//! Used to represent P4 header instances. It includes a vector of Field
//! objects.
class Header : public NamedP4Object {
 public:
  typedef std::vector<Field>::iterator iterator;
  typedef std::vector<Field>::const_iterator const_iterator;
  typedef std::vector<Field>::reference reference;
  typedef std::vector<Field>::const_reference const_reference;
  typedef size_t size_type;

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

  //! Sets all the fields in the header to value `0`
  void reset() {
    for (Field &f : fields)
      f.set(0);
  }

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
  bool is_VL_header() const { return VL_expr != nullptr; }

  // phv needed for variable length extraction
  void extract(const char *data, const PHV &phv);

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
  void swap_values(Header *other) {
    std::swap(valid, other->valid);
    // cannot do that, would invalidate references
    // std::swap(fields, other.fields);
    for (size_t i = 0; i < fields.size(); i++) {
      fields[i].swap_values(&other->fields[i]);
    }
  }

  void copy_fields(const Header &src) {
    for (size_t f = 0; f < fields.size(); f++)
      fields[f].copy_value(src.fields[f]);
  }

  void set_packet_id(const Debugger::PacketId *id);

  Header(const Header &other) = delete;
  Header &operator=(const Header &other) = delete;

  Header(Header &&other) = default;
  // because of reference member, this is not possible
  Header &operator=(Header &&other) = delete;

 private:
  void extract_VL(const char *data, const PHV &phv);

 private:
  const HeaderType &header_type;
  std::vector<Field> fields{};
  bool valid{false};
  // is caching this pointer here really useful?
  Field *valid_field{nullptr};
  bool metadata{false};
  int nbytes_packet{0};
  std::unique_ptr<ArithExpression> VL_expr{nullptr};
  const Debugger::PacketId *packet_id{&Debugger::dummy_PacketId};
};

}  // namespace bm

#endif  // BM_BM_SIM_HEADERS_H_
