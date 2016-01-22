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

//! @file fields.h

#ifndef BM_SIM_INCLUDE_BM_SIM_FIELDS_H_
#define BM_SIM_INCLUDE_BM_SIM_FIELDS_H_

#include <algorithm>

#include <cassert>

#include "data.h"
#include "bytecontainer.h"
#include "bignum.h"
#include "debugger.h"

namespace bm {

//! Field objects are used to represent P4 fields. Each Field instance belongs
//! to a Header instance. When defining your own target, you will have to
//! manipulate Field objects to set and access the target's intrinsic metadata.
//! Most of the interesting methods for a target designer are inherited from
//! Data.
class Field : public Data {
 public:
  // Data() is called automatically
  explicit Field(int nbits, bool arith_flag = true)
    : nbits(nbits), nbytes((nbits + 7) / 8), bytes(nbytes) {
    arith = arith_flag;
    // TODO(antonin) ?
    // should I only do that for arith fields ?
    mask <<= nbits; mask -= 1;
  }

  // Overload set? Make it more generic (arbitary length) ?
  // It is probably only going to be used by the checksum engine anyway...
  void set_bytes(const char *src_bytes, int len) {
    assert(len == nbytes);
    std::copy(src_bytes, src_bytes + len, bytes.begin());
    if (arith) sync_value();
  }

  void sync_value() {
    bignum::import_bytes(&value, bytes.data(), nbytes);
    DEBUGGER_NOTIFY_UPDATE(*packet_id, my_id, bytes.data(), nbits);
  }

  //! Return the byte representation of this field. Note that this returns a
  //! reference to a byte container, which is only valid as long as the Field
  //! instance is alive.
  const ByteContainer &get_bytes() const {
    return bytes;
  }

  //! Get the number of bytes occupied by this field, not based on its layout in
  //! the packet header, but assuming a byte boundary alignment for the most
  //! significant bit. `nbytes = (nbits + 7) / 8`
  int get_nbytes() const {
    return nbytes;
  }

  //! Get the number of bits occupied by this field in the packet header
  int get_nbits() const {
    return nbits;
  }

  void set_arith(bool arith_flag) { arith = arith_flag; }

  void export_bytes() {
    std::fill(bytes.begin(), bytes.end(), 0);  // very important !
    // TODO(antonin): this can overflow !!!
    // maybe bytes is not large enough !!!
    // I am supposed to mask off extra bits...
    // is this efficient enough:
    value &= mask;
    bignum::export_bytes(bytes.data(), nbytes, value);
    DEBUGGER_NOTIFY_UPDATE(*packet_id, my_id, bytes.data(), nbits);
  }

  // useful for header stacks
  void swap_values(Field *other) {
    // just the values, nothing else (especially not .arith)
    std::swap(value, other->value);
    std::swap(bytes, other->bytes);
  }

  /* returns the number of bits extracted */
  int extract(const char *data, int hdr_offset);

  int extract_VL(const char *data, int hdr_offset, int computed_nbits);

  /* returns the number of bits deparsed */
  int deparse(char *data, int hdr_offset) const;

  void set_id(uint64_t id) { my_id = id; }
  void set_packet_id(const Debugger::PacketId *id) { packet_id = id; }

 private:
  int nbits;
  int nbytes;
  ByteContainer bytes;
  Bignum mask{1};
  uint64_t my_id{};
  const Debugger::PacketId *packet_id{&Debugger::dummy_PacketId};
};

}  // namespace bm

#endif  // BM_SIM_INCLUDE_BM_SIM_FIELDS_H_
