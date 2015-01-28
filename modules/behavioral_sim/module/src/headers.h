#ifndef _BM_HEADERS_H_
#define _BM_HEADERS_H_

typedef int header_type_id;

struct HeaderType
{
  int num_fields;

  HeaderType(int num_fields)
    : num_fields(num_fields) {}
};

class Header
{
  friend class PHV;

  header_type_id header_type;
  std::vector<Field *> fields;

  Header() {}

  Header(header_type_id header_type, const std::vector<Field *> &fields)
    : header_type(header_type), fields(fields) {
  }

  ~Header() {

  }

  Header& operator=(const Header &other) {
    if(&other == this)
      return *this;
    assert(header_type == other.header_type);
    unsigned field_index;
    for(field_index = 0; field_index < fields.size(); field_index++) {
      *(fields[field_index]) = *(other.fields[field_index]);
    }
    return *this;
  }
};

#endif
