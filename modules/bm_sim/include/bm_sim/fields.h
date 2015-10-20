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

#ifndef _BM_FIELDS_H_
#define _BM_FIELDS_H_

#include <algorithm>

#include <cassert>

#include "data.h"
#include "bytecontainer.h"
#include "bignum.h"

class Field : public Data
{
public:
  // Data() is called automatically
  Field(int nbits, bool arith_flag = true)
    : nbits(nbits), nbytes( (nbits + 7) / 8 ), bytes(nbytes) {
    arith = arith_flag;
    // TODO ?
    // should I only do that for arith fields ?
    mask <<= nbits; mask -= 1;
  }

  // Overload set? Make it more generic (arbitary length) ?
  // It is probably only going to be used by the checksum engine anyway...
  void set_bytes(const char *src_bytes, int len) {
    assert(len == nbytes);
    std::copy(src_bytes, src_bytes + len, bytes.begin());
    if(arith) sync_value();
  }
  
  void sync_value() {
    bignum::import_bytes(value, bytes.data(), nbytes);
  }

  const ByteContainer &get_bytes() const {
    return bytes;
  }

  int get_nbytes() const {
    return nbytes;
  }

  int get_nbits() const {
    return nbits;
  }

  void set_arith(bool arith_flag) { arith = arith_flag; }

  void export_bytes() {
    std::fill(bytes.begin(), bytes.end(), 0); // very important !
    // TODO: this can overflow !!!
    // maybe bytes is not large enough !!!
    // I am supposed to mask off extra bits...
    // is this efficient enough:
    value &= mask;
    bignum::export_bytes(bytes.data(), nbytes, value);
  }

  /* returns the number of bits extracted */
  int extract(const char *data, int hdr_offset);

  int extract_VL(const char *data, int hdr_offset, int computed_nbits);

  /* returns the number of bits deparsed */
  int deparse(char *data, int hdr_offset) const;

private:
  int nbits;
  int nbytes;
  ByteContainer bytes;
  Bignum mask{1};
};

#endif
