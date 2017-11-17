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

#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <vector>
#include <set>

namespace bm {

PHV::PHV(size_t num_headers, size_t num_header_stacks,
         size_t num_header_unions, size_t num_header_union_stacks)
    : capacity(num_headers), capacity_stacks(num_header_stacks),
      capacity_unions(num_header_unions),
      capacity_union_stacks(num_header_union_stacks) {
  // this is needed, otherwise our references will not be valid anymore
  headers.reserve(num_headers);
  header_stacks.reserve(num_header_stacks);
  header_unions.reserve(num_header_unions);
  header_union_stacks.reserve(num_header_union_stacks);
}

void
PHV::reset() {
  for (auto &h : headers) {
    h.mark_invalid();
    if (h.is_VL_header()) h.reset_VL_header();
  }
}

void
PHV::reset_header_stacks() {
  for (auto &hs : header_stacks)
    hs.reset();
  for (auto &hus : header_union_stacks)
    hus.reset();
}

// so slow I want to die, but optional for a target...
// I need to find a better way of doing this
void
PHV::reset_metadata() {
  for (auto &h : headers) {
    if (h.is_metadata())
      h.reset();
  }
}

void
PHV::reset_headers() {
  for (auto &h : headers)
    h.reset();
}

void
PHV::set_written_to(bool written_to_value) {
  for (auto &h : headers)
    h.set_written_to(written_to_value);
}

void
PHV::copy_headers(const PHV &src) {
  for (size_t h = 0; h < headers.size(); h++) {
    headers[h].valid = src.headers[h].valid;
    headers[h].metadata = src.headers[h].metadata;
    if (headers[h].valid || headers[h].metadata)
      headers[h].copy_fields(src.headers[h]);
  }
  for (size_t hs = 0; hs < header_stacks.size(); hs++) {
    header_stacks[hs].next = src.header_stacks[hs].next;
  }
  for (size_t hu = 0; hu < header_unions.size(); hu++) {
    header_unions[hu].valid = src.header_unions[hu].valid;
    header_unions[hu].valid_header_idx = src.header_unions[hu].valid_header_idx;
  }
  for (size_t hus = 0; hus < header_union_stacks.size(); hus++) {
    header_union_stacks[hus].next = src.header_union_stacks[hus].next;
  }
}

void
PHV::push_back_header(const std::string &header_name,
                      header_id_t header_index,
                      const HeaderType &header_type,
                      const std::set<int> &arith_offsets,
                      const bool metadata) {
  assert(header_index < static_cast<int>(capacity));
  assert(header_index == static_cast<int>(headers.size()));
  // cannot call push_back here, as the Header constructor passes "this" to the
  // Field constructor (i.e. Header cannot be moved or the pointer would be
  // invalid); this is not a very robust design
  headers.emplace_back(
      header_name, header_index, header_type, arith_offsets, metadata);
  headers.back().set_packet_id(&packet_id);

  headers_map.emplace(header_name, get_header(header_index));

  for (int i = 0; i < header_type.get_num_fields(); i++) {
    const std::string name = header_name + "." + header_type.get_field_name(i);
    // std::cout << header_index << " " << i << " " << name << std::endl;
    fields_map.emplace(name, get_field(header_index, i));
  }

  if (header_type.is_VL_header()) {
    headers.back().VL_expr = header_type.resolve_VL_expr(header_index);

    if (headers.back().VL_expr != nullptr) {
      for (const int offset : header_type.get_VL_input_offsets())
        headers.back()[offset].set_arith(true);
    }
  }
}

void
PHV::push_back_header_stack(const std::string &header_stack_name,
                            header_stack_id_t header_stack_index,
                            const HeaderType &header_type,
                            const std::vector<header_id_t> &header_ids) {
  (void) header_type;  // not used anymore
  assert(header_stack_index < static_cast<int>(capacity_stacks));
  assert(header_stack_index == static_cast<int>(header_stacks.size()));
  HeaderStack header_stack(header_stack_name, header_stack_index);
  for (header_id_t header_id : header_ids) {
    header_stack.set_next_element(get_header(header_id));
  }
  header_stacks.push_back(std::move(header_stack));
}

void
PHV::push_back_header_union(const std::string &header_union_name,
                            header_union_id_t header_union_index,
                            const std::vector<header_id_t> &header_ids) {
  assert(header_union_index < static_cast<int>(capacity_unions));
  assert(header_union_index == static_cast<int>(header_unions.size()));
  HeaderUnion header_union(header_union_name, header_union_index);
  for (header_id_t header_id : header_ids) {
    header_union.set_next_header(get_header(header_id));
  }
  header_unions.push_back(std::move(header_union));
  size_t idx = 0;
  for (header_id_t header_id : header_ids) {
    auto &header = get_header(header_id);
    header.set_union_membership(&header_unions.back(), idx++);
  }
}

void
PHV::push_back_header_union_stack(
    const std::string &header_union_stack_name,
    header_union_stack_id_t header_union_stack_index,
    const std::vector<header_id_t> &header_union_ids) {
  assert(header_union_stack_index < static_cast<int>(capacity_union_stacks));
  assert(
      header_union_stack_index == static_cast<int>(header_union_stacks.size()));
  HeaderUnionStack header_union_stack(
      header_union_stack_name, header_union_stack_index);
  for (header_id_t header_union_id : header_union_ids) {
    header_union_stack.set_next_element(get_header_union(header_union_id));
  }
  header_union_stacks.push_back(std::move(header_union_stack));
}

void
PHV::add_field_alias(const std::string &from, const std::string &to) {
  // if an alias has the same name as an actual field, we give priority to the
  // alias definition; the future will tell use if this is the right choice...
  auto &ref = get_field(to);
  auto r = fields_map.emplace(from, ref);
  if (!r.second) r.first->second = ref;
  // fields_map.emplace(from, ref);
}

const std::string
PHV::get_field_name(header_id_t header_index, int field_offset) const {
  return get_header(header_index).get_field_full_name(field_offset);
}


void
PHVFactory::push_back_header(const std::string &header_name,
                             const header_id_t header_index,
                             const HeaderType &header_type,
                             const bool metadata) {
  HeaderDesc desc(header_name, header_index, header_type, metadata);
  // cannot use operator[] because it requires default constructibility
  header_descs.insert(std::make_pair(header_index, desc));
  for (int i = 0; i < header_type.get_num_fields(); i++) {
    field_names.insert(header_name + "." + header_type.get_field_name(i));
  }
}

void
PHVFactory::push_back_header_stack(const std::string &header_stack_name,
                                   const header_stack_id_t header_stack_index,
                                   const HeaderType &header_type,
                                   const std::vector<header_id_t> &headers) {
  HeaderStackDesc desc(
      header_stack_name, header_stack_index, header_type, headers);
  // cannot use operator[] because it requires default constructibility
  header_stack_descs.insert(std::make_pair(header_stack_index, desc));
}

void
PHVFactory::push_back_header_union(const std::string &header_union_name,
                                   const header_union_id_t header_union_index,
                                   const std::vector<header_id_t> &headers) {
  HeaderUnionDesc desc(header_union_name, header_union_index, headers);
  // cannot use operator[] because it requires default constructibility
  header_union_descs.insert(std::make_pair(header_union_index, desc));
}

void
PHVFactory::push_back_header_union_stack(
    const std::string &header_union_stack_name,
    const header_union_stack_id_t header_union_stack_index,
    const std::vector<header_union_id_t> &header_unions) {
  HeaderUnionStackDesc desc(
      header_union_stack_name, header_union_stack_index, header_unions);
  // cannot use operator[] because it requires default constructibility
  header_union_stack_descs.insert(
      std::make_pair(header_union_stack_index, desc));
}

void
PHVFactory::add_field_alias(const std::string &from, const std::string &to) {
  auto it = field_names.find(from);
  if (it != field_names.end()) {
    Logger::get()->info("Field alias {} has the same name as an actual field; "
                        "we will give priority to the alias", *it);
  }
  field_aliases[from] = to;
}

void
PHVFactory::enable_field_arith(header_id_t header_id, int field_offset) {
  auto &desc = header_descs.at(header_id);
  desc.arith_offsets.insert(field_offset);
}

void
PHVFactory::enable_all_field_arith(header_id_t header_id) {
  auto &desc = header_descs.at(header_id);
  for (int offset = 0; offset < desc.header_type.get_num_fields(); offset++) {
    desc.arith_offsets.insert(offset);
  }
}

void
PHVFactory::disable_field_arith(header_id_t header_id, int field_offset) {
  auto &desc = header_descs.at(header_id);
  desc.arith_offsets.erase(field_offset);
}

void
PHVFactory::disable_all_field_arith(header_id_t header_id) {
  auto &desc = header_descs.at(header_id);
  desc.arith_offsets.clear();
}

void
PHVFactory::enable_stack_field_arith(header_stack_id_t header_stack_id,
                                     int field_offset) {
  const auto &desc = header_stack_descs.at(header_stack_id);
  for (header_id_t header_id : desc.headers)
    enable_field_arith(header_id, field_offset);
}

void
PHVFactory::enable_all_stack_field_arith(header_stack_id_t header_stack_id) {
  const auto &desc = header_stack_descs.at(header_stack_id);
  for (header_id_t header_id : desc.headers)
    enable_all_field_arith(header_id);
}

void
PHVFactory::enable_union_stack_field_arith(
    header_union_stack_id_t header_union_stack_id, size_t header_offset,
    int field_offset) {
  const auto &stack_desc = header_union_stack_descs.at(header_union_stack_id);
  for (header_union_id_t union_id : stack_desc.header_unions) {
    const auto &union_desc = header_union_descs.at(union_id);
    enable_field_arith(union_desc.headers.at(header_offset), field_offset);
  }
}

void
PHVFactory::enable_all_union_stack_field_arith(
    header_union_stack_id_t header_union_stack_id, size_t header_offset) {
  const auto &stack_desc = header_union_stack_descs.at(header_union_stack_id);
  for (header_union_id_t union_id : stack_desc.header_unions) {
    const auto &union_desc = header_union_descs.at(union_id);
    enable_all_field_arith(union_desc.headers.at(header_offset));
  }
}

void
PHVFactory::enable_all_union_stack_field_arith(
    header_union_stack_id_t header_union_stack_id) {
  const auto &stack_desc = header_union_stack_descs.at(header_union_stack_id);
  for (header_union_id_t union_id : stack_desc.header_unions) {
    const auto &union_desc = header_union_descs.at(union_id);
    for (header_id_t header_id : union_desc.headers)
      enable_all_field_arith(header_id);
  }
}

void
PHVFactory::enable_all_arith() {
  for (auto it : header_descs)
    enable_all_field_arith(it.first);
}

std::unique_ptr<PHV>
PHVFactory::create() const {
  std::unique_ptr<PHV> phv(new PHV(
      header_descs.size(), header_stack_descs.size(),
      header_union_descs.size(), header_union_stack_descs.size()));

  for (const auto &e : header_descs) {
    const auto &desc = e.second;
    phv->push_back_header(desc.name, desc.index,
                          desc.header_type, desc.arith_offsets,
                          desc.metadata);
  }

  for (const auto &e : header_stack_descs) {
    const auto &desc = e.second;
    phv->push_back_header_stack(desc.name, desc.index,
                                desc.header_type, desc.headers);
  }

  for (const auto &e : header_union_descs) {
    const auto &desc = e.second;
    phv->push_back_header_union(desc.name, desc.index, desc.headers);
  }

  for (const auto &e : header_union_stack_descs) {
    const auto &desc = e.second;
    phv->push_back_header_union_stack(desc.name, desc.index,
                                      desc.header_unions);
  }

  for (const auto &e : field_aliases)
    phv->add_field_alias(e.first, e.second);

  return phv;
}

}  // namespace bm
