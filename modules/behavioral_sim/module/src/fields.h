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

  void add(const Data &src1, const Data &src2) {
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
};

#endif
