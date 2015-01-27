#ifndef _BM_DATA_H_
#define _BM_DATA_H_

#include <iostream>
#include <cstring>

#include <gmp.h>

#include "data.h"

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

  void set(unsigned int i) {
    mpz_set_ui(value, i);
  }

  void set(char *bytes, int nbytes) {
    mpz_import(value, 1, 1, nbytes, 1, 0, bytes);
  }

  unsigned int get_ui() const {
    return mpz_get_ui(value);
  }

  void sync_value() {
  }

  template<typename U, typename V> void add(U &src1, V &src2) {
    src1.sync_value();
    src2.sync_value();
    mpz_add(value, src1.value, src2.value);
  }

  friend bool operator==(const Data &lhs, const Data &rhs) {
    return (lhs.value == rhs.value);
  }

  friend bool operator!=(const Data &lhs, const Data &rhs) {
    return !(lhs == rhs);
  }

  friend std::ostream& operator<<( std::ostream &out, Data &d ) {
    out << d.value;
    return out;
  }
};

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

class Register : public Data
{

};

#endif
