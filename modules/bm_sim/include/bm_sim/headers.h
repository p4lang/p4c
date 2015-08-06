/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

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

  int get_bit_width() const {
    int bitwidth = 0;
    for(int i : fields_bit_width)
      bitwidth += i;
    return bitwidth;
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
	 const std::set<int> &arith_offsets, const bool metadata = false);

  int get_nbytes_packet() const {
    return nbytes_packet;
  }

  bool is_valid() const {
    return (metadata || valid);
  }

  bool is_metadata() const {
    return metadata;
  }

  void mark_valid() {
    valid = true;
  }

  void mark_invalid() {
    valid = false;
  }

  void reset() {
    for(Field &f : fields)
      f.set(0);
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

  // useful for header stacks
  void swap_values(Header &other) {
    std::swap(valid, other.valid);
    // cannot do that, would invalidate references
    // std::swap(fields, other.fields);
    for(size_t i = 0; i < fields.size(); i++) {
      std::swap(fields[i], other.fields[i]);
    }
  }

  Header(const Header &other) = delete;
  Header &operator=(const Header &other) = delete;

  // TODO: this may as well be delete's I think, class a a reference member
  Header(Header &&other) = default;
  Header &operator=(Header &&other) = default;

private:
  const HeaderType &header_type;
  std::vector<Field> fields{};
  bool valid{false};
  bool metadata{false};
  int nbytes_phv{0};
  int nbytes_packet{0};
};

#endif
