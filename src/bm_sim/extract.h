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

#ifndef BM_SIM_EXTRACT_H_
#define BM_SIM_EXTRACT_H_

#include <algorithm>  // for std::fill, std::copy

namespace bm {

namespace extract {

static inline void generic_extract(const char *data, int bit_offset,
                                   int bitwidth, char *dst) {
  int nbytes = (bitwidth + 7) / 8;

  if (bit_offset == 0 && bitwidth % 8 == 0) {
    memcpy(dst, data, nbytes);
    return;
  }

  int dst_offset = (nbytes << 3) - bitwidth;
  int i;

  // necessary to ensure correct behavior when shifting right (no sign
  // extension)
  auto udata = reinterpret_cast<const unsigned char *>(data);

  int offset = bit_offset - dst_offset;
  if (offset == 0) {
    memcpy(dst, udata, nbytes);
    dst[0] &= (0xFF >> dst_offset);
  } else if (offset > 0) {  // shift left
    for (i = 0; i < nbytes - 1; i++) {
      dst[i] = (udata[i] << offset) | (udata[i + 1] >> (8 - offset));
    }
    dst[0] &= (0xFF >> dst_offset);
    dst[i] = udata[i] << offset;
    if ((bit_offset + bitwidth) > (nbytes << 3)) {
      dst[i] |= (udata[i + 1] >> (8 - offset));
    }
  } else {  // shift right
    offset = -offset;
    dst[0] = udata[0] >> offset;
    dst[0] &= (0xFF >> dst_offset);
    for (i = 1; i < nbytes; i++) {
      dst[i] = (udata[i - 1] << (8 - offset)) | (udata[i] >> offset);
    }
  }
}

static inline void generic_deparse(const char *data, int bitwidth,
                                   char *dst, int hdr_offset) {
  int nbytes = (bitwidth + 7) / 8;

  if (hdr_offset == 0 && bitwidth % 8 == 0) {
    memcpy(dst, data, nbytes);
    return;
  }

  int field_offset = (nbytes << 3) - bitwidth;
  int hdr_bytes = (hdr_offset + bitwidth + 7) / 8;

  int i;

  // necessary to ensure correct behavior when shifting right (no sign
  // extension)
  auto udata = reinterpret_cast<const unsigned char *>(data);

  // zero out bits we are going to write in dst[0]
  dst[0] &= (~(0xFF >> hdr_offset));

  int offset = field_offset - hdr_offset;
  if (offset == 0) {
    std::copy(data + 1, data + hdr_bytes, dst + 1);
    dst[0] |= udata[0];
  } else if (offset > 0) {  // shift left
    // don't know if this is very efficient, we memset the remaining bytes to 0
    // so we can use |= and preserve what was originally in dst[0]
    std::fill(&dst[1], &dst[hdr_bytes], 0);
    for (i = 0; i < hdr_bytes - 1; i++) {
      dst[i] |= (udata[i] << offset) | (udata[i + 1] >> (8 - offset));
    }
    dst[i] |= udata[i] << offset;
  } else {  // shift right
    offset = -offset;
    dst[0] |= (udata[0] >> offset);
    if (nbytes == 1) {
      // dst[1] is always valid, otherwise we would not need to shift the field
      // to the right
      dst[1] = udata[0] << (8 - offset);
      return;
    }
    for (i = 1; i < hdr_bytes - 1; i++) {
      dst[i] = (udata[i - 1] << (8 - offset)) | (udata[i] >> offset);
    }
    int tail_offset = (hdr_bytes << 3) - (hdr_offset + bitwidth);
    dst[i] &= ((1 << tail_offset) - 1);
    dst[i] |= (udata[i - 1] << (8 - offset));
  }
}

}  // namespace extract

}  // namespace bm

#endif  // BM_SIM_EXTRACT_H_
