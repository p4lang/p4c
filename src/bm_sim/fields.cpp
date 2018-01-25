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
#include <bm/bm_sim/headers.h>

#include <algorithm>  // for std::swap
#include "extract.h"

namespace bm {

Field::Field(int nbits, Header *parent_hdr, bool arith_flag, bool is_signed,
             bool hidden, bool VL, bool is_saturating)
    : nbits(nbits), nbytes((nbits + 7) / 8), bytes(nbytes),
      parent_hdr(parent_hdr),
      is_signed(is_signed), hidden(hidden), VL(VL),
      is_saturating(is_saturating) {
  arith = arith_flag;
  // TODO(antonin) ?
  // should I only do that for arith fields ?
  mask <<= nbits; mask -= 1;
  if (is_signed) {
    assert(nbits > 1);
    max <<= (nbits - 1); max -= 1;
    min <<= (nbits - 1); min *= -1;
  } else {
    max = mask;
    min = 0;
  }
}

void
Field::reserve_VL(size_t max_bytes) {
  if (VL) bytes.reserve(max_bytes);
}

void
Field::swap_values(Field *other) {
  // do not swap arith!
  std::swap(value, other->value);
  std::swap(bytes, other->bytes);
  if (VL) {
    std::swap(nbits, other->nbits);
    std::swap(nbytes, other->nbytes);
    std::swap(mask, other->mask);
    assert(is_signed == other->is_signed);
    assert(is_saturating == other->is_saturating);
    std::swap(max, other->max);
    std::swap(min, other->min);
  }
}

int
Field::extract(const char *data, int hdr_offset) {
  extract::generic_extract(data, hdr_offset, nbits, bytes.data());

  if (arith) sync_value();

  return nbits;
}

int
Field::extract_VL(const char *data, int hdr_offset, int computed_nbits) {
  nbits = computed_nbits;
  nbytes = (nbits + 7) / 8;
  mask = 1; mask <<= nbits; mask -= 1;
  bytes.resize(nbytes);
  if (is_signed) {
    assert(nbits > 1);
    max <<= (nbits - 1); max -= 1;
    min <<= (nbits - 1); min *= -1;
  } else {
    max = mask;
    min = 0;
  }
  return Field::extract(data, hdr_offset);
}

int
Field::deparse(char *data, int hdr_offset) const {
  // this does not work for empty variable-length fields, as we assert in the
  // ByteContainer's [] operator. The right thing to do would probably be to add
  // a at() method to ByteContainer and not perform any check in [].
  // extract::generic_deparse(&bytes[0], nbits, data, hdr_offset);
  extract::generic_deparse(bytes.data(), nbits, data, hdr_offset);
  return nbits;
}

void
Field::assign_VL(const Field &src) {
  assert(VL);
  nbits = src.nbits;
  nbytes = src.nbytes;
  bytes.resize(nbytes);
  mask = src.mask;
  max = src.max;
  min = src.min;
  set(src);
  parent_hdr->recompute_nbytes_packet();
}

void
Field::reset_VL() {
  assert(VL);
  nbits = 0;
  nbytes = 0;
  mask = 1;
  if (is_signed) {
    max = 1;
    min = 1;
  }
}

void
Field::copy_value(const Field &src) {
  // it's important to have a way of copying a field value without the
  // packet_id pointer. This is used by PHV::copy_headers().
  value = src.value;
  bytes = src.bytes;
  if (VL) {
    nbits = src.nbits;
    nbytes = src.nbytes;
    mask = src.mask;
    min = src.min;
    max = src.max;
  }
}

}  // namespace bm
