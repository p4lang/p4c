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

#include <bm/bm_sim/match_units.h>
#include <bm/bm_sim/match_key_types.h>
#include <bm/bm_sim/match_tables.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/lookup_structures.h>

#include <limits>
#include <string>
#include <vector>
#include <algorithm>  // for std::copy, std::max

#include <cstring>

#include "utils.h"

namespace bm {

#define HANDLE_VERSION(h) (h >> 24)
#define HANDLE_INTERNAL(h) (h & 0x00ffffff)
#define HANDLE_SET(v, i) ((((uint32_t) (v & 0xff)) << 24) | i)
#define MAX_TABLE_SIZE 0x00ffffff

using MatchUnit::EntryMeta;

namespace {

size_t nbits_to_nbytes(size_t nbits) {
  return (nbits + 7) / 8;
}

}  // namespace

std::string
MatchKeyParam::type_to_string(Type t) {
  switch (t) {
    case Type::RANGE:
      return "RANGE";
    case Type::VALID:
      return "VALID";
    case Type::EXACT:
      return "EXACT";
    case Type::LPM:
      return "LPM";
    case Type::TERNARY:
      return "TERNARY";
  }
  return "";
}

namespace {

// TODO(antonin): basically copied from ByteConatiner, need to avoid duplication
void
// NOLINTNEXTLINE(runtime/references)
dump_hexstring(std::ostream &out, const std::string &s,
               bool upper_case = false) {
  utils::StreamStateSaver state_saver(out);
  for (const char c : s) {
    out << std::setw(2) << std::setfill('0') << std::hex
        << (upper_case ? std::uppercase : std::nouppercase)
        << static_cast<int>(static_cast<unsigned char>(c));
  }
}

}  // namespace

std::ostream& operator<<(std::ostream &out, const MatchKeyParam &p) {
  // need to restore the state right away (thus the additional scope), otherwise
  // dump_hexstring() can give strange results
  {
    utils::StreamStateSaver state_saver(out);
    // type column is 10 chars wide
    out << std::setw(10) << std::left << MatchKeyParam::type_to_string(p.type);
  }
  dump_hexstring(out, p.key);
  switch (p.type) {
    case MatchKeyParam::Type::LPM:
      out << "/" << p.prefix_length;
      break;
    case MatchKeyParam::Type::TERNARY:
      out << " &&& ";
      dump_hexstring(out, p.mask);
      break;
    case MatchKeyParam::Type::RANGE:
      out << " -> ";
      dump_hexstring(out, p.mask);
      break;
    default:
      break;
  }
  return out;
}

void
MatchKeyBuilder::build() {
  if (built) return;

  key_mapping = std::vector<size_t>(key_input.size());
  inv_mapping = std::vector<size_t>(key_input.size());

  size_t n = 0;
  std::generate(inv_mapping.begin(), inv_mapping.end(), [&n]{ return n++; });

  auto sort_fn = [this](size_t i1, size_t i2) {
    int t1 = static_cast<int>(key_input[i1].mtype);
    int t2 = static_cast<int>(key_input[i2].mtype);
    if (t1 == t2) return i1 < i2;
    return t1 < t2;
  };

  std::sort(inv_mapping.begin(), inv_mapping.end(), sort_fn);

  std::vector<KeyF> new_input;
  for (const auto idx : inv_mapping) {
    new_input.push_back(key_input[idx]);
    key_mapping[inv_mapping[idx]] = idx;
  }
  key_input.swap(new_input);

  std::vector<size_t> offsets;
  size_t curr_offset = 0;
  for (const auto &f_info : key_input) {
    offsets.push_back(curr_offset);
    curr_offset += nbits_to_nbytes(f_info.nbits);
  }
  for (size_t i = 0; i < key_mapping.size(); i++)
    key_offsets.push_back(offsets[key_mapping[i]]);

  big_mask = ByteContainer(nbytes_key);
  for (size_t i = 0; i < key_offsets.size(); i++)
    std::copy(masks.at(i).begin(), masks.at(i).end(),
              big_mask.begin() + key_offsets.at(i));

  built = true;
}

// This function is in charge of re-organizing the input key on the fly to
// satisfy the implementation requirements (e.g. LPM matches last...). At the
// same time we keep track of the original order thanks to key_mapping (for
// debugging).
void
MatchKeyBuilder::push_back(KeyF &&input, const ByteContainer &mask,
                           const std::string &name) {
  key_input.push_back(std::move(input));
  size_t f_nbytes = nbits_to_nbytes(input.nbits);
  nbytes_key += f_nbytes;
  masks.push_back(mask);
  name_map.push_back(name);
}

void
MatchKeyBuilder::push_back_field(header_id_t header, int field_offset,
                                 size_t nbits, MatchKeyParam::Type mtype,
                                 const std::string &name) {
  push_back({header, field_offset, mtype, nbits},
            ByteContainer(nbits_to_nbytes(nbits), '\xff'), name);
}

void
MatchKeyBuilder::push_back_field(header_id_t header, int field_offset,
                                 size_t nbits, const ByteContainer &mask,
                                 MatchKeyParam::Type mtype,
                                 const std::string &name) {
  assert(mask.size() == nbits_to_nbytes(nbits));
  push_back({header, field_offset, mtype, nbits}, mask, name);
  has_big_mask = true;
}

void
MatchKeyBuilder::push_back_valid_header(header_id_t header,
                                        const std::string &name) {
  // set "nbits" to 8 (i.e. 1 byte); it is kind of a hack but ensure that most
  // of the code can be the same...
  push_back({header, 0, MatchKeyParam::Type::VALID, 8},
            ByteContainer(1, '\xff'), name);
}

void
MatchKeyBuilder::apply_big_mask(ByteContainer *key) const {
  if (has_big_mask)
    key->apply_mask(big_mask);
}

void
MatchKeyBuilder::operator()(const PHV &phv, ByteContainer *key) const {
  for (const auto &in : key_input) {
    const Header &header = phv.get_header(in.header);
    // if speed is an issue here, I can probably come up with something faster
    // (with a switch statement maybe)
    if (in.mtype == MatchKeyParam::Type::VALID) {
      key->push_back(header.is_valid() ? '\x01' : '\x00');
    } else {
      // we do not reset all fields to 0 in between packets
      // so I need this hack if the P4 programmer assumed that:
      // field not valid => field set to 0
      // for hidden fields, we want the actual value, even though for $valid$,
      // it does not make a difference
      const Field &field = header[in.f_offset];
      if (header.is_valid() || field.is_hidden()) {
        key->append(field.get_bytes());
      } else {
        key->append(std::string(field.get_nbytes(), '\x00'));
      }
    }
  }
  if (has_big_mask)
    key->apply_mask(big_mask);
}

std::vector<std::string>
MatchKeyBuilder::key_to_fields(const ByteContainer &key) const {
  std::vector<std::string> fields;

  size_t nfields = key_mapping.size();
  for (size_t i = 0; i < nfields; i++) {
    const size_t imp_idx = key_mapping.at(i);
    const auto &f_info = key_input.at(imp_idx);
    const size_t byte_offset = key_offsets.at(i);
    auto start = key.begin() + byte_offset;
    fields.emplace_back(start, start + nbits_to_nbytes(f_info.nbits));
  }

  return fields;
}

// TODO(antonin): re-use above function instead?
std::string
MatchKeyBuilder::key_to_string(const ByteContainer &key, std::string separator,
                               bool upper_case) const {
  std::ostringstream ret;

  size_t nfields = key_mapping.size();
  bool first = true;
  for (size_t i = 0; i < nfields; i++) {
    const size_t imp_idx = key_mapping.at(i);
    const auto &f_info = key_input.at(imp_idx);
    const size_t byte_offset = key_offsets.at(i);
    if (!first)
      ret << separator;
    ret << key.to_hex(byte_offset, nbits_to_nbytes(f_info.nbits), upper_case);
    first = false;
  }

  return ret.str();
}

namespace detail {

namespace {

template <typename It>
int
pref_len_from_mask(const It start, const It end) {
  int pref = 0;
  for (auto it = start; it < end; it++) {
    if (*it == '\xff') {
      pref += 8;
      continue;
    }
    char c = *it;
    c -= (c >> 1) & 0x55;
    c = (c & 0x33) + ((c >> 2) & 0x33);
    pref += (c + (c >> 4)) & 0x0f;
  }
  return pref;
}

std::string
create_mask_from_pref_len(int prefix_length, int size) {
  std::string mask(size, '\x00');
  std::fill(mask.begin(), mask.begin() + (prefix_length / 8), '\xff');
  if (prefix_length % 8 != 0) {
    mask[prefix_length / 8] =
      static_cast<unsigned char>(0xFF) << (8 - (prefix_length % 8));
  }
  return mask;
}

void
format_ternary_key(char *key, const char *mask, size_t n) {
  for (size_t byte_index = 0; byte_index < n; byte_index++) {
    key[byte_index] = key[byte_index] & mask[byte_index];
  }
}

char get_byte0_mask(size_t bitwidth) {
  if (bitwidth % 8 == 0) return 0xff;
  int nbits = bitwidth % 8;
  return ((1 << nbits) - 1);
}

}  // namespace

class MatchKeyBuilderHelper {
 public:
  template <typename K,
            typename std::enable_if<K::mut == MatchUnitType::EXACT, int>::type
            = 0>
  static std::vector<MatchKeyParam>
  entry_to_match_params(const MatchKeyBuilder &kb, const K &key) {
    std::vector<MatchKeyParam> params;

    size_t nfields = kb.key_mapping.size();
    for (size_t i = 0; i < nfields; i++) {
      const size_t imp_idx = kb.key_mapping.at(i);
      const auto &f_info = kb.key_input.at(imp_idx);
      const size_t byte_offset = kb.key_offsets.at(i);

      auto start = key.data.begin() + byte_offset;
      auto end = start + nbits_to_nbytes(f_info.nbits);
      assert(f_info.mtype == MatchKeyParam::Type::VALID ||
             f_info.mtype == MatchKeyParam::Type::EXACT);
      params.emplace_back(f_info.mtype, std::string(start, end));
    }

    return params;
  }

