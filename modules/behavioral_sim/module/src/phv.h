#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <vector>

#include <cassert>

#include "fields.h"
#include "headers.h"

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

  void init_header(header_id header_index,
		   header_type_id header_type,
		   std::vector<field_id> &field_ids) {
    std::vector<Field *> field_ptrs;
    for (std::vector<field_id>::iterator it = field_ids.begin();
	 it != field_ids.end();
	 ++it) {
      field_ptrs.push_back(&fields[*it]);
    }
    headers[header_index] = Header(header_type, field_ptrs);
  }

  Field &get_field(field_id field) {
    return fields[field];
  }

  Header &get_header(header_id header) {
    return headers[header];
  }

};


#endif
