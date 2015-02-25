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
public:
  typedef vector<Field>::iterator iterator;
  typedef vector<Field>::const_iterator const_iterator;
  typedef vector<Field>::reference reference;
  typedef vector<Field>::const_reference const_reference;
  typedef size_t size_type;

public:
  Header(const string &name, const HeaderType &header_type);

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

  void extract(const char *data);

  void deparse(char *data) const;

  // iterators
  iterator begin() { return fields.begin(); }

  const_iterator begin() const { return fields.begin(); }

  iterator end() { return fields.end(); }

  const_iterator end() const { return fields.end(); }

  reference operator[](size_type n) {
    assert(n < fields.size());
    return fields[n];
  }

  const_reference operator[](size_type n) const {
    assert(n < fields.size());
    return fields[n];
  }

private:
  string name;
  const HeaderType &header_type;
  std::vector<Field> fields;
  bool valid;
  int nbytes_phv;
  int nbytes_packet;
};

#endif