  template <typename K,
            typename std::enable_if<K::mut == MatchUnitType::LPM, int>::type
            = 0>
  static std::vector<MatchKeyParam>
  entry_to_match_params(const MatchKeyBuilder &kb, const K &key) {
    std::vector<MatchKeyParam> params;
    size_t LPM_idx = 0;
    int pref = key.prefix_length;

    size_t nfields = kb.key_mapping.size();
    for (size_t i = 0; i < nfields; i++) {
      const size_t imp_idx = kb.key_mapping.at(i);
      const auto &f_info = kb.key_input.at(imp_idx);
      const size_t byte_offset = kb.key_offsets.at(i);

      auto start = key.data.begin() + byte_offset;
      auto end = start + nbits_to_nbytes(f_info.nbits);
      assert(f_info.mtype != MatchKeyParam::Type::TERNARY);

      params.emplace_back(f_info.mtype, std::string(start, end));

      if (f_info.mtype == MatchKeyParam::Type::LPM)
        LPM_idx = i;
      else
        // IMO, same thing
        // pref -= ((end - start) << 3);
        pref -= (std::distance(start, end) << 3);
    }

    params.at(LPM_idx).prefix_length = pref;

    return params;
  }

  template <typename K,
            typename std::enable_if<K::mut == MatchUnitType::TERNARY, int>::type
            = 0>
  static std::vector<MatchKeyParam>
  entry_to_match_params(const MatchKeyBuilder &kb, const K &key) {
    std::vector<MatchKeyParam> params;

    size_t nfields = kb.key_mapping.size();
    for (size_t i = 0; i < nfields; i++) {
      const size_t imp_idx = kb.key_mapping.at(i);
      const auto &f_info = kb.key_input.at(imp_idx);
      const size_t byte_offset = kb.key_offsets.at(i);

      auto start = key.data.begin() + byte_offset;
      size_t nbytes = nbits_to_nbytes(f_info.nbits);
      auto end = start + nbytes;
      switch (f_info.mtype) {
        case MatchKeyParam::Type::VALID:
        case MatchKeyParam::Type::EXACT:
          params.emplace_back(f_info.mtype, std::string(start, end));
          break;
        case MatchKeyParam::Type::RANGE:
          // should only happen for RangeMatchKey
          // range treated the same as ternary
        case MatchKeyParam::Type::TERNARY:
          {
            auto mask_start = key.mask.begin() + byte_offset;
            auto mask_end = mask_start + nbytes;
            params.emplace_back(f_info.mtype, std::string(start, end),
                                std::string(mask_start, mask_end));
            break;
          }
        case MatchKeyParam::Type::LPM:
          {
            auto mask_start = key.mask.begin() + byte_offset;
            auto mask_end = mask_start + nbytes;
            params.emplace_back(f_info.mtype, std::string(start, end),
                                pref_len_from_mask(mask_start, mask_end));
            break;
          }
      }
    }

    return params;
  }

