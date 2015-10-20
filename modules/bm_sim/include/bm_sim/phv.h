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

#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <set>
#include <map>
#include <memory>

#include <cassert>

#include "fields.h"
#include "headers.h"
#include "header_stacks.h"
#include "named_p4object.h"
#include "expressions.h"

typedef p4object_id_t header_id_t;

// forward declaration
class PHVFactory;

class PHV
{
public:
  friend class PHVFactory;

private:
  typedef std::reference_wrapper<Header> HeaderRef;
  typedef std::reference_wrapper<Field> FieldRef;

public:
  PHV() {}

  PHV(size_t num_headers, size_t num_header_stacks)
    : capacity(num_headers), capacity_stacks(num_header_stacks) {
    // this is needed, otherwise our references will not be valid anymore
    headers.reserve(num_headers);
    header_stacks.reserve(num_header_stacks);
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

  bool has_header(const std::string &header_name) const {
    auto it = headers_map.find(header_name);
    return (it != headers_map.end());
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

  bool has_field(const std::string &field_name) const {
    auto it = fields_map.find(field_name);
    return (it != fields_map.end());
  }

  HeaderStack &get_header_stack(header_stack_id_t header_stack_index) {
    return header_stacks[header_stack_index];
  }

  const HeaderStack &get_header_stack(header_stack_id_t header_stack_index) const {
    return header_stacks[header_stack_index];
  }

  void reset(); // mark all headers as invalid

  void reset_header_stacks();

  void reset_metadata();

  PHV(const PHV &other) = delete;
  PHV &operator=(const PHV &other) = delete;

  PHV(PHV &&other) = default;
  PHV &operator=(PHV &&other) = default;

  void copy_headers(const PHV &src) {
    for(unsigned int h = 0; h < headers.size(); h++) {
      headers[h].valid = src.headers[h].valid;
      headers[h].metadata = src.headers[h].metadata;
      if(headers[h].valid || headers[h].metadata)
	headers[h].fields = src.headers[h].fields;
    }
  }

private:
  // To  be used only by PHVFactory
  // all headers need to be pushed back in order (according to header_index) !!!
  // TODO: remove this constraint?
  void push_back_header(
      const std::string &header_name,
      header_id_t header_index,
      const HeaderType &header_type,
      const std::set<int> &arith_offsets,
      const bool metadata
  );

  void push_back_header_stack(
      const std::string &header_stack_name,
      header_stack_id_t header_stack_index,
      const HeaderType &header_type,
      const std::vector<header_id_t> &header_ids
  );

private:
  std::vector<Header> headers{};
  std::vector<HeaderStack> header_stacks{};
  std::unordered_map<std::string, HeaderRef> headers_map{};
  std::unordered_map<std::string, FieldRef> fields_map{};
  size_t capacity{0};
  size_t capacity_stacks{0};
};

class PHVFactory
{
private:
  struct HeaderDesc {
    const std::string name;
    header_id_t index;
    const HeaderType &header_type;
    std::set<int> arith_offsets{};
    bool metadata;

    HeaderDesc(const std::string &name, const header_id_t index,
	       const HeaderType &header_type, const bool metadata)
      : name(name), index(index), header_type(header_type), metadata(metadata) {
      for(int offset = 0; offset < header_type.get_num_fields(); offset++) {
	arith_offsets.insert(offset);
      }
    }
  };

  struct HeaderStackDesc {
    const std::string name;
    header_stack_id_t index;
    const HeaderType &header_type;
    std::vector<header_id_t> headers;

    HeaderStackDesc(const std::string &name, const header_stack_id_t index,
		    const HeaderType &header_type,
		    const std::vector<header_id_t> &headers)
      : name(name), index(index), header_type(header_type), headers(headers) { }
  };

public:
  void push_back_header(const std::string &header_name,
			const header_id_t header_index,
			const HeaderType &header_type,
			const bool metadata = false) {
    HeaderDesc desc = HeaderDesc(header_name, header_index, header_type, metadata);
    // cannot use operator[] because it requires default constructibility
    header_descs.insert(std::make_pair(header_index, desc));
  }

  void push_back_header_stack(const std::string &header_stack_name,
			      const header_stack_id_t header_stack_index,
			      const HeaderType &header_type,
			      const std::vector<header_id_t> &headers) {
    HeaderStackDesc desc = HeaderStackDesc(
      header_stack_name, header_stack_index, header_type, headers
    );
    // cannot use operator[] because it requires default constructibility
    header_stack_descs.insert(std::make_pair(header_stack_index, desc));
  }

  const HeaderType &get_header_type(header_id_t header_id) const {
    return header_descs.at(header_id).header_type;
  }

  void enable_field_arith(header_id_t header_id, int field_offset) {
    HeaderDesc &desc = header_descs.at(header_id);
    desc.arith_offsets.insert(field_offset);
  }

  void enable_all_field_arith(header_id_t header_id) {
    HeaderDesc &desc = header_descs.at(header_id);
    for(int offset = 0; offset < desc.header_type.get_num_fields(); offset++) {
      desc.arith_offsets.insert(offset);
    }
  }

  void disable_field_arith(header_id_t header_id, int field_offset) {
    HeaderDesc &desc = header_descs.at(header_id);
    desc.arith_offsets.erase(field_offset);
  }

  void disable_all_field_arith(header_id_t header_id) {
    HeaderDesc &desc = header_descs.at(header_id);
    desc.arith_offsets.clear();
  }

  std::unique_ptr<PHV> create() const {
    std::unique_ptr<PHV> phv(new PHV(header_descs.size(),
				     header_stack_descs.size()));

    for(const auto &e : header_descs) {
      const HeaderDesc &desc = e.second;
      phv->push_back_header(desc.name, desc.index,
			    desc.header_type, desc.arith_offsets,
			    desc.metadata);
    }

    for(const auto &e : header_stack_descs) {
      const HeaderStackDesc &desc = e.second;
      phv->push_back_header_stack(desc.name, desc.index,
				  desc.header_type, desc.headers);
    }

    return phv;
  }

private:
  std::map<header_id_t, HeaderDesc> header_descs{}; // sorted by header id
  std::map<header_stack_id_t, HeaderStackDesc> header_stack_descs{};
};

#endif
