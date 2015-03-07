#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

#include <cassert>

#include "fields.h"
#include "headers.h"

typedef int header_id_t;

class PHV
{
private:
  typedef std::reference_wrapper<Header> HeaderRef;
  typedef std::reference_wrapper<Field> FieldRef;

private:
  std::vector<Header> headers;
  std::unordered_map<std::string, HeaderRef> headers_map;
  std::unordered_map<std::string, FieldRef> fields_map;

public:
  header_id_t push_back_header(const string &header_name,
			       const HeaderType &header_type)
  {
    // use emplace_back instead?
    headers.push_back( Header(header_name, header_type) );

    header_id_t header_index = headers.size() - 1;

    headers_map.emplace(header_name, get_header(header_index));

    for(int i = 0; i < header_type.get_num_fields(); i++) {
      const string name = header_name + "." + header_type.get_field_name(i);
      fields_map.emplace(name, get_field(header_index, i));
    }

    return header_index;
  }

  Header &get_header(header_id_t header_index) {
    return headers[header_index];
  }

  const Header &get_header(header_id_t header_index) const {
    return headers[header_index];
  }

  Header &get_header(const std::string &header_name) {
    return headers_map.at(header_name);
  }

  const Header &get_header(const std::string &header_name) const {
    return headers_map.at(header_name);
  }

  Field &get_field(header_id_t header_index, int field_offset) {
    return headers[header_index].get_field(field_offset);
  }

  const Field &get_field(header_id_t header_index, int field_offset) const {
    return headers[header_index].get_field(field_offset);
  }

  Field &get_field(const std::string &field_name) {
    return fields_map.at(field_name);
  }

  const Field &get_field(const std::string &field_name) const {
    return fields_map.at(field_name);
  }

  void reset(); // mark all headers as invalid

};


#endif