  // TODO(antonin): This avoids code duplication and works because RangeMatchKey
  // inherits from TernaryMatchKey, but is this the best solution?
  template <typename K,
            typename std::enable_if<K::mut == MatchUnitType::RANGE, int>::type
            = 0>
  static std::vector<MatchKeyParam>
  entry_to_match_params(const MatchKeyBuilder &kb, const K &key) {
    return entry_to_match_params<TernaryMatchKey>(kb, key);
  }

  // TODO(antonin):
  // We recently added automatic masking of the first byte of each match
  // param. For example, if a match field is 14 bit wide and we receive value
  // 0xffff from the client (instead of the correct 0x3fff), we will
  // automatically do the conversion. Before that change, we would not perform
  // any checks and simply use the user-provided value, which would cause
  // unexpected dataplane behavior. But is this silent conversion better than
  // returning an error to the client?

  template <typename E, typename std::enable_if<
              decltype(E::key)::mut == MatchUnitType::EXACT, int>::type = 0>
  static E
  match_params_to_entry(const MatchKeyBuilder &kb,
                        const std::vector<MatchKeyParam> &params) {
    E entry;
    entry.key.data.reserve(kb.nbytes_key);

    size_t first_byte = 0;
    for (size_t i = 0; i < kb.inv_mapping.size(); i++) {
      const auto &param = params.at(kb.inv_mapping[i]);
      entry.key.data.append(param.key);
      entry.key.data[first_byte] &= get_byte0_mask(kb.key_input[i].nbits);
      first_byte += param.key.size();
    }

    return entry;
  }

