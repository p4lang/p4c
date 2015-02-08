#ifndef _BM_DATA_H_
#define _BM_DATA_H_

#include <iostream>
#include <cstring>

#include "bignum.h"

using bignum::Bignum;

class Data
{  
public:
  Data() {}

  Data(unsigned i)
    : value(i) {}

  void set(unsigned int i) {
    value = i;
  }

  void set(const char *bytes, int nbytes) {
    bignum::import_bytes(value, bytes, nbytes);
  }

  unsigned int get_ui() const{
    // Bad ?
    return (unsigned) value;
  }

  virtual void sync_value() {}

  void add(Data &src1, Data &src2) {
    src1.sync_value();
    src2.sync_value();
    value = src1.value + src2.value;
  }

  friend bool operator==(const Data &lhs, const Data &rhs) {
    return lhs.value == rhs.value;
  }

  friend bool operator!=(const Data &lhs, const Data &rhs) {
    return !(lhs == rhs);
  }

  friend std::ostream& operator<<( std::ostream &out, Data &d ) {
    d.sync_value();
    out << d.value;
    return out;
  }

protected:
  Bignum value;
};

#endif
