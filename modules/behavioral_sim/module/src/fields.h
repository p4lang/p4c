#ifndef _BM_FIELDS_H_
#define _BM_FIELDS_H_

#include <algorithm>

#include "data.h"

#define BYTE_ROUND_UP(x) ((x + 7) >> 3)

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

  int extract(const char *data, int hdr_offset) {
    if(hdr_offset == 0 && nbits % 8 == 0) {
      memcpy(bytes, data, nbytes);
    }

    int field_offset = (nbytes << 3) - nbits;
    int i;

    int offset = hdr_offset - field_offset;
    if (offset == 0) {
      memcpy(bytes, data, nbytes);
      bytes[0] &= (0xFF >> field_offset);
    }
    else if (offset > 0) { /* shift left */
      for (i = 0; i < nbytes - 1; i++) {
	bytes[i] = (data[i] << offset) | (data[i + 1] >> (8 - offset));
      }
      bytes[i] = data[i] << offset;
      if((offset + nbits) > (nbytes << 3)) {
	bytes[i] |= (data[i + 1] >> (8 - offset));
      }
    }
    else { /* shift right */
      offset = -offset;
      bytes[0] = data[0] >> offset;
      for (i = 1; i < nbytes; i++) {
	bytes[i] = (data[i - 1] << (8 - offset)) | (data[i] >> offset);
      }
      /* Am I forgetting something ? */
    }

    return nbits;
  }

};

#endif
