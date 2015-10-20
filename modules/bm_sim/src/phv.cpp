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

#include "bm_sim/phv.h"

void PHV::reset() {
  for(auto &h : headers)
    h.mark_invalid();
}

void PHV::reset_header_stacks() {
  for(auto &hs : header_stacks)
    hs.reset();
}

// so slow I want to die, but optional for a target...
// I need to find a better way of doing this
void PHV::reset_metadata() {
  for(auto &h : headers) {
    if(h.is_metadata())
      h.reset();
  }
}

void PHV::push_back_header(
  const std::string &header_name,
  header_id_t header_index,
  const HeaderType &header_type,
  const std::set<int> &arith_offsets,
  const bool metadata
) {
  assert(header_index < (int) capacity);
  assert(header_index == (int) headers.size());
  headers.push_back(
    Header(header_name, header_index, header_type, arith_offsets, metadata)
  );

  headers_map.emplace(header_name, get_header(header_index));

  for(int i = 0; i < header_type.get_num_fields(); i++) {
    const std::string name = header_name + "." + header_type.get_field_name(i);
    // std::cout << header_index << " " << i << " " << name << std::endl;
    fields_map.emplace(name, get_field(header_index, i));
  }

  if(header_type.is_VL_header()) {
    headers.back().VL_expr = header_type.resolve_VL_expr(header_index);
  }
}

void PHV::push_back_header_stack(
  const std::string &header_stack_name,
  header_stack_id_t header_stack_index,
  const HeaderType &header_type,
  const std::vector<header_id_t> &header_ids
) {
  assert(header_stack_index < (int) capacity_stacks);
  assert(header_stack_index == (int) header_stacks.size());
  HeaderStack header_stack(header_stack_name, header_stack_index, header_type);
  for(header_id_t header_id : header_ids) {
    header_stack.set_next_header(get_header(header_id));
  }
  header_stacks.push_back(std::move(header_stack));
}
