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

#include <bm/config.h>

#include <bm/bm_sim/headers.h>
#include <bm/bm_sim/header_unions.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/expressions.h>

#include <algorithm>  // for std::swap
#include <string>
#include <set>
#include <map>
#include <vector>

namespace bm {

namespace {

// hidden fields in the map needs to be ordered just like the enum, which may
// not be very robust code...
class HiddenFMap {
 public:
  // TODO(antonin): find something more elegant?
  static const std::map<HeaderType::HiddenF, HeaderType::FInfo> &map();
  static size_t size();

 private:
  static HiddenFMap *get_instance();
  HiddenFMap();

  std::map<HeaderType::HiddenF, HeaderType::FInfo> fmap;
};

HiddenFMap::HiddenFMap() {
  fmap = {
    {HeaderType::HiddenF::VALID, {"$valid$", 1, false, false, false, true}},
  };
}

HiddenFMap *
HiddenFMap::get_instance() {
  static HiddenFMap instance;
  return &instance;
}

const std::map<HeaderType::HiddenF, HeaderType::FInfo> &
HiddenFMap::map() {
  auto instance = get_instance();
  return instance->fmap;
}

size_t
HiddenFMap::size() {
  return map().size();
}

}  // namespace

HeaderType::HeaderType(const std::string &name, p4object_id_t id)
    : NamedP4Object(name, id) {
  const auto &fmap = HiddenFMap::map();
  for (auto p : fmap) fields_info.push_back(p.second);
}

int
HeaderType::push_back_field(const std::string &field_name, int field_bit_width,
                            bool is_signed, bool is_saturating, bool is_VL) {
  auto pos = fields_info.end() - HiddenFMap::size();
  auto offset = std::distance(fields_info.begin(), pos);
  fields_info.insert(
      pos,
      {field_name, field_bit_width, is_signed, is_saturating, is_VL, false});
  return offset;
}

int
HeaderType::push_back_VL_field(
    const std::string &field_name,
    int max_header_bytes,
    std::unique_ptr<VLHeaderExpression> field_length_expr,
    bool is_signed,
    bool is_saturating) {
  auto offset = push_back_field(field_name, 0, is_signed, is_saturating, true);
  // TODO(antonin)
  assert(!is_VL_header() && "header can only have one VL field");
  VL_expr_raw = std::move(field_length_expr);
  VL_offset = offset;
  VL_max_header_bytes = max_header_bytes;
  return offset;
}

bool
HeaderType::has_VL_expr() const {
  return (VL_expr_raw != nullptr);
}

std::unique_ptr<ArithExpression>
HeaderType::resolve_VL_expr(header_id_t header_id) const {
  if (!is_VL_header()) return nullptr;
  // expression will be provided by extract, so nothing to do
  if (VL_expr_raw == nullptr) return nullptr;
  std::unique_ptr<ArithExpression> expr(new ArithExpression());
  *expr = VL_expr_raw->resolve(header_id);
  return expr;
}

const std::vector<int> &
HeaderType::get_VL_input_offsets() const {
  return VL_expr_raw->get_input_offsets();
}

int
HeaderType::get_VL_max_header_bytes() const {
  return VL_max_header_bytes;
}


Header::Header(const std::string &name, p4object_id_t id,
               const HeaderType &header_type,
               const std::set<int> &arith_offsets,
               const bool metadata)
    : NamedP4Object(name, id), header_type(header_type), metadata(metadata) {
  // header_type_id = header_type.get_type_id();
  for (int i = 0; i < header_type.get_num_fields(); i++) {
    const auto &finfo = header_type.get_finfo(i);
    bool arith_flag = true;
    if (arith_offsets.find(i) == arith_offsets.end()) {
      arith_flag = false;
    }
    fields.emplace_back(finfo.bitwidth, this, arith_flag, finfo.is_signed,
                        finfo.is_hidden, finfo.is_VL, finfo.is_saturating);
    uint64_t field_unique_id = id;
    field_unique_id <<= 32;
    field_unique_id |= i;
    fields.back().set_id(field_unique_id);
    if (!finfo.is_hidden) nbytes_packet += fields.back().get_nbits();
  }
  assert(nbytes_packet % 8 == 0);
  nbytes_packet /= 8;
  valid_field = &fields.at(header_type.get_hidden_offset(
      HeaderType::HiddenF::VALID));

  for (int i = 0; i < header_type.get_num_fields(); i++) {
    const auto &finfo = header_type.get_finfo(i);
    if (finfo.is_VL) {
      fields.at(i).reserve_VL(
          header_type.get_VL_max_header_bytes() - nbytes_packet);
      break;
    }
  }
}

int
Header::recompute_nbytes_packet() {
  nbytes_packet = 0;
  for (const auto &f : fields) {
    if (f.is_hidden()) break;  // all hidden fields are at the end
    nbytes_packet += f.get_nbits();
  }
  assert(nbytes_packet % 8 == 0);
  nbytes_packet /= 8;
  return nbytes_packet;
}

void
Header::mark_valid() {
  valid = true;
  valid_field->set(1);
  if (union_membership) union_membership->make_valid();
}

void
Header::mark_invalid() {
  valid = false;
  valid_field->set(0);
  if (union_membership) union_membership->make_invalid();
}

void
Header::reset() {
  for (Field &f : fields)
    f.set(0);
}

void
Header::reset_VL_header() {
  if (!is_VL_header()) return;
  int VL_offset = header_type.get_VL_offset();
  auto &VL_f = fields[VL_offset];
  // this works because we only support VL fields whose bitwidth is a multiple
  // of 8
  nbytes_packet -= VL_f.get_nbytes();
  VL_f.reset_VL();
}

void
Header::set_written_to(bool written_to_value) {
  for (Field &f : fields)
    f.set_written_to(written_to_value);
}

void
Header::extract(const char *data, const PHV &phv) {
  if (is_VL_header()) return extract_VL(data, phv);
  int hdr_offset = 0;
  for (Field &f : fields) {
    if (f.is_hidden()) break;  // all hidden fields are at the end
    hdr_offset += f.extract(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
  mark_valid();
}

template <typename Fn>
void
Header::extract_VL_common(const char *data, const Fn &VL_fn) {
  int VL_offset = header_type.get_VL_offset();
  int hdr_offset = 0;
  nbytes_packet = 0;
  for (int i = 0; i < header_type.get_num_fields(); i++) {
    Field &f = fields[i];
    if (f.is_hidden()) break;  // all hidden fields are at the end
    if (VL_offset == i) {
      hdr_offset += f.extract_VL(data, hdr_offset, VL_fn());
    } else {
      hdr_offset += f.extract(data, hdr_offset);
    }
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
    nbytes_packet += f.get_nbits();
  }
  assert(nbytes_packet % 8 == 0);
  nbytes_packet /= 8;
  mark_valid();
}

void
Header::extract_VL(const char *data, int VL_nbits) {
  extract_VL_common(data, [VL_nbits]() { return VL_nbits; });
}

void
Header::extract_VL(const char *data, const PHV &phv) {
  static thread_local Data computed_nbits;
  auto VL_fn = [&phv, this]() {
    VL_expr->eval(phv, &computed_nbits);
    return computed_nbits.get<int>();
  };
  extract_VL_common(data, VL_fn);
}

void
Header::deparse(char *data) const {
  // TODO(antonin): special case for VL header ?
  int hdr_offset = 0;
  for (const Field &f : fields) {
    if (f.is_hidden()) break;  // all hidden fields are at the end
    hdr_offset += f.deparse(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
}

#ifdef BM_DEBUG_ON
void
Header::set_packet_id(const Debugger::PacketId *id) {
  for (Field &f : fields) f.set_packet_id(id);
}
#endif

const std::string &
Header::get_field_name(int field_offset) const {
  return header_type.get_field_name(field_offset);
}

const std::string
Header::get_field_full_name(int field_offset) const {
  return name + "." + get_field_name(field_offset);
}

void
Header::swap_values(Header *other) {
  std::swap(valid, other->valid);
  // cannot do that, would invalidate references
  // std::swap(fields, other.fields);
  for (size_t i = 0; i < fields.size(); i++)
    fields[i].swap_values(&other->fields[i]);
  // in case header has a VL field
  std::swap(nbytes_packet, other->nbytes_packet);
}

void
Header::copy_fields(const Header &src) {
  for (size_t f = 0; f < fields.size(); f++)
    fields[f].copy_value(src.fields[f]);
  // in case header has a VL field
  nbytes_packet = src.nbytes_packet;
}

bool
Header::cmp(const Header &other) const {
  return (header_type.get_type_id() == other.header_type.get_type_id()) &&
      is_valid() && other.is_valid() &&
      (fields == other.fields);
}

Header::UnionMembership::UnionMembership(HeaderUnion *header_union, size_t idx)
    : header_union(header_union), idx(idx) { }

void
Header::UnionMembership::make_valid() {
  header_union->make_header_valid(idx);
}

void
Header::UnionMembership::make_invalid() {
  header_union->make_header_invalid(idx);
}

void
Header::set_union_membership(HeaderUnion *header_union, size_t idx) {
  union_membership.reset(new UnionMembership(header_union, idx));
}

}  // namespace bm