  template <typename E, typename std::enable_if<
              decltype(E::key)::mut == MatchUnitType::LPM, int>::type = 0>
  static E
  match_params_to_entry(const MatchKeyBuilder &kb,
                        const std::vector<MatchKeyParam> &params) {
    E entry;
    entry.key.data.reserve(kb.nbytes_key);
    entry.key.prefix_length = 0;

    size_t first_byte = 0;
    for (size_t i = 0; i < kb.inv_mapping.size(); i++) {
      const auto &param = params.at(kb.inv_mapping[i]);
      entry.key.data.append(param.key);
      entry.key.data[first_byte] &= get_byte0_mask(kb.key_input[i].nbits);
      switch (param.type) {
        case MatchKeyParam::Type::VALID:
          entry.key.prefix_length += 8;
          break;
        case MatchKeyParam::Type::EXACT:
          entry.key.prefix_length += param.key.size() << 3;
          break;
        case MatchKeyParam::Type::LPM:
          entry.key.prefix_length += param.prefix_length;
          break;
        case MatchKeyParam::Type::RANGE:
        case MatchKeyParam::Type::TERNARY:
          assert(0);
      }
      first_byte += param.key.size();
    }

    return entry;
  }

  template <typename E, typename std::enable_if<
              decltype(E::key)::mut == MatchUnitType::TERNARY, int>::type = 0>
  static E
  match_params_to_entry(const MatchKeyBuilder &kb,
                        const std::vector<MatchKeyParam> &params) {
    E entry;
    auto &key = entry.key;
    match_params_to_entry_ternary_and_range(kb, params, &key);
    assert(key.data.size() == key.mask.size());
    format_ternary_key(&key.data[0], &key.mask[0], key.data.size());
    return entry;
  }

  template <typename E, typename std::enable_if<
              decltype(E::key)::mut == MatchUnitType::RANGE, int>::type = 0>
  static E
  match_params_to_entry(const MatchKeyBuilder &kb,
                        const std::vector<MatchKeyParam> &params) {
    E entry;
    match_params_to_entry_ternary_and_range(kb, params, &entry.key);
    size_t w = 0;
    auto &key = entry.key;
    for (size_t i = 0; i < kb.inv_mapping.size(); i++) {
      const auto &param = params.at(kb.inv_mapping[i]);
      if (param.type == MatchKeyParam::Type::RANGE) {
        key.range_widths.push_back(param.key.size());
        w += param.key.size();
      }
    }
    size_t s = key.data.size();
    assert(s == key.mask.size());
    assert(s >= w);
    if (s > w) format_ternary_key(&key.data[w], &key.mask[w], s - w);
    return entry;
  }

