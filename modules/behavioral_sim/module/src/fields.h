#ifndef _BM_FIELDS_H_
#define _BM_FIELDS_H_

#include "data.h"

class Field : public Data
{
private:
  char *bytes;
  int nbytes;
  bool value_sync;

public:
  Field() {
    value_sync = false;
  }

  Field(char *b, int n) {
    bytes = b;
    nbytes = n;
    value_sync = false;
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

};

#endif
