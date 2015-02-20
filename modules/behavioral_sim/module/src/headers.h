#ifndef _BM_HEADERS_H_
#define _BM_HEADERS_H_

#include <vector>
#include <string>

#include "fields.h"

using std::vector;
using std::string;

typedef int header_type_id_t;

class HeaderType
{
private:
  header_type_id_t type_id;
  // TODO: replace vectors by map ?
  vector<int> fields_bit_width;
  vector<string> fields_name;
  string name;

public:
  HeaderType(header_type_id_t type_id, const string &name)
    : type_id(type_id), name(name) {}

  // returns field offset
  int push_back_field(const string &field_name, int field_bit_width) {
    fields_bit_width.push_back(field_bit_width);
    fields_name.push_back(field_name);
    return fields_bit_width.size() - 1;
  }

  int get_bit_width(int field_offset) const {
    return fields_bit_width[field_offset];
  }

  header_type_id_t get_type_id() const {
    return type_id;
  }

  int get_num_fields() const {
    return fields_bit_width.size();
  }

  int get_field_offset(const string &field_name) const {
    int res = 0;
    while(field_name != fields_name[res]) res++;
    return res;
  }
};

class Header
{
private:
  string name;
  const HeaderType &header_type;
  std::vector<Field> fields;
  bool valid;
  int nbytes_phv;
  int nbytes_packet;

public:
  Header(const string &name, const HeaderType &header_type)
    : name(name), header_type(header_type) {
    valid = false;
    nbytes_phv = 0;
    nbytes_packet = 0;
    // header_type_id = header_type.get_type_id();
    for(int i = 0; i < header_type.get_num_fields(); i++) {
      // use emplace_back instead?
      fields.push_back(Field(header_type.get_bit_width(i)));
      nbytes_phv += fields.back().get_nbytes();
      nbytes_packet += fields.back().get_nbits();
    }
    assert(nbytes_packet % 8 == 0);
    nbytes_packet /= 8;
  }

  int get_nbytes_packet() const {
    return nbytes_packet;
  }

  bool is_valid() const{
    return valid;
  }

  void mark_valid() {
    valid = true;
  }

  void mark_invalid() {
    valid = false;
  }

  Field &get_field(int field_offset) {
    return fields[field_offset];
  }

  const Field &get_field(int field_offset) const {
    return fields[field_offset];
  }

  const HeaderType &get_header_type() const { return header_type; }
  
  header_type_id_t get_header_type_id() const {
    return header_type.get_type_id();
  }

  void extract(const char *data) {
    int hdr_offset = 0;
    for(std::vector<Field>::iterator it = fields.begin();
	it != fields.end();
	++it) {
      hdr_offset += it->extract(data, hdr_offset);
      data += hdr_offset / 8;
      hdr_offset = hdr_offset % 8;
    }
    mark_valid();
    return;
  }

  void deparse(char *data) const {
    int hdr_offset = 0;
    for(std::vector<Field>::const_iterator it = fields.begin();
	it != fields.end();
	++it) {
      hdr_offset += it->deparse(data, hdr_offset);
      data += hdr_offset / 8;
      hdr_offset = hdr_offset % 8;
    }
    return;
  }
};

#endif
