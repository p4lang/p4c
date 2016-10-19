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

#include <bm/bm_sim/fields.h>

#include "extract.h"

namespace bm {

int Field::extract(const char *data, int hdr_offset) {
  extract::generic_extract(data, hdr_offset, nbits, bytes.data());

  if (arith) sync_value();

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
  extract::generic_deparse(&bytes[0], nbits, data, hdr_offset);
  return nbits;
}

}  // namespace bm
