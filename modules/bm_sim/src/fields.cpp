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

#include <algorithm>

#include "bm_sim/fields.h"
#include "extract.h"

int Field::extract(const char *data, int hdr_offset) {
  generic_extract(data, hdr_offset, nbits, bytes.data());

  if(arith) sync_value();

  return nbits;
}

int Field::extract_VL(const char *data, int hdr_offset, int computed_nbits) {
  nbits = computed_nbits;
  nbytes = (nbits + 7) / 8;
  mask = (1 << nbits); mask -= 1;
  bytes.resize(nbytes);
  return Field::extract(data, hdr_offset);
}

int Field::deparse(char *data, int hdr_offset) const {
  if(hdr_offset == 0 && nbits % 8 == 0) {
    std::copy(bytes.begin(), bytes.end(), data);
    return nbits;
  }

  int field_offset = (nbytes << 3) - nbits;
  int hdr_bytes = (hdr_offset + nbits + 7) / 8;

  int i;

  // necessary to ensure correct behavior when shifting right (no sign extension)
  unsigned char *ubytes = (unsigned char *) bytes.data();
  
  // zero out bits we are going to write in data[0]
  data[0] &= (~(0xFF >> hdr_offset));

  int offset = field_offset - hdr_offset;
  if (offset == 0) {
    std::copy(bytes.begin() + 1, bytes.begin() + hdr_bytes, data + 1);
    data[0] |= ubytes[0];
  }
  else if (offset > 0) { /* shift left */
    /* this assumes that the packet was memset to 0, TODO: improve */
    for(i = 0; i < hdr_bytes - 1; i++) {
      data[i] = (ubytes[i] << offset) | (ubytes[i + 1] >> (8 - offset));
    }
    if(i > 0) data[i] = 0;
    data[i] |= ubytes[i] << offset;
  }
  else { /* shift right */
    offset = -offset;
    data[0] |= (ubytes[0] >> offset);
    if(nbytes == 1) {
      /* data[1] is always valid, otherwise we would not need to shift the field
	 to the right */
      data[1] = ubytes[0] << (8 - offset);
      return nbits;
    }
    for(i = 1; i < hdr_bytes - 1; i++) {
      data[i] = (ubytes[i - 1] << (8 - offset)) | (ubytes[i] >> offset);
    }
    int tail_offset = (hdr_bytes << 3) - (hdr_offset + nbits);
    data[i] &= ((1 << tail_offset) - 1);
    data[i] |= (ubytes[i - 1] << (8 - offset));
  }

  return nbits;
}
