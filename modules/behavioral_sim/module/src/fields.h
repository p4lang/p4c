#ifndef _BM_FIELDS_H_
#define _BM_FIELDS_H_

#include <algorithm>

#include <cassert>

#include "data.h"

class Field : public Data
{
private:
  char *bytes;
  int nbytes;
  int nbits;
  bool value_sync;

public:
  Field(int nbits)
    : nbits(nbits) {
    nbytes = (nbits + 7) / 8;
    bytes = new char[nbytes];
    value_sync = false;
  }

  ~Field() {
    delete[] bytes;
  }

  Field(const Field &other)
    : Data(other) {
    nbytes = other.nbytes;
    nbits = other.nbits;
    value_sync = other.value_sync;
    bytes = new char[nbytes];
    memcpy(bytes, other.bytes, nbytes);
  }

  unsigned int get_ui() {
    sync_value();
    return mpz_get_ui(value);
  }

  void sync_value() {
    if(value_sync) return;
    mpz_import(value, 1, 1, nbytes, 1, 0, bytes);
    value_sync = true;
  }

  const char *get_bytes() const {
    return bytes;
  }

  int get_nbytes() const {
    return nbytes;
  }

  int get_nbits() const {
    return nbits;
  }

  void add(Data &src1, Data &src2) {
    Data::add(src1, src2);
    size_t count;
    mpz_export(bytes, &count, 1, nbytes, 1, 0, value);
  }
  

  Field& operator=(const Field &other) {
    assert(NULL);
    // std::cout << "PPPP\n";
    // if(&other == this)
    //   return *this;
    // memset(bytes, 0, nbytes);
    // memcpy(bytes, other.bytes, std::min(nbytes, other.nbytes));
    // value_sync = false;
    return *this;
  }

  /* Field& operator=(const Data &other) { */
  /*   // TODO */
  /*   return *this; */
  /* } */

  friend std::ostream& operator<<( std::ostream &out, Field &f ) {
    f.sync_value();
    out << f.value;
    return out;
  }

  /* returns the number of bits extracted */
  int extract(const char *data, int hdr_offset);

  /* returns the number of bits deparsed */
  int deparse(char *data, int hdr_offset) const;

};

#endif
