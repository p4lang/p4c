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
  Field(int nbits)
    : nbits(nbits), nbytes( (nbits + 7) / 8 ),
    bytes(nbytes), value_sync(false) {}

  void sync_value() {
    if(value_sync) return;
    bignum::import_bytes(value, bytes.data(), nbytes);
    value_sync = true;
  }

  unsigned int get_ui() const = delete;

  unsigned int get_ui() {
    sync_value();
    // Bad ?
    return (unsigned) value;
  }

  /* TODO: change */
  const char *get_bytes() const {
    return bytes.data();
  }

  int get_nbytes() const {
    return nbytes;
  }

  int get_nbits() const {
    return nbits;
  }

  void add(Data &src1, Data &src2) {
    Data::add(src1, src2);
    bignum::export_bytes(bytes.data(), value);
  }

  /* returns the number of bits extracted */
  int extract(const char *data, int hdr_offset);

  /* returns the number of bits deparsed */
  int deparse(char *data, int hdr_offset) const;

private:
  int nbits;
  int nbytes;
  ByteContainer bytes;
  bool value_sync;
};

#endif
