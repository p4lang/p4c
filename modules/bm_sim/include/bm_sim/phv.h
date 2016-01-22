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

//! @file phv.h

#ifndef BM_SIM_INCLUDE_BM_SIM_PHV_H_
#define BM_SIM_INCLUDE_BM_SIM_PHV_H_

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

namespace bm {

typedef p4object_id_t header_id_t;

// forward declaration
class PHVFactory;

//! Each Packet instance owns a PHV instance, used to store all the data
//! extracted from the packet by parsing. It essentially consists of a vector of
//! Header instances, each one of these Header instance itself consisting of a
//! vector of Field instances. The PHV also owns the HeaderStack instances for
//! the packet.
//!
//! Because PHV objects are expensive to construct, we maintain a pool of
//! them. Every time a new Packet is constructed, we retrieve one PHV from the
//! pool (construct a new one if the pool is empty). When the Packet is
//! destroyed, we release its PHV to the pool. Because the PHV depends on the P4
//! Context, there is actually one PHV pool per Contex. If the Context of a
//! Packet changes, its current PHV is released to the old pool and a new PHV is
//! obtained from the new pool.
//! Because we use a pool system, a Packet may receive a PHV which still
//! contains "state" (e.g. field values) from its previous Packet owner. This is
//! why we expose methods like reset(), reset_header_stacks() and
//! reset_metadata().
class PHV {
 public:
  friend class PHVFactory;
  friend class Packet;

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

  //! Access the Header with id \p header_index, with no bound checking.
  Header &get_header(header_id_t header_index) {
    return headers[header_index];
  }

  //! @copydoc get_header(header_id_t header_index)
  const Header &get_header(header_id_t header_index) const {
    return headers[header_index];
  }

  //! Access the Header with name \p header_name. If \p header_name does not
  //! match any known headers, an std::out_of_range exception will be
  //! thrown.
  Header &get_header(const std::string &header_name) {
    return headers_map.at(header_name);
  }

  //! @copydoc get_header(const std::string &header_name)
  const Header &get_header(const std::string &header_name) const {
    return headers_map.at(header_name);
  }

  //! Returns true if there exists a Header with name \p header_name in this PHV
  bool has_header(const std::string &header_name) const {
    auto it = headers_map.find(header_name);
    return (it != headers_map.end());
  }

  //! Access the Field at offset \p field_offset in the Header with id \p
  //! header_index.
  //! See PHV::get_header(header_id_t header_index) and
  //! Header::get_field(int field_offset) for more information.
  Field &get_field(header_id_t header_index, int field_offset) {
    return headers[header_index].get_field(field_offset);
  }

  //! @copydoc get_field(header_id_t header_index, int field_offset)
  const Field &get_field(header_id_t header_index, int field_offset) const {
    return headers[header_index].get_field(field_offset);
  }

  //! Access the Field with name \p field_name. If \p field_name does not match
  //! any known fields, an std::out_of_range exception will be thrown. \p
  //! field_name must follow the `"hdr.f"` format.
  Field &get_field(const std::string &field_name) {
    return fields_map.at(field_name);
  }

  //! @copydoc get_field(const std::string &field_name)
  const Field &get_field(const std::string &field_name) const {
    return fields_map.at(field_name);
  }

  //! Returns true if there exists a Field with name \p field_name in this
  //! PHV. \p field_name must follow the `"hdr.f"` format.
  bool has_field(const std::string &field_name) const {
    auto it = fields_map.find(field_name);
    return (it != fields_map.end());
  }

  //! Access the HeaderStack with id \p header_stack_index, with no bound
  //! checking.
  HeaderStack &get_header_stack(header_stack_id_t header_stack_index) {
    return header_stacks[header_stack_index];
  }

  //! @copydoc get_header_stack(header_stack_id_t header_stack_index)
  const HeaderStack &get_header_stack(
      header_stack_id_t header_stack_index) const {
    return header_stacks[header_stack_index];
  }

  //! Mark all Header instances in the PHV as invalid.
  void reset();

