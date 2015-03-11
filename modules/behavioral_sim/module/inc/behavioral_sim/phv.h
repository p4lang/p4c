#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

#include <cassert>

#include "fields.h"
#include "headers.h"
#include "named_p4object.h"

typedef p4object_id_t header_id_t;

class PHV
{
private:
  typedef std::reference_wrapper<Header> HeaderRef;
  typedef std::reference_wrapper<Field> FieldRef;

public:
  PHV() {}

  PHV(size_t num_headers)
    : capacity(num_headers) {
    // this is needed, otherwise our references will not be valid anymore
    headers.reserve(num_headers);
  }

  // all headers need to be pushed back in order (according to header_index) !!!
  // TODO: remove this constraint?
  void push_back_header(const string &header_name,
			header_id_t header_index,
			const HeaderType &header_type) {
    assert(header_index < (int) capacity);
    assert(header_index == (int) headers.size());
    headers.push_back(Header(header_name, header_index, header_type));

    headers_map.emplace(header_name, get_header(header_index));

    for(int i = 0; i < header_type.get_num_fields(); i++) {
      const string name = header_name + "." + header_type.get_field_name(i);
      // std::cout << header_index << " " << i << " " << name << std::endl;
      fields_map.emplace(name, get_field(header_index, i));
    }
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

private:
  std::vector<Header> headers;
  std::unordered_map<std::string, HeaderRef> headers_map;
  std::unordered_map<std::string, FieldRef> fields_map;
  size_t capacity{0};
};


#endif
