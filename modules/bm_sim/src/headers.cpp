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

#include "bm_sim/headers.h"

Header::Header(const std::string &name, p4object_id_t id,
	       const HeaderType &header_type,
	       const std::set<int> &arith_offsets,
	       const bool metadata)
  : NamedP4Object(name, id), header_type(header_type), metadata(metadata) {
  // header_type_id = header_type.get_type_id();
  for(int i = 0; i < header_type.get_num_fields(); i++) {
    // use emplace_back instead?
    bool arith_flag = true;
    if(arith_offsets.find(i) == arith_offsets.end()) {
      arith_flag = false;
    }
    fields.push_back(Field(header_type.get_bit_width(i), arith_flag));
    nbytes_phv += fields.back().get_nbytes();
    nbytes_packet += fields.back().get_nbits();
  }
  assert(nbytes_packet % 8 == 0);
  nbytes_packet /= 8;
}

void Header::extract(const char *data) {
  int hdr_offset = 0;
  for(std::vector<Field>::iterator it = fields.begin();
      it != fields.end();
      ++it) {
    hdr_offset += it->extract(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
  mark_valid();
  return;
}

void Header::deparse(char *data) const {
  int hdr_offset = 0;
  for(std::vector<Field>::const_iterator it = fields.begin();
      it != fields.end();
      ++it) {
    hdr_offset += it->deparse(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
  return;
}
