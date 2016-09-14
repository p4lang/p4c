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

#include <bm/bm_sim/headers.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/expressions.h>

#include <string>
#include <set>
#include <map>

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
    {HeaderType::HiddenF::VALID, {"$valid$", 1, false, true}},
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
                            bool is_signed) {
  auto pos = fields_info.end() - HiddenFMap::size();
  auto offset = std::distance(fields_info.begin(), pos);
  fields_info.insert(pos, {field_name, field_bit_width, is_signed, false});
  return offset;
}

int
HeaderType::push_back_VL_field(
    const std::string &field_name,
    std::unique_ptr<VLHeaderExpression> field_length_expr,
    bool is_signed) {
  auto offset = push_back_field(field_name, 0, is_signed);
  // TODO(antonin)
  assert(!is_VL_header() && "header can only have one VL field");
  VL_expr_raw = std::move(field_length_expr);
  VL_offset = offset;
  return offset;
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
    fields.emplace_back(finfo.bitwidth, arith_flag, finfo.is_signed,
                        finfo.is_hidden);
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
}

void
Header::mark_valid() {
  valid = true;
  valid_field->set(1);
}

void
Header::mark_invalid() {
  valid = false;
  valid_field->set(0);
}

void Header::extract(const char *data, const PHV &phv) {
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

void Header::extract_VL(const char *data, const PHV &phv) {
  static thread_local Data computed_nbits;
  int VL_offset = header_type.get_VL_offset();
  int hdr_offset = 0;
  nbytes_packet = 0;
  for (int i = 0; i < header_type.get_num_fields(); i++) {
    Field &f = fields[i];
    if (f.is_hidden()) break;  // all hidden fields are at the end
    if (VL_offset == i) {
      VL_expr->eval(phv, &computed_nbits);
      hdr_offset += f.extract_VL(data, hdr_offset, computed_nbits.get_int());
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

void Header::deparse(char *data) const {
  // TODO(antonin): special case for VL header ?
  int hdr_offset = 0;
  for (const Field &f : fields) {
    if (f.is_hidden()) break;  // all hidden fields are at the end
    hdr_offset += f.deparse(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
}

void Header::set_packet_id(const Debugger::PacketId *id) {
  for (Field &f : fields) f.set_packet_id(id);
}

}  // namespace bm
