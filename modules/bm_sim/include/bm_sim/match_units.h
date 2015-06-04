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

#ifndef _BM_MATCH_UNITS_H_
#define _BM_MATCH_UNITS_H_

#include <string>
#include <unordered_map>
#include <vector>

// shared_mutex will only be available in C++-14, so for now I'm using boost
#include <boost/thread/shared_mutex.hpp>

#include "match_error_codes.h"
#include "bytecontainer.h"
#include "phv.h"
#include "packet.h"
#include "handle_mgr.h"
#include "lpm_trie.h"

typedef uintptr_t entry_handle_t;

// using string and not ByteBontainer for efficiency
struct MatchKeyParam {
  enum class Type {
    EXACT,
    LPM,
    TERNARY,
    VALID
  };

  MatchKeyParam(const Type &type, std::string key)
    : type(type), key(std::move(key)) { }

  MatchKeyParam(const Type &type, std::string key, std::string mask)
    : type(type), key(std::move(key)), mask(std::move(mask)) { }

  MatchKeyParam(const Type &type, std::string key, int prefix_length)
    : type(type), key(std::move(key)), prefix_length(prefix_length) { }

  Type type;
  std::string key;
  std::string mask{}; // optional
  int prefix_length{0}; // optional
};

struct MatchKeyBuilder
{
  std::vector<header_id_t> valid_headers{};
  std::vector<std::pair<header_id_t, int> > fields{};
  size_t nbytes_key{0};

  void push_back_field(header_id_t header, int field_offset, size_t nbits) {
    fields.push_back(std::pair<header_id_t, int>(header, field_offset));
    nbytes_key += (nbits + 7) / 8;
  }

  void push_back_valid_header(header_id_t header) {
    valid_headers.push_back(header);
    nbytes_key++;
  }

  void operator()(const PHV &phv, ByteContainer &key) const
  {
    for(const auto &h : valid_headers) {
      key.push_back(phv.get_header(h).is_valid() ? '\x01' : '\x00');
    }
    for(const auto &p : fields) {
      const Field &field = phv.get_field(p.first, p.second);
      key.append(field.get_bytes());
    }
  }

  size_t get_nbytes_key() const { return nbytes_key; }
};

template <typename V>
class MatchUnitAbstract
{
public:
  struct MatchUnitLookup {
    MatchUnitLookup(entry_handle_t handle, const V *value)
      : handle(handle), value(value) { }

    bool found() const { return (value != nullptr); }

    static MatchUnitLookup empty_entry() { return MatchUnitLookup(0, nullptr); }

    entry_handle_t handle{0};
    const V *value{nullptr};
  };

public:
  MatchUnitAbstract(size_t size, const MatchKeyBuilder &match_key_builder)
    : size(size),
      nbytes_key(match_key_builder.get_nbytes_key()),
      match_key_builder(match_key_builder) { }

  virtual ~MatchUnitAbstract() { }

  MatchUnitLookup lookup(const Packet &pkt) const;

  virtual MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
				   V value, // by value for possible std::move
				   entry_handle_t *handle,
				   int priority = -1) = 0;
  virtual MatchErrorCode delete_entry(entry_handle_t handle) = 0;
  virtual MatchErrorCode modify_entry(entry_handle_t handle, V value) = 0;
  virtual MatchErrorCode get_value(entry_handle_t handle, const V **value) = 0;

  size_t get_num_entries() const { return num_entries; }

  size_t get_size() const { return size; }

  size_t get_nbytes_key() const { return nbytes_key; }

  bool valid_handle(entry_handle_t handle) const;

protected:
  MatchErrorCode get_and_set_handle(entry_handle_t *handle);
  MatchErrorCode unset_handle(entry_handle_t handle);

private:
  virtual MatchUnitLookup lookup_key(const ByteContainer &key) const = 0;

  // TODO: change key to pointer
  void build_key(const PHV &phv, ByteContainer &key) const {
    match_key_builder(phv, key);
  }

protected:
  size_t size{0};
  size_t num_entries{0};
  size_t nbytes_key;
  HandleMgr handles{};
  MatchKeyBuilder match_key_builder;
};

template <typename V>
class MatchUnitExact : public MatchUnitAbstract<V>
{
public:
  typedef typename MatchUnitAbstract<V>::MatchUnitLookup MatchUnitLookup;

public:
  MatchUnitExact(size_t size, const MatchKeyBuilder &match_key_builder)
    : MatchUnitAbstract<V>(size, match_key_builder),
      entries(size) {
    entries_map.reserve(size);
  }

private:
  struct Entry {
    Entry() { }

    Entry(ByteContainer key, V value)
      : key(std::move(key)), value(std::move(value)) { }

    ByteContainer key{};
    V value{};
  };

private:
  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   V value, // by value for possible std::move
			   entry_handle_t *handle,
			   int prority = -1) override;

  MatchErrorCode delete_entry(entry_handle_t handle) override;

  MatchErrorCode modify_entry(entry_handle_t handle, V value) override;

  MatchErrorCode get_value(entry_handle_t handle, const V **value) override;

  MatchUnitLookup lookup_key(const ByteContainer &key) const override;

private:
  std::vector<Entry> entries{};
  std::unordered_map<ByteContainer, entry_handle_t, ByteContainerKeyHash>
  entries_map{};
};

template <typename V>
class MatchUnitLPM : public MatchUnitAbstract<V>
{
public:
  typedef typename MatchUnitAbstract<V>::MatchUnitLookup MatchUnitLookup;

public:
  MatchUnitLPM(size_t size, const MatchKeyBuilder &match_key_builder)
    : MatchUnitAbstract<V>(size, match_key_builder),
      entries(size),
      entries_trie(this->nbytes_key) { }

private:
  struct Entry {
    Entry() { }

    Entry(ByteContainer key, int prefix_length, V value)
      : key(std::move(key)), prefix_length(prefix_length),
	value(std::move(value)) { }

    ByteContainer key{};
    int prefix_length{0};
    V value{};
  };

private:
  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   V value, // by value for possible std::move
			   entry_handle_t *handle,
			   int prority = -1) override;

  MatchErrorCode delete_entry(entry_handle_t handle) override;

  MatchErrorCode modify_entry(entry_handle_t handle, V value) override;

  MatchErrorCode get_value(entry_handle_t handle, const V **value) override;

  MatchUnitLookup lookup_key(const ByteContainer &key) const override;

private:
  std::vector<Entry> entries{};
  LPMTrie entries_trie;
};

template <typename V>
class MatchUnitTernary : public MatchUnitAbstract<V>
{
public:
  typedef typename MatchUnitAbstract<V>::MatchUnitLookup MatchUnitLookup;

public:
  MatchUnitTernary(size_t size, const MatchKeyBuilder &match_key_builder)
    : MatchUnitAbstract<V>(size, match_key_builder),
      entries(size) { }

private:
  struct Entry {
    Entry() { }

    Entry(ByteContainer key, ByteContainer mask, int priority, V value)
      : key(std::move(key)), mask(std::move(mask)), priority(priority),
	value(std::move(value)) { }

    ByteContainer key{};
    ByteContainer mask{};
    int priority{0};
    V value{};
  };

private:
  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   V value, // by value for possible std::move
			   entry_handle_t *handle,
			   int prority = -1) override;

  MatchErrorCode delete_entry(entry_handle_t handle) override;

  MatchErrorCode modify_entry(entry_handle_t handle, V value) override;

  MatchErrorCode get_value(entry_handle_t handle, const V **value) override;

  MatchUnitLookup lookup_key(const ByteContainer &key) const override;

private:
  std::vector<Entry> entries{};
};

#endif
