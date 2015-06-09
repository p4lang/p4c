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

int Field::extract(const char *data, int hdr_offset) {
  if(hdr_offset == 0 && nbits % 8 == 0) {
    std::copy(data, data + nbytes, bytes.begin());
    if(arith) sync_value();
    return nbits;
  }

  int field_offset = (nbytes << 3) - nbits;
  int i;

  // necessary to ensure correct behavior when shifting right (no sign extension)
  unsigned char *udata = (unsigned char *) data;
  
  int offset = hdr_offset - field_offset;
  if (offset == 0) {
    std::copy(udata, udata + nbytes, bytes.begin());
    bytes[0] &= (0xFF >> field_offset);
  }
  else if (offset > 0) { /* shift left */
    for (i = 0; i < nbytes - 1; i++) {
      bytes[i] = (udata[i] << offset) | (udata[i + 1] >> (8 - offset));
    }
    bytes[0] &= (0xFF >> field_offset);
    bytes[i] = udata[i] << offset;
    if((hdr_offset + nbits) > (nbytes << 3)) {
      bytes[i] |= (udata[i + 1] >> (8 - offset));
    }
  }
  else { /* shift right */
    offset = -offset;
    bytes[0] = udata[0] >> offset;
    for (i = 1; i < nbytes; i++) {
      bytes[i] = (udata[i - 1] << (8 - offset)) | (udata[i] >> offset);
    }
  }

  if(arith) sync_value();

  return nbits;
}

int Field::deparse(char *data, int hdr_offset) const {
  if(hdr_offset == 0 && nbits % 8 == 0) {
    std::copy(bytes.begin(), bytes.end(), data);
    return nbits;
  }

  int field_offset = (nbytes << 3) - nbits;
  int hdr_bytes = (hdr_offset + nbits + 7) / 8;

  int i;
  
  // zero out bits we are going to write in data[0]
  data[0] &= (~(0xFF >> hdr_offset));

  int offset = field_offset - hdr_offset;
  if (offset == 0) {
    std::copy(bytes.begin() + 1, bytes.begin() + hdr_bytes, data + 1);
    data[0] |= bytes[0];
  }
  else if (offset > 0) { /* shift left */
    /* this assumes that the packet was memset to 0, TODO: improve */
    for (i = 0; i < hdr_bytes - 1; i++) {
      data[i] |= (bytes[i] << offset) | (bytes[i + 1] >> (8 - offset));
    }
    int tail_offset = (hdr_bytes << 3) - (hdr_offset + nbits);
    data[i] &= ((1 << tail_offset) - 1);
    data[i] |= bytes[i] << offset;
    if((field_offset + nbits) > (hdr_bytes << 3)) {
      data[i] |= (bytes[i + 1] >> (8 - offset));
    }
  }
  else { /* shift right */
    offset = -offset;
    data[0] |= (bytes[0] >> offset);
    for (i = 1; i < hdr_bytes - 1; i++) {
      data[i] = (bytes[i - 1] << (8 - offset)) | (bytes[i] >> offset);
    }
    int tail_offset = (hdr_bytes >> 3) - (hdr_offset + nbits);
    data[i] &= ((1 << tail_offset) - 1);
    data[i] |= (bytes[i - 1] << (8 - offset)) | (bytes[i] >> offset);
  }

  return nbits;
}