 private:
  static void match_params_to_entry_ternary_and_range(
      const MatchKeyBuilder &kb, const std::vector<MatchKeyParam> &params,
      TernaryMatchKey *key) {
    key->data.reserve(kb.nbytes_key);
    key->mask.reserve(kb.nbytes_key);

    size_t first_byte = 0;
    for (size_t i = 0; i < kb.inv_mapping.size(); i++) {
      const auto &param = params.at(kb.inv_mapping[i]);
      key->data.append(param.key);
      key->data[first_byte] &= get_byte0_mask(kb.key_input[i].nbits);
      switch (param.type) {
        case MatchKeyParam::Type::VALID:
          key->mask.append("\xff");
          break;
        case MatchKeyParam::Type::EXACT:
          key->mask.append(std::string(param.key.size(), '\xff'));
          break;
        case MatchKeyParam::Type::LPM:
          key->mask.append(
              create_mask_from_pref_len(param.prefix_length, param.key.size()));
          break;
        case MatchKeyParam::Type::RANGE:
        case MatchKeyParam::Type::TERNARY:
          key->mask.append(param.mask);
          break;
      }
      first_byte += param.key.size();
    }
  }
};

}  // namespace detail

template <typename E>
std::vector<MatchKeyParam>
MatchKeyBuilder::entry_to_match_params(const E &entry) const {
  return detail::MatchKeyBuilderHelper::entry_to_match_params(*this, entry);
}

template <typename E>
E
MatchKeyBuilder::match_params_to_entry(
    const std::vector<MatchKeyParam> &params) const {
  return detail::MatchKeyBuilderHelper::match_params_to_entry<E>(*this, params);
}

bool
MatchKeyBuilder::match_params_sanity_check(
    const std::vector<MatchKeyParam> &params) const {
  if (params.size() != key_input.size()) return false;

  for (size_t i = 0; i < inv_mapping.size(); i++) {
    size_t p_i = inv_mapping[i];
    const auto &param = params[p_i];
    const auto &f_info = key_input[i];

    if (param.type != f_info.mtype) return false;

    size_t nbytes = nbits_to_nbytes(f_info.nbits);
    if (param.key.size() != nbytes) return false;

    switch (param.type) {
      case MatchKeyParam::Type::VALID:
      case MatchKeyParam::Type::EXACT:
        break;
      case MatchKeyParam::Type::LPM:
        if (static_cast<size_t>(param.prefix_length) > f_info.nbits)
          return false;
        break;
      case MatchKeyParam::Type::TERNARY:
        if (param.mask.size() != nbytes) return false;
        break;
      case MatchKeyParam::Type::RANGE:
        if (param.mask.size() != nbytes) return false;
        if (std::memcmp(param.key.data(), param.mask.data(), nbytes) > 0)
          return false;
        break;
    }
  }
  return true;
}

void
MatchKeyBuilder::NameMap::push_back(const std::string &name) {
  names.push_back(name);
  max_s = std::max(max_s, name.size());
}

const std::string &
MatchKeyBuilder::NameMap::get(size_t idx) const {
  return names.at(idx);
}

size_t
MatchKeyBuilder::NameMap::max_size() const {
  return max_s;
}


MatchUnitAbstract_::handle_iterator::handle_iterator(
    const MatchUnitAbstract_ *mu, HandleMgr::const_iterator it)
    : mu(mu), it(it) {
  if (it != mu->handles.end())
    handle = HANDLE_SET(mu->entry_meta.at(*it).version, *it);
}

MatchUnitAbstract_::handle_iterator &
MatchUnitAbstract_::handle_iterator::operator++() {
  assert(it != mu->handles.end() && "Out-of-bounds iterator increment.");
  if (++it != mu->handles.end())
    handle = HANDLE_SET(mu->entry_meta.at(*it).version, *it);
  return *this;
}

MatchUnitAbstract_::handle_iterator
MatchUnitAbstract_::handles_begin() const {
  return handle_iterator(this, handles.begin());
}

MatchUnitAbstract_::handle_iterator
MatchUnitAbstract_::handles_end() const {
  return handle_iterator(this, handles.end());
}

MatchUnitAbstract_::MatchUnitAbstract_(size_t size,
                                       const MatchKeyBuilder &key_builder)
    : size(size), nbytes_key(key_builder.get_nbytes_key()),
      match_key_builder(key_builder), entry_meta(size) {
  if (size > MAX_TABLE_SIZE) {
    Logger::get()->error("Table size is limited to {} but size requested is "
                         "{}, table will therefore be smaller than requested",
                         MAX_TABLE_SIZE, size);
    size = MAX_TABLE_SIZE;
  }
  match_key_builder.build();
}

MatchErrorCode
MatchUnitAbstract_::get_and_set_handle(internal_handle_t *handle) {
  if (num_entries >= size) {  // table is full
    return MatchErrorCode::TABLE_FULL;
  }

  if (handles.get_handle(handle)) return MatchErrorCode::ERROR;

  num_entries++;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
MatchUnitAbstract_::unset_handle(internal_handle_t handle) {
  if (handles.release_handle(handle)) return MatchErrorCode::INVALID_HANDLE;

  num_entries--;
  return MatchErrorCode::SUCCESS;
}

bool
MatchUnitAbstract_::valid_handle_(internal_handle_t handle) const {
  return handles.valid_handle(handle);
}

bool
MatchUnitAbstract_::valid_handle(entry_handle_t handle) const {
  return this->valid_handle_(HANDLE_INTERNAL(handle));
}

EntryMeta &
MatchUnitAbstract_::get_entry_meta(entry_handle_t handle) {
  return this->entry_meta[HANDLE_INTERNAL(handle)];
}

const EntryMeta &
MatchUnitAbstract_::get_entry_meta(entry_handle_t handle) const {
  return this->entry_meta[HANDLE_INTERNAL(handle)];
}

void
MatchUnitAbstract_::reset_counters() {
  // could take a while, but do not block anyone else
  // lock (even read lock) does not have to be held while doing this
  for (EntryMeta &meta : entry_meta) {
    meta.counter.reset_counter();
  }
}

void
MatchUnitAbstract_::set_direct_meters(MeterArray *meter_array) {
  assert(meter_array);
  assert(size == meter_array->size());
  direct_meters = meter_array;
}

Meter &
MatchUnitAbstract_::get_meter(entry_handle_t handle) {
  return direct_meters->at(HANDLE_INTERNAL(handle));
}

MatchErrorCode
MatchUnitAbstract_::set_entry_ttl(entry_handle_t handle, unsigned int ttl_ms) {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  if (!this->valid_handle_(handle_)) return MatchErrorCode::INVALID_HANDLE;
  EntryMeta &meta = entry_meta[handle_];
  meta.timeout_ms = ttl_ms;
  return MatchErrorCode::SUCCESS;
}

void
MatchUnitAbstract_::sweep_entries(std::vector<entry_handle_t> *entries) const {
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto tp = Packet::clock::now();
  uint64_t now_ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
  for (auto it = handles.begin(); it != handles.end(); ++it) {
    const EntryMeta &meta = entry_meta[*it];
    assert(now_ms >= meta.ts.get_ms());
    if (meta.timeout_ms > 0 && (now_ms - meta.ts.get_ms() >= meta.timeout_ms)) {
      entries->push_back(HANDLE_SET(meta.version, *it));
    }
  }
}

void
MatchUnitAbstract_::dump_key_params(
    std::ostream *out, const std::vector<MatchKeyParam> &params,
    int priority) const {
  const auto &kb = match_key_builder;
  *out << "Match key:\n";
  const size_t out_name_w = std::max(size_t(20), kb.max_name_size());
  for (size_t i = 0; i < params.size(); i++) {
    const auto &name = kb.get_name(i);
    *out << "* ";
    if (name != "") {
      utils::StreamStateSaver state_saver(*out);
      *out << std::setw(out_name_w) << std::left << name << ": ";
    }
    *out << params[i] << "\n";
  }
  if (priority >= 0)
    *out << "Priority: " << priority << "\n";
}

std::string
MatchUnitAbstract_::key_to_string_with_names(const ByteContainer &key) const {
  std::ostringstream ret;

  auto values = match_key_builder.key_to_fields(key);
  const size_t out_name_w = std::max(size_t(20),
                                     match_key_builder.max_name_size());
  for (size_t i = 0; i < values.size(); i++) {
    const auto &name = match_key_builder.get_name(i);
    ret << "* ";
    if (name != "") {
      utils::StreamStateSaver state_saver(ret);
      ret << std::setw(out_name_w) << std::left << name << ": ";
    }
    dump_hexstring(ret, values[i]);
    ret << "\n";
  }

  return ret.str();
}

template<typename V>
typename MatchUnitAbstract<V>::MatchUnitLookup
MatchUnitAbstract<V>::lookup(const Packet &pkt) {
  static thread_local ByteContainer key;
  key.clear();
  build_key(*pkt.get_phv(), &key);

  // BMLOG_DEBUG_PKT(pkt, "Looking up key {}", key_to_string(key));
  BMLOG_DEBUG_PKT(pkt, "Looking up key:\n{}", key_to_string_with_names(key));

  MatchUnitLookup res = lookup_key(key);
  if (res.found()) {
    EntryMeta &meta = entry_meta[HANDLE_INTERNAL(res.handle)];
    update_counters(&meta.counter, pkt);
    update_ts(&meta.ts, pkt);
  }
  return res;
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::add_entry(const std::vector<MatchKeyParam> &match_key,
                                V value, entry_handle_t *handle, int priority) {
  MatchErrorCode rc = add_entry_(match_key, std::move(value), handle, priority);
  if (rc != MatchErrorCode::SUCCESS) return rc;
  EntryMeta &meta = entry_meta[HANDLE_INTERNAL(*handle)];
  meta.reset();
  meta.version = HANDLE_VERSION(*handle);
  return rc;
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::delete_entry(entry_handle_t handle) {
  return delete_entry_(handle);
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::modify_entry(entry_handle_t handle, V value) {
  return modify_entry_(handle, std::move(value));
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::get_value(entry_handle_t handle, const V **value) {
  return get_value_(handle, value);
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::get_entry(entry_handle_t handle,
                                std::vector<MatchKeyParam> *match_key,
                                const V **value, int *priority) const {
  return get_entry_(handle, match_key, value, priority);
}

template<typename V>
std::string
MatchUnitAbstract<V>::entry_to_string(entry_handle_t handle) const {
  std::ostringstream ret;
  if (dump_match_entry(&ret, handle) != MatchErrorCode::SUCCESS) {
    Logger::get()->error("entry_to_string() called on invalid handle, "
                         "returning empty string");
    return "";
  }
  return ret.str();
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::dump_match_entry(std::ostream *out,
                                       entry_handle_t handle) const {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  if (!this->valid_handle_(handle_)) return MatchErrorCode::INVALID_HANDLE;
  return dump_match_entry_(out, handle);
}

template<typename V>
void
MatchUnitAbstract<V>::reset_state() {
  this->num_entries = 0;
  this->handles.clear();
  this->entry_meta = std::vector<EntryMeta>(size);
  reset_state_();
}


namespace {
  // Utility to transparently either get the real priority value from a
  // ternary entry or simply return -1 for other types of entries

  int get_priority(const MatchKey &key) {
    (void) key;
    return -1;
  }

  int get_priority(const TernaryMatchKey &key) {
    return key.priority;
  }

  // Matching setter utility

  void set_priority(MatchKey *entry, int p) {
    (void) entry;
    (void) p;
  }
  void set_priority(TernaryMatchKey *entry, int p) {
    entry->priority = p;
  }

}  // anonymous namespace


template <typename K, typename V>
typename MatchUnitGeneric<K, V>::MatchUnitLookup
MatchUnitGeneric<K, V>::lookup_key(const ByteContainer &key) const {
  internal_handle_t handle_;
  bool entry_found = lookup_structure->lookup(key, &handle_);
  if (entry_found) {
    const Entry &entry = entries[handle_];
    entry_handle_t handle = HANDLE_SET(entry.key.version, handle_);
    return MatchUnitLookup(handle, &entry.value);
  }
  return MatchUnitLookup::empty_entry();
}

template <typename K, typename V>
MatchErrorCode
MatchUnitGeneric<K, V>::add_entry_(const std::vector<MatchKeyParam> &match_key,
                                   V value, entry_handle_t *handle,
                                   int priority) {
  const auto &KeyB = this->match_key_builder;

  if (!KeyB.match_params_sanity_check(match_key))
    return MatchErrorCode::BAD_MATCH_KEY;

  // for why "template" keyword is needed, see:
  // http://stackoverflow.com/questions/1840253/n/1840318#1840318
  Entry entry = KeyB.template match_params_to_entry<Entry>(match_key);

  // needs to go before duplicate check, because 2 different user keys can
  // become the same key. We would then have a problem when erasing the key from
  // the hash map.
  // TODO(antonin): maybe change this by modifying delete_entry method
  // TODO(antonin): does this really make sense for a Ternary/LPM table?
  KeyB.apply_big_mask(&entry.key.data);

  // For ternary and range. Must be done before the entry_exists call below
  set_priority(&entry.key, priority);

  // check if the key is already present
  if (lookup_structure->entry_exists(entry.key))
    return MatchErrorCode::DUPLICATE_ENTRY;

  internal_handle_t handle_;
  MatchErrorCode status = this->get_and_set_handle(&handle_);
  if (status != MatchErrorCode::SUCCESS) return status;

  uint32_t version = entries[handle_].key.version;
  *handle = HANDLE_SET(version, handle_);

  entry.value = std::move(value);
  entry.key.version = version;
  entries[handle_] = std::move(entry);

  // calling this after copying the entry into the entries vector, which means
  // that the lookup structure can use a pointer to the entry if it wants to
  // avoid making a copy. This works because the entries vector is NEVER
  // resized, which means the pointer will remain valid.
  lookup_structure->add_entry(entries[handle_].key, handle_);

  return MatchErrorCode::SUCCESS;
}

template <typename K, typename V>
MatchErrorCode
MatchUnitGeneric<K, V>::delete_entry_(entry_handle_t handle) {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  if (!this->valid_handle_(handle_)) return MatchErrorCode::INVALID_HANDLE;
  Entry &entry = entries[handle_];
  if (HANDLE_VERSION(handle) != entry.key.version)
    return MatchErrorCode::EXPIRED_HANDLE;
  entry.key.version += 1;
  lookup_structure->delete_entry(entry.key);

  return this->unset_handle(handle_);
}

template <typename K, typename V>
MatchErrorCode
MatchUnitGeneric<K, V>::modify_entry_(entry_handle_t handle, V value) {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  if (!this->valid_handle_(handle_)) return MatchErrorCode::INVALID_HANDLE;
  Entry &entry = entries[handle_];
  if (HANDLE_VERSION(handle) != entry.key.version)
    return MatchErrorCode::EXPIRED_HANDLE;
  entry.value = std::move(value);

  return MatchErrorCode::SUCCESS;
}

template <typename K, typename V>
MatchErrorCode
MatchUnitGeneric<K, V>::get_value_(entry_handle_t handle, const V **value) {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  if (!this->valid_handle_(handle_)) return MatchErrorCode::INVALID_HANDLE;
  Entry &entry = entries[handle_];
  if (HANDLE_VERSION(handle) != entry.key.version)
    return MatchErrorCode::EXPIRED_HANDLE;
  *value = &entry.value;

  return MatchErrorCode::SUCCESS;
}

template <typename K, typename V>
MatchErrorCode
MatchUnitGeneric<K, V>::get_entry_(entry_handle_t handle,
                            std::vector<MatchKeyParam> *match_key,
                            const V **value, int *priority) const {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  if (!this->valid_handle(handle_)) return MatchErrorCode::INVALID_HANDLE;
  const Entry &entry = entries[handle_];
  if (HANDLE_VERSION(handle) != entry.key.version)
    return MatchErrorCode::EXPIRED_HANDLE;

  *match_key = this->match_key_builder.entry_to_match_params(entry.key);
  *value = &entry.value;
  if (priority) *priority = get_priority(entry.key);

  return MatchErrorCode::SUCCESS;
}

template <typename K, typename V>
MatchErrorCode
MatchUnitGeneric<K, V>::dump_match_entry_(std::ostream *out,
                                   entry_handle_t handle) const {
  internal_handle_t handle_ = HANDLE_INTERNAL(handle);
  const Entry &entry = entries[handle_];
  if (HANDLE_VERSION(handle) != entry.key.version)
    return MatchErrorCode::EXPIRED_HANDLE;

  *out << "Dumping entry " << handle << "\n";
  this->dump_key_params(
      out, this->match_key_builder.entry_to_match_params(entry.key),
      get_priority(entry.key));
  return MatchErrorCode::SUCCESS;
}

template <typename K, typename V>
void
MatchUnitGeneric<K, V>::reset_state_() {
  lookup_structure->clear();
  entries = std::vector<Entry>(this->size);
}

namespace {

void serialize_key(const ExactMatchKey &key, std::ostream *out) {
  (void) key; (void) out;
}

void serialize_key(const LPMMatchKey &key, std::ostream *out) {
  (*out) << key.prefix_length << "\n";
}

// works for RangeMatchKey as well
void serialize_key(const TernaryMatchKey &key, std::ostream *out) {
  (*out) << key.mask.to_hex() << "\n";
  (*out) << key.priority << "\n";
}

}  // namespace

template <typename K, typename V>
void
MatchUnitGeneric<K, V>::serialize_(std::ostream *out) const {
  (*out) << this->num_entries << "\n";
  for (internal_handle_t handle_ : this->handles) {
    const Entry &entry = entries[handle_];
    // dump entry handle to be able to have the exact same one when
    // deserializing
    (*out) << handle_ << "\n";
    (*out) << entry.key.version << "\n";
    (*out) << entry.key.data.to_hex() << "\n";
    serialize_key(entry.key, out);
    entry.value.serialize(out);
    const EntryMeta &meta = this->entry_meta[handle_];
    (*out) << meta.timeout_ms << "\n";
    // not a runtime configuration, should be serialized anyway?
    // meta.counter.serialize(out);
    assert(meta.version == entry.key.version);
  }
  if (this->direct_meters) this->direct_meters->serialize(out);
}

namespace {

void deserialize_key(ExactMatchKey *key, std::istream *in) {
  (void) key; (void) in;
}

void deserialize_key(LPMMatchKey *key, std::istream *in) {
  (*in) >> key->prefix_length;
}

// works for RangeMatchKey as well
void deserialize_key(TernaryMatchKey *key, std::istream *in) {
  std::string mask_hex; (*in) >> mask_hex;
  key->mask = ByteContainer(mask_hex);
  (*in) >> key->priority;
}

}  // namespace

template <typename K, typename V>
void
MatchUnitGeneric<K, V>::deserialize_(std::istream *in, const P4Objects &objs) {
  (*in) >> this->num_entries;
  for (size_t i = 0; i < this->num_entries; i++) {
    Entry entry;
    internal_handle_t handle_; (*in) >> handle_;
    assert(!this->handles.set_handle(handle_));
    uint32_t version; (*in) >> version;
    std::string key_hex; (*in) >> key_hex;
    entry.key.data = ByteContainer(key_hex);
    deserialize_key(&entry.key, in);
    entry.value.deserialize(in, objs);
    entry.key.version = version;
    entries[handle_] = std::move(entry);
    lookup_structure->add_entry(entry.key, handle_);
    EntryMeta &meta = this->entry_meta[handle_];
    meta.reset();
    meta.version = version;
    (*in) >> meta.timeout_ms;
    // meta.counter.deserialize(in);
  }
  if (this->direct_meters) this->direct_meters->deserialize(in);
}

// explicit template instantiation

// I did not think I had to explicitly instantiate MatchUnitAbstract, because it
// is a base class for the others, but I get an linker error if I don't
template class
MatchUnitAbstract<MatchTableAbstract::ActionEntry>;
template class
MatchUnitAbstract<MatchTableIndirect::IndirectIndex>;

// The following are all instantiations of MatchUnitGeneric, based on the
// aliases created in match_units.h
template class
MatchUnitGeneric<ExactMatchKey, MatchTableAbstract::ActionEntry>;
template class
MatchUnitGeneric<ExactMatchKey, MatchTableIndirect::IndirectIndex>;

template class
MatchUnitGeneric<LPMMatchKey, MatchTableAbstract::ActionEntry>;
template class
MatchUnitGeneric<LPMMatchKey, MatchTableIndirect::IndirectIndex>;

template class
MatchUnitGeneric<TernaryMatchKey, MatchTableAbstract::ActionEntry>;
template class
MatchUnitGeneric<TernaryMatchKey, MatchTableIndirect::IndirectIndex>;

template class
MatchUnitGeneric<RangeMatchKey, MatchTableAbstract::ActionEntry>;
template class
MatchUnitGeneric<RangeMatchKey, MatchTableIndirect::IndirectIndex>;

}  // namespace bm
