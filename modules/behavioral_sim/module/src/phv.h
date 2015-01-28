#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <cassert>

#include "fields.h"

class Header
{
  Field **fields;
  int num_fields;
  int field_offset;

public:
  Header(int num_fields)
    : num_fields(num_fields) {
    field_offset = 0;
    fields = new Field *[num_fields];
  }

  ~Header() {
    delete[] fields;
  }

  void add_field(Field *field) {
    fields[field_offset++] = field;
  }
};

class PHV
{
private:
  int num_fields;
  int num_headers;

  Field **fields;
  Header **headers;

public:
  PHV(int num_fields, int num_headers)
    : num_fields(num_fields), num_headers(num_headers) {
    fields = new Field *[num_fields];
    headers = new Header *[num_headers];
  }

  ~PHV() {
    delete[] headers;
    delete[] fields;
  }

  void add_field(int field_index, Field *field) {
    fields[field_index] = field;
  }

  void add_header(int header_index, Header *header) {
    headers[header_index] = header;
  }

  Field *get_field(int field_index) const {
    return fields[field_index];
  }

  Header *get_header(int header_index) const {
    return headers[header_index];
  }

};


#endif
