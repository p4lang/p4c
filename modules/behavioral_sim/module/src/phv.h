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
  Header() {}

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

typedef int field_id;
typedef int header_id;

class PHV
{
private:
  int num_fields;
  int num_headers;

  Field *fields;
  Header *headers;

public:
  PHV(int num_fields, int num_headers)
    : num_fields(num_fields), num_headers(num_headers) {
    fields = new Field [num_fields];
    headers = new Header [num_headers];
  }

  ~PHV() {
    delete[] headers;
    delete[] fields;
  }

  void init_field(field_id field_index, int nbits) {
    fields[field_index] = Field(nbits);
  }

  void init_header(header_id header_index, int num_fields) {
    headers[header_index] = Header(num_fields);
  }

  void add_field_to_header(header_id header_index, field_id field_index) {
    Header &header = headers[header_index];
    Field *field = &fields[field_index];
    header.add_field(field);
  }

  void primitive_add(field_id dst, field_id src1, field_id src2) {
    Field &field_dst = fields[dst];
    Field &field_src1 = fields[src1];
    Field &field_src2 = fields[src2];

    field_dst.add(field_src1, field_src2);
  }

  void primitive_copy_header(header_id dst, header_id src) {
    Header &dst = headers[dst];
    const Header &src = headers[src];
    
    
  }

  void primitive_modify_field(field_id dst, field_id src) {
    Field &field_dst = fields[dst];
    const Field &field_src = fields[src];
    field_dst = field_src;
  }

  void primitive_modify_field(field_id dst, const Data &src) {
    Field &field_dst = fields[dst];
    field_dst = src;
  }

};


#endif
