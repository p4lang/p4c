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

namespace bm {

namespace extract {

static void generic_extract(const char *data, int bit_offset,
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
  unsigned char *udata = (unsigned char *) data;

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

}  // namespace extract

}  // namespace bm

#endif  // BM_SIM_EXTRACT_H_