  //! Reset the state (i.e. make them empty) of all HeaderStack instances in the
  //! PHV.
  void reset_header_stacks();

  //! Reset all metadata fields to `0`. If your target assumes that metadata
  //! fields are zero-initialized for every incoming packet, you will need to
  //! call this on the PHV member of every new Packet you create.
  void reset_metadata();

  //! Deleted copy constructor
  PHV(const PHV &other) = delete;
  //! Deleted copy assignment operator
  PHV &operator=(const PHV &other) = delete;

  //! Default move constructor
  PHV(PHV &&other) = default;
  //! Default move assignment operator
  PHV &operator=(PHV &&other) = default;

  //! Copy the headers of \p src PHV to this PHV. The two PHV instances need to
  //! belong to the same Context.
  //! Every header:
  //!   - is marked valid iff the corresponding \p src header is valid
  //!   - is marked as metadata iff the corresponding \p src header is metadata
  //!   - receives the same field values as the corresponding \p src header iff
  //! it is a valid packet header or a metadata header
  void copy_headers(const PHV &src) {
    for (unsigned int h = 0; h < headers.size(); h++) {
      headers[h].valid = src.headers[h].valid;
      headers[h].metadata = src.headers[h].metadata;
      if (headers[h].valid || headers[h].metadata)
        headers[h].fields = src.headers[h].fields;
    }
  }

  void set_packet_id(const uint64_t id1, const uint64_t id2) {
    packet_id = {id1, id2};
  }

 private:
  // To  be used only by PHVFactory
  // all headers need to be pushed back in order (according to header_index) !!!
  // TODO(antonin): remove this constraint?
  void push_back_header(const std::string &header_name,
                        header_id_t header_index,
                        const HeaderType &header_type,
                        const std::set<int> &arith_offsets,
                        const bool metadata);

  void push_back_header_stack(const std::string &header_stack_name,
                              header_stack_id_t header_stack_index,
                              const HeaderType &header_type,
                              const std::vector<header_id_t> &header_ids);

 private:
  std::vector<Header> headers{};
  std::vector<HeaderStack> header_stacks{};
  std::unordered_map<std::string, HeaderRef> headers_map{};
  std::unordered_map<std::string, FieldRef> fields_map{};
  size_t capacity{0};
  size_t capacity_stacks{0};
  Debugger::PacketId packet_id;
};

class PHVFactory {
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
      for (int offset = 0; offset < header_type.get_num_fields(); offset++) {
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
    HeaderDesc desc = HeaderDesc(header_name, header_index,
                                 header_type, metadata);
    // cannot use operator[] because it requires default constructibility
    header_descs.insert(std::make_pair(header_index, desc));
  }

  void push_back_header_stack(const std::string &header_stack_name,
                              const header_stack_id_t header_stack_index,
                              const HeaderType &header_type,
                              const std::vector<header_id_t> &headers) {
    HeaderStackDesc desc = HeaderStackDesc(
      header_stack_name, header_stack_index, header_type, headers);
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
    for (int offset = 0; offset < desc.header_type.get_num_fields(); offset++) {
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

  void enable_all_arith() {
    for (auto it : header_descs)
      enable_all_field_arith(it.first);
  }

  std::unique_ptr<PHV> create() const {
    std::unique_ptr<PHV> phv(new PHV(header_descs.size(),
                                     header_stack_descs.size()));

    for (const auto &e : header_descs) {
      const HeaderDesc &desc = e.second;
      phv->push_back_header(desc.name, desc.index,
                            desc.header_type, desc.arith_offsets,
                            desc.metadata);
    }

    for (const auto &e : header_stack_descs) {
      const HeaderStackDesc &desc = e.second;
      phv->push_back_header_stack(desc.name, desc.index,
                                  desc.header_type, desc.headers);
    }

    return phv;
  }

 private:
  std::map<header_id_t, HeaderDesc> header_descs{};  // sorted by header id
  std::map<header_stack_id_t, HeaderStackDesc> header_stack_descs{};
};

}  // namespace bm

#endif  // BM_SIM_INCLUDE_BM_SIM_PHV_H_
