#ifndef _BM_HEADERS_H_
#define _BM_HEADERS_H_

#include <vector>
#include <string>
#include <set>

#include "fields.h"
#include "named_p4object.h"

using std::vector;
using std::string;

typedef p4object_id_t header_type_id_t;

class HeaderType : public NamedP4Object {
public:
  HeaderType(const string &name, p4object_id_t id)
    : NamedP4Object(name, id) {}

  // returns field offset
  int push_back_field(const string &field_name, int field_bit_width) {
    fields_bit_width.push_back(field_bit_width);
    fields_name.push_back(field_name);
    return fields_bit_width.size() - 1;
  }

  int get_bit_width(int field_offset) const {
    return fields_bit_width[field_offset];
  }

  const string &get_field_name(int field_offset) const {
    return fields_name[field_offset];
  }

  header_type_id_t get_type_id() const {
    return get_id();
  }

  int get_num_fields() const {
    return fields_bit_width.size();
  }

  int get_field_offset(const string &field_name) const {
    int res = 0;
    while(field_name != fields_name[res]) res++;
    return res;
  }

private:
  vector<int> fields_bit_width{};
  vector<string> fields_name{};
};

class Header : public NamedP4Object
{
public:
  typedef vector<Field>::iterator iterator;
  typedef vector<Field>::const_iterator const_iterator;
  typedef vector<Field>::reference reference;
  typedef vector<Field>::const_reference const_reference;
  typedef size_t size_type;

  friend class PHV;

public:
  Header(const string &name, p4object_id_t id, const HeaderType &header_type,
	 const std::set<int> &arith_offsets);

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

  // prefer operator [] to those functions
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

  // return the number of fields
  size_type size() const noexcept { return fields.size(); }

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

  Header(const Header &other) = delete;
  Header &operator=(const Header &other) = delete;

  Header(Header &&other) = default;
  Header &operator=(Header &&other) = default;

private:
  const HeaderType &header_type;
  std::vector<Field> fields{};
  bool valid{false};
  int nbytes_phv{0};
  int nbytes_packet{0};
};

#endif
