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
#include "named_p4object.h"

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

  PHV(size_t num_headers)
    : capacity(num_headers) {
    // this is needed, otherwise our references will not be valid anymore
    headers.reserve(num_headers);
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

  PHV(const PHV &other) = delete;
  PHV &operator=(const PHV &other) = delete;

  PHV(PHV &&other) = default;
  PHV &operator=(PHV &&other) = default;

  void copy_headers(const PHV &src) {
    for(unsigned int h = 0; h < headers.size(); h++) {
      headers[h].valid = src.headers[h].valid;
      if(headers[h].valid) headers[h].fields = src.headers[h].fields;
    }
  }

private:
  // To  be used only by PHVFactory
  // all headers need to be pushed back in order (according to header_index) !!!
  // TODO: remove this constraint?
  void push_back_header(
      const string &header_name,
      header_id_t header_index,
      const HeaderType &header_type,
      const std::set<int> &arith_offsets
  ) {
    assert(header_index < (int) capacity);
    assert(header_index == (int) headers.size());
    headers.push_back(
        Header(header_name, header_index, header_type, arith_offsets)
    );

    headers_map.emplace(header_name, get_header(header_index));

    for(int i = 0; i < header_type.get_num_fields(); i++) {
      const string name = header_name + "." + header_type.get_field_name(i);
      // std::cout << header_index << " " << i << " " << name << std::endl;
      fields_map.emplace(name, get_field(header_index, i));
    }
  }

private:
  std::vector<Header> headers;
  std::unordered_map<std::string, HeaderRef> headers_map;
  std::unordered_map<std::string, FieldRef> fields_map;
  size_t capacity{0};
};

class PHVFactory
{
private:
  struct HeaderDesc {
    const std::string name;
    header_id_t index;
    const HeaderType &header_type;
    std::set<int> arith_offsets;

    HeaderDesc(const std::string &name, const header_id_t index,
	       const HeaderType &header_type)
      : name(name), index(index), header_type(header_type) {
      for(int offset = 0; offset < header_type.get_num_fields(); offset++) {
	arith_offsets.insert(offset);
      }
    }
  };

public:
  void push_back_header(const string &header_name,
			const header_id_t header_index,
			const HeaderType &header_type) {
    HeaderDesc desc = HeaderDesc(header_name, header_index, header_type);
    // cannot use opeartor[] because it requires default constructibility
    header_descs.insert(std::make_pair(header_index, desc));
  }

  const HeaderType &get_header_type(header_id_t header_id) const {
    return header_descs.at(header_id).header_type;
  }

  void enable_field_arith(header_id_t header_id, int field_offset) {
    HeaderDesc &desc = header_descs.at(header_id);
    desc.arith_offsets.insert(field_offset);
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
    std::unique_ptr<PHV> phv(new PHV(header_descs.size()));
    for(const auto &e : header_descs) {
      const HeaderDesc &desc = e.second;
      phv->push_back_header(desc.name, desc.index,
			    desc.header_type, desc.arith_offsets);
    }
    return phv;
  }

private:
  std::map<header_id_t, HeaderDesc> header_descs; // sorted by header id
};

#endif
