#ifndef _BM_DATA_H_
#define _BM_DATA_H_

#include <iostream>
#include <cstring>

#include <cstddef> // libgmp5 is buggy withh gcc4.9, this fixes it
#include <gmp.h>

class Data
{
protected:
  mpz_t value;
  
public:
  Data() {
    mpz_init(value);
  }

  Data(unsigned int i) {
    mpz_init(value);
    mpz_set_ui(value, i);
  }

  Data (const Data &other) {
    mpz_init_set(value, other.value);
  }

  ~Data() {
    mpz_clear(value);
  }

  void set(unsigned int i) {
    mpz_set_ui(value, i);
  }

  void set(char *bytes, int nbytes) {
    mpz_import(value, 1, 1, nbytes, 1, 0, bytes);
  }

  unsigned int get_ui() const {
    return mpz_get_ui(value);
  }

  virtual void sync_value() {}

  /* we do not overload the + operator to avoid temporaries */
  void add(Data &src1, Data &src2) {
    src1.sync_value();
    src2.sync_value();
    mpz_add(value, src1.value, src2.value);
  }

  Data& operator=(const Data &other) {
    if(&other == this)
      return *this;
    mpz_set(value, other.value);
    return *this;
  }

  friend bool operator==(const Data &lhs, const Data &rhs) {
    return !mpz_cmp(lhs.value, rhs.value);
  }

  friend bool operator!=(const Data &lhs, const Data &rhs) {
    return !(lhs == rhs);
  }

  friend std::ostream& operator<<( std::ostream &out, Data &d ) {
    out << d.value;
    return out;
  }
};

#endif
