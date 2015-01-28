#ifndef _BM_FIELDS_H_
#define _BM_FIELDS_H_

#include <algorithm>

#include "data.h"

class Field : public Data
{
private:
  char *bytes;
  int nbytes;
  int nbits;
  bool value_sync;
  char first_byte_mask;

public:
  Field() {
    bytes = NULL;
  }

  Field(int nbits)
    : nbits(nbits) {
    nbytes = (nbits + 7) / 8;
    bytes = new char[nbytes];
    value_sync = false;
    first_byte_mask = 0xFF >> (nbytes * 8 - nbits);
  }

  ~Field() {
    if(bytes) delete[] bytes;
  }

  void sync_value() {
    if(value_sync) return;
    mpz_import(value, 1, 1, nbytes, 1, 0, bytes);
    value_sync = true;
  }

  char *get_bytes() {
    return bytes;
  }

  template<typename U, typename V> void add(U &src1, V &src2) {
    Data::add(src1, src2);
    size_t count;
    mpz_export(bytes, &count, 1, nbytes, 1, 0, value);
  }

  Field& operator=(const Field &other) {
    if(&other == this)
      return *this;
    memset(bytes, 0, nbytes);
    memcpy(bytes, other.bytes, std::min(nbytes, other.nbytes));
    value_sync = false;
    return *this;
  }

  Field& operator=(const Data &other) {
    /* TODO */
    return *this;
  }

};

#endif
