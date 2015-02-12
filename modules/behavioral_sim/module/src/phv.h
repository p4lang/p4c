#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <vector>

#include <cassert>

#include "fields.h"
#include "headers.h"

typedef int header_id_t;


class PHV
{
private:
  std::vector<Header> headers;

public:
  header_id_t add_header(const HeaderType &header_type)
  {
    headers.push_back( Header(header_type) );
    return headers.size() - 1;
  }

  Field &get_field(header_id_t header_index, int field_offset) {
    return headers[header_index].get_field(field_offset);
  }

  Header &get_header(header_id_t header_index) {
    return headers[header_index];
  }

  const Field &get_field(header_id_t header_index, int field_offset) const {
    return headers[header_index].get_field(field_offset);
  }

  const Header &get_header(header_id_t header_index) const {
    return headers[header_index];
  }

  void reset(); // mark all headers as invalid

};


#endif
