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

void
PHV::reset() {
  for (auto &h : headers)
    h.mark_invalid();
}

void
PHV::reset_header_stacks() {
  for (auto &hs : header_stacks)
    hs.reset();
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
  for (auto &h : headers) {
    h.reset();
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
  headers.push_back(
    Header(header_name, header_index, header_type, arith_offsets, metadata));
  headers.back().set_packet_id(&packet_id);

  headers_map.emplace(header_name, get_header(header_index));

  for (int i = 0; i < header_type.get_num_fields(); i++) {
    const std::string name = header_name + "." + header_type.get_field_name(i);
    // std::cout << header_index << " " << i << " " << name << std::endl;
    fields_map.emplace(name, get_field(header_index, i));
  }

  if (header_type.is_VL_header()) {
    headers.back().VL_expr = header_type.resolve_VL_expr(header_index);

    for (const int offset : header_type.get_VL_input_offsets())
      headers.back()[offset].set_arith(true);
  }
}

void
PHV::push_back_header_stack(const std::string &header_stack_name,
                            header_stack_id_t header_stack_index,
                            const HeaderType &header_type,
                            const std::vector<header_id_t> &header_ids) {
  assert(header_stack_index < static_cast<int>(capacity_stacks));
  assert(header_stack_index == static_cast<int>(header_stacks.size()));
  HeaderStack header_stack(header_stack_name, header_stack_index, header_type);
  for (header_id_t header_id : header_ids) {
    header_stack.set_next_header(get_header(header_id));
  }
  header_stacks.push_back(std::move(header_stack));
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


void
PHVFactory::push_back_header(const std::string &header_name,
                             const header_id_t header_index,
                             const HeaderType &header_type,
                             const bool metadata) {
  HeaderDesc desc = HeaderDesc(header_name, header_index,
                               header_type, metadata);
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
  HeaderStackDesc desc = HeaderStackDesc(
      header_stack_name, header_stack_index, header_type, headers);
  // cannot use operator[] because it requires default constructibility
  header_stack_descs.insert(std::make_pair(header_stack_index, desc));
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
  HeaderDesc &desc = header_descs.at(header_id);
  desc.arith_offsets.insert(field_offset);
}

void
PHVFactory::enable_all_field_arith(header_id_t header_id) {
  HeaderDesc &desc = header_descs.at(header_id);
  for (int offset = 0; offset < desc.header_type.get_num_fields(); offset++) {
    desc.arith_offsets.insert(offset);
  }
}

void
PHVFactory::disable_field_arith(header_id_t header_id, int field_offset) {
  HeaderDesc &desc = header_descs.at(header_id);
  desc.arith_offsets.erase(field_offset);
}

void
PHVFactory::disable_all_field_arith(header_id_t header_id) {
  HeaderDesc &desc = header_descs.at(header_id);
  desc.arith_offsets.clear();
}

void
PHVFactory::enable_all_arith() {
  for (auto it : header_descs)
    enable_all_field_arith(it.first);
}

std::unique_ptr<PHV>
PHVFactory::create() const {
  std::unique_ptr<PHV> phv(new PHV(header_descs.size(),
                                   header_stack_descs.size()));

  for (const auto &e : header_descs) {
    const HeaderDesc &desc = e.second;
    phv->push_back_header(desc.name, desc.index,
                          desc.header_type, desc.arith_offsets,
                          desc.metadata);
  }

  for (const auto &e : header_stack_descs) {
    const HeaderStackDesc &desc = e.second;
    phv->push_back_header_stack(desc.name, desc.index,
                                desc.header_type, desc.headers);
  }

  for (const auto &e : field_aliases)
    phv->add_field_alias(e.first, e.second);

  return phv;
}

}  // namespace bm
