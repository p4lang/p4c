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

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/switch.h>

#include <PI/int/pi_int.h>
#include <PI/int/serialize.h>
#include <PI/p4info.h>
#include <PI/pi.h>
#include <PI/target/pi_tables_imp.h>

#include <algorithm>
#include <iterator>  // std::distance
#include <limits>
#include <string>
#include <vector>

#include <cstring>

#include "action_helpers.h"
#include "direct_res_spec.h"
#include "common.h"

namespace {

// We check which of pi_priority_t (PI type) and int (bmv2 type) can fit the
// largest unsigned integer. If it is priority_t, BM_MAX_PRIORITY is set to the
// max value for an int. If it is int, BM_MAX_PRIORITY is set to the max value
// for a priority_t. BM_MAX_PRIORITY is then used as a pivot to invert priority
// values passed by PI.
static constexpr pi_priority_t BM_MAX_PRIORITY =
    (static_cast<uintmax_t>(std::numeric_limits<pi_priority_t>::max()) >=
     static_cast<uintmax_t>(std::numeric_limits<int>::max())) ?
    static_cast<pi_priority_t>(std::numeric_limits<int>::max()) :
    std::numeric_limits<pi_priority_t>::max();

class PriorityInverter {
 public:
  PriorityInverter() = delete;
  static int pi_to_bm(pi_priority_t from) {
    assert(from <= BM_MAX_PRIORITY);
    return BM_MAX_PRIORITY - from;
  }
  static pi_priority_t bm_to_pi(int from) {
    assert(from >= 0 && static_cast<uintmax_t>(from) <= BM_MAX_PRIORITY);
    return BM_MAX_PRIORITY - static_cast<pi_priority_t>(from);
  }
};

std::vector<bm::MatchKeyParam> build_key(pi_p4_id_t table_id,
                                         const pi_match_key_t *match_key,
                                         const pi_p4info_t *p4info,
                                         int *priority) {
  std::vector<bm::MatchKeyParam> key;
  *priority = 0;

  const char *mk_data = match_key->data;

  size_t num_match_fields = pi_p4info_table_num_match_fields(p4info, table_id);
  key.reserve(num_match_fields);
  for (size_t i = 0; i < num_match_fields; i++) {
    auto finfo = pi_p4info_table_match_field_info(p4info, table_id, i);
    size_t f_bw = finfo->bitwidth;
    size_t nbytes = (f_bw + 7) / 8;

    switch (finfo->match_type) {
      case PI_P4INFO_MATCH_TYPE_VALID:
        key.emplace_back(
            bm::MatchKeyParam::Type::VALID,
            (*mk_data != 0) ? std::string("\x01", 1) : std::string("\x00", 1));
        mk_data++;
        break;
      case PI_P4INFO_MATCH_TYPE_EXACT:
        key.emplace_back(bm::MatchKeyParam::Type::EXACT,
                         std::string(mk_data, nbytes));
        mk_data += nbytes;
        break;
      case PI_P4INFO_MATCH_TYPE_LPM:
        {
          std::string k(mk_data, nbytes);
          mk_data += nbytes;
          uint32_t pLen;
          mk_data += retrieve_uint32(mk_data, &pLen);
          key.emplace_back(bm::MatchKeyParam::Type::LPM, k, pLen);
        }
        break;
      case PI_P4INFO_MATCH_TYPE_TERNARY:
      case PI_P4INFO_MATCH_TYPE_OPTIONAL:
        {
          std::string k(mk_data, nbytes);
          mk_data += nbytes;
          std::string mask(mk_data, nbytes);
          mk_data += nbytes;
          key.emplace_back(bm::MatchKeyParam::Type::TERNARY, k, mask);
        }
        *priority = PriorityInverter::pi_to_bm(match_key->priority);
        break;
      case PI_P4INFO_MATCH_TYPE_RANGE:
        {
          std::string start(mk_data, nbytes);
          mk_data += nbytes;
          std::string end(mk_data, nbytes);
          mk_data += nbytes;
          key.emplace_back(bm::MatchKeyParam::Type::RANGE, start, end);
        }
        *priority = PriorityInverter::pi_to_bm(match_key->priority);
        break;
      default:
        assert(0);
    }
  }

  return key;
}

class bm_exception : public std::exception {
 public:
  explicit bm_exception(bm::MatchErrorCode error_code)
      : error_code(error_code) { }

  const char* what() const noexcept override {
    return "bm_exception";
  }

  pi_status_t get() const noexcept {
    return pibmv2::convert_error_code(error_code);
  }

 private:
  bm::MatchErrorCode error_code;
};

void set_ttl(const std::string &t_name,
             const pi_table_entry_t *table_entry,
             pi_entry_handle_t entry_handle) {
  auto *properties = table_entry->entry_properties;
  if (pi_entry_properties_is_set(properties, PI_ENTRY_PROPERTY_TYPE_TTL)) {
    auto error_code = pibmv2::switch_->mt_set_entry_ttl(
        0, t_name, entry_handle,
        static_cast<unsigned int>(properties->ttl_ns / 1000000));
    if (error_code != bm::MatchErrorCode::SUCCESS)
      throw bm_exception(error_code);
  }
}

void set_indirect_ttl(const std::string &t_name,
                      const pi_table_entry_t *table_entry,
                      pi_entry_handle_t entry_handle) {
  auto *properties = table_entry->entry_properties;
  if (pi_entry_properties_is_set(properties, PI_ENTRY_PROPERTY_TYPE_TTL)) {
    auto error_code = pibmv2::switch_->mt_indirect_set_entry_ttl(
        0, t_name, entry_handle,
        static_cast<unsigned int>(properties->ttl_ns / 1000000));
    if (error_code != bm::MatchErrorCode::SUCCESS)
      throw bm_exception(error_code);
  }
}

pi_entry_handle_t add_entry(const pi_p4info_t *p4info,
                            pi_dev_tgt_t dev_tgt,
                            const std::string &t_name,
                            std::vector<bm::MatchKeyParam> &&match_key,
                            int priority,
                            const pi_table_entry_t *table_entry) {
  _BM_UNUSED(dev_tgt);
  const pi_action_data_t *adata = table_entry->entry.action_data;
  auto action_data = pibmv2::build_action_data(adata, p4info);
  pi_p4_id_t action_id = adata->action_id;
  std::string a_name(pi_p4info_action_name_from_id(p4info, action_id));
  bm::entry_handle_t entry_handle;
  auto error_code = pibmv2::switch_->mt_add_entry(
      0, t_name, std::move(match_key), a_name,
      std::move(action_data), &entry_handle, priority);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
  set_ttl(t_name, table_entry, entry_handle);
  return static_cast<pi_entry_handle_t>(entry_handle);
}

pi_entry_handle_t add_indirect_entry(const pi_p4info_t *p4info,
                                     pi_dev_tgt_t dev_tgt,
                                     const std::string &t_name,
                                     std::vector<bm::MatchKeyParam> &&match_key,
                                     int priority,
                                     const pi_table_entry_t *table_entry) {
  _BM_UNUSED(p4info);  // needed later?
  _BM_UNUSED(dev_tgt);
  pi_indirect_handle_t h = table_entry->entry.indirect_handle;
  bm::entry_handle_t entry_handle;
  bool is_grp_h = pibmv2::IndirectHMgr::is_grp_h(h);
  bm::MatchErrorCode error_code;
  if (!is_grp_h) {
    error_code = pibmv2::switch_->mt_indirect_add_entry(
        0, t_name, std::move(match_key), h, &entry_handle, priority);
  } else {
    h = pibmv2::IndirectHMgr::clear_grp_h(h);
    error_code = pibmv2::switch_->mt_indirect_ws_add_entry(
        0, t_name, std::move(match_key), h, &entry_handle, priority);
  }
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
  set_indirect_ttl(t_name, table_entry, entry_handle);
  return static_cast<pi_entry_handle_t>(entry_handle);
}

void set_default_entry(const pi_p4info_t *p4info,
                       pi_dev_tgt_t dev_tgt,
                       const std::string &t_name,
                       const pi_action_data_t *adata) {
  _BM_UNUSED(dev_tgt);
  auto action_data = pibmv2::build_action_data(adata, p4info);
  pi_p4_id_t action_id = adata->action_id;
  std::string a_name(pi_p4info_action_name_from_id(p4info, action_id));
  auto error_code = pibmv2::switch_->mt_set_default_action(
      0, t_name, a_name, std::move(action_data));
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
}

void set_default_indirect_entry(const pi_p4info_t *p4info,
                                pi_dev_tgt_t dev_tgt,
                                const std::string &t_name,
                                pi_indirect_handle_t h) {
  _BM_UNUSED(p4info);  // needed later?
  _BM_UNUSED(dev_tgt);
  bool is_grp_h = pibmv2::IndirectHMgr::is_grp_h(h);
  bm::MatchErrorCode error_code;
  if (!is_grp_h) {
    error_code = pibmv2::switch_->mt_indirect_set_default_member(
        0, t_name, h);
  } else {
    h = pibmv2::IndirectHMgr::clear_grp_h(h);
    error_code = pibmv2::switch_->mt_indirect_ws_set_default_group(
        0, t_name, h);
  }
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
}

void modify_entry(const pi_p4info_t *p4info,
                  pi_dev_id_t dev_id,
                  const std::string &t_name,
                  pi_entry_handle_t entry_handle,
                  const pi_action_data_t *adata) {
  _BM_UNUSED(dev_id);
  auto action_data = pibmv2::build_action_data(adata, p4info);
  pi_p4_id_t action_id = adata->action_id;
  std::string a_name(pi_p4info_action_name_from_id(p4info, action_id));
  auto error_code = pibmv2::switch_->mt_modify_entry(
      0, t_name, entry_handle, a_name, std::move(action_data));
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
}

void modify_indirect_entry(const pi_p4info_t *p4info,
                           pi_dev_id_t dev_id,
                           const std::string &t_name,
                           pi_entry_handle_t entry_handle,
                           pi_indirect_handle_t h) {
  _BM_UNUSED(p4info);  // needed later?
  _BM_UNUSED(dev_id);
  bool is_grp_h = pibmv2::IndirectHMgr::is_grp_h(h);
  bm::MatchErrorCode error_code;
  if (!is_grp_h) {
    error_code = pibmv2::switch_->mt_indirect_modify_entry(
        0, t_name, entry_handle, h);
  } else {
    h = pibmv2::IndirectHMgr::clear_grp_h(h);
    error_code = pibmv2::switch_->mt_indirect_ws_modify_entry(
        0, t_name, entry_handle, h);
  }
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
}

template <typename E>
void build_action_entry(const pi_p4info_t *p4info, const E &entry,
                        pi_table_entry_t *table_entry);

template <>
void build_action_entry<bm::MatchTable::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTable::Entry &entry,
    pi_table_entry_t *table_entry) {
  table_entry->entry_type = PI_ACTION_ENTRY_TYPE_DATA;

  const pi_p4_id_t action_id = pi_p4info_action_id_from_name(
      p4info, entry.action_fn->get_name().c_str());

  const auto adata_size = pi_p4info_action_data_size(p4info, action_id);
  // no alignment issue with new[]
  char *data_ = new char[sizeof(pi_action_data_t) + adata_size];
  pi_action_data_t *adata = reinterpret_cast<pi_action_data_t *>(data_);
  data_ += sizeof(pi_action_data_t);
  adata->p4info = p4info;
  adata->action_id = action_id;
  adata->data_size = adata_size;
  adata->data = data_;
  table_entry->entry.action_data = adata;
  data_ = pibmv2::dump_action_data(p4info, data_, action_id, entry.action_data);
}

template <>
void build_action_entry<bm::MatchTableIndirect::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTableIndirect::Entry &entry,
    pi_table_entry_t *table_entry) {
  _BM_UNUSED(p4info);
  table_entry->entry_type = PI_ACTION_ENTRY_TYPE_INDIRECT;
  auto indirect_handle = static_cast<pi_indirect_handle_t>(entry.mbr);
  table_entry->entry.indirect_handle = indirect_handle;
}

template <>
void build_action_entry<bm::MatchTableIndirectWS::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTableIndirectWS::Entry &entry,
    pi_table_entry_t *table_entry) {
  _BM_UNUSED(p4info);
  table_entry->entry_type = PI_ACTION_ENTRY_TYPE_INDIRECT;
  pi_indirect_handle_t indirect_handle;
  if (entry.mbr < entry.grp) {
    indirect_handle = static_cast<pi_indirect_handle_t>(entry.mbr);
  } else {
    indirect_handle = static_cast<pi_indirect_handle_t>(entry.grp);
    indirect_handle = pibmv2::IndirectHMgr::make_grp_h(indirect_handle);
  }
  table_entry->entry.indirect_handle = indirect_handle;
}

template <typename M,
          bm::MatchErrorCode (bm::RuntimeInterface::*GetFn)(
              bm::cxt_id_t, const std::string &, typename M::Entry *) const>
typename M::Entry get_default_entry_common(
    const pi_p4info_t *p4info, const std::string &t_name,
    pi_table_entry_t *table_entry) {
  typename M::Entry entry;
  auto error_code = std::bind(GetFn, pibmv2::switch_, 0, t_name, &entry)();
  if (error_code == bm::MatchErrorCode::NO_DEFAULT_ENTRY) {
    table_entry->entry_type = PI_ACTION_ENTRY_TYPE_NONE;
  } else if (error_code != bm::MatchErrorCode::SUCCESS) {
    throw bm_exception(error_code);
  } else {
    build_action_entry(p4info, entry, table_entry);
  }
  table_entry->entry_properties = nullptr;
  // TODO(antonin): bmv2 currently does not support direct resources for default
  // entries
  table_entry->direct_res_config = nullptr;
  return entry;
}

void get_default_entry(const pi_p4info_t *p4info, const std::string &t_name,
                       pi_table_entry_t *table_entry) {
  switch (pibmv2::switch_->mt_get_type(0, t_name)) {
    case bm::MatchTableType::NONE:
      throw bm_exception(bm::MatchErrorCode::INVALID_TABLE_NAME);
    case bm::MatchTableType::SIMPLE:
      get_default_entry_common<
        bm::MatchTable, &bm::RuntimeInterface::mt_get_default_entry>(
            p4info, t_name, table_entry);
      break;
    case bm::MatchTableType::INDIRECT:
      get_default_entry_common<
        bm::MatchTableIndirect,
        &bm::RuntimeInterface::mt_indirect_get_default_entry>(
            p4info, t_name, table_entry);
      break;
    case bm::MatchTableType::INDIRECT_WS:
      get_default_entry_common<
        bm::MatchTableIndirectWS,
        &bm::RuntimeInterface::mt_indirect_ws_get_default_entry>(
            p4info, t_name, table_entry);
      break;
  }
}

using pibmv2::Buffer;

// The difference with build_action_entry is the output format.
template <typename E>
void build_action_entry_2(const pi_p4info_t *p4info, const E &entry,
                          Buffer *buffer);

template <>
void build_action_entry_2<bm::MatchTable::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTable::Entry &entry,
    Buffer *buffer) {
  emit_action_entry_type(buffer->extend(sizeof(s_pi_action_entry_type_t)),
                         PI_ACTION_ENTRY_TYPE_DATA);

  const pi_p4_id_t action_id = pi_p4info_action_id_from_name(
      p4info, entry.action_fn->get_name().c_str());

  const auto adata_size = pi_p4info_action_data_size(p4info, action_id);

  emit_p4_id(buffer->extend(sizeof(s_pi_p4_id_t)), action_id);
  emit_uint32(buffer->extend(sizeof(uint32_t)), adata_size);
  if (adata_size > 0) {
    pibmv2::dump_action_data(p4info, buffer->extend(adata_size), action_id,
                             entry.action_data);
  }
}

template <>
void build_action_entry_2<bm::MatchTableIndirect::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTableIndirect::Entry &entry,
    Buffer *buffer) {
  _BM_UNUSED(p4info);
  emit_action_entry_type(buffer->extend(sizeof(s_pi_action_entry_type_t)),
                         PI_ACTION_ENTRY_TYPE_INDIRECT);
  auto indirect_handle = static_cast<pi_indirect_handle_t>(entry.mbr);
  emit_indirect_handle(buffer->extend(sizeof(s_pi_indirect_handle_t)),
                       indirect_handle);
}

template <>
void build_action_entry_2<bm::MatchTableIndirectWS::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTableIndirectWS::Entry &entry,
    Buffer *buffer) {
  _BM_UNUSED(p4info);
  emit_action_entry_type(buffer->extend(sizeof(s_pi_action_entry_type_t)),
                         PI_ACTION_ENTRY_TYPE_INDIRECT);
  pi_indirect_handle_t indirect_handle;
  if (entry.mbr < entry.grp) {
    indirect_handle = static_cast<pi_indirect_handle_t>(entry.mbr);
  } else {
    indirect_handle = static_cast<pi_indirect_handle_t>(entry.grp);
    indirect_handle = pibmv2::IndirectHMgr::make_grp_h(indirect_handle);
  }
  emit_indirect_handle(buffer->extend(sizeof(s_pi_indirect_handle_t)),
                       indirect_handle);
}

void emit_match_key(const std::vector<bm::MatchKeyParam> &match_key,
                    Buffer *buffer) {
  for (const auto &p : match_key) {
    switch (p.type) {
      case bm::MatchKeyParam::Type::EXACT:
        std::copy(p.key.begin(), p.key.end(), buffer->extend(p.key.size()));
        break;
      case bm::MatchKeyParam::Type::LPM:
        std::copy(p.key.begin(), p.key.end(), buffer->extend(p.key.size()));
        emit_uint32(buffer->extend(sizeof(uint32_t)), p.prefix_length);
        break;
      case bm::MatchKeyParam::Type::TERNARY:
      case bm::MatchKeyParam::Type::RANGE:
        std::copy(p.key.begin(), p.key.end(), buffer->extend(p.key.size()));
        std::copy(p.mask.begin(), p.mask.end(), buffer->extend(p.mask.size()));
        break;
      case bm::MatchKeyParam::Type::VALID:
        *buffer->extend(1) = (p.key == std::string("\x01", 1)) ? 1 : 0;
        break;
    }
  }
}

template <typename It>
void emit_entries(const pi_p4info_t *p4info, pi_p4_id_t table_id,
                  const It first, const It last, pi_table_fetch_res_t *res) {
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  res->num_entries = std::distance(first, last);
  res->mkey_nbytes = pi_p4info_table_match_key_size(p4info, table_id);

  size_t num_direct_resources;
  auto *res_ids = pi_p4info_table_get_direct_resources(
      p4info, table_id, &num_direct_resources);
  // in bmv2, a table can have at most one direct counter and one direct meter
  pi_p4_id_t counter_id = PI_INVALID_ID, meter_id = PI_INVALID_ID;
  for (size_t i = 0; i < num_direct_resources; i++) {
    if (pi_is_direct_counter_id(res_ids[i])) counter_id = res_ids[i];
    if (pi_is_direct_meter_id(res_ids[i])) meter_id = res_ids[i];
  }

  Buffer buffer;
  for (auto it = first; it != last; it++) {
    const auto &e = *it;
    emit_entry_handle(buffer.extend(sizeof(s_pi_entry_handle_t)), e.handle);
    // TODO(antonin): temporary hack; for match types which do not require a
    // priority, bmv2 returns -1, but the PI tends to expect 0, which is a
    // problem for looking up entry state in the PI software. A better
    // solution would be to ignore this value in the PI based on the key match
    // type.
    int priority = (e.priority == -1) ?
        0 : PriorityInverter::bm_to_pi(e.priority);
    emit_uint32(buffer.extend(sizeof(uint32_t)), priority);
    emit_match_key(e.match_key, &buffer);

    build_action_entry_2(p4info, e, &buffer);

    // properties
    // TODO(antonin): add TTL
    emit_uint32(buffer.extend(sizeof(uint32_t)), 0);

    // direct resources
    // the entry handle may not be valid any more by the time we query the
    // direct resource configs, so we try to handle this case gracefully by not
    // including the config in the buffer. A better solution may be to modify
    // the bmv2 get_entries method to include the direct resource configs, but I
    // am reluctant to change the interface at this stage.
    {
      bool valid_counter = false;
      uint64_t bytes, packets;
      if (counter_id != PI_INVALID_ID) {
        auto error_code = pibmv2::switch_->mt_read_counters(
            0, t_name, e.handle, &bytes, &packets);
        valid_counter = (error_code == bm::MatchErrorCode::SUCCESS);
      }
      bool valid_meter = false;
      std::vector<bm::Meter::rate_config_t> rates;
      if (meter_id != PI_INVALID_ID) {
        auto error_code = pibmv2::switch_->mt_get_meter_rates(
            0, t_name, e.handle, &rates);
        valid_meter = (error_code == bm::MatchErrorCode::SUCCESS);
      }
      size_t valid_direct = (valid_counter ? 1 : 0) + (valid_meter ? 1 : 0);
      emit_uint32(buffer.extend(sizeof(uint32_t)), valid_direct);

      if (valid_counter) {
        PIDirectResMsgSizeFn msg_size_fn;
        PIDirectResEmitFn emit_fn;
        pi_direct_res_get_fns(
            PI_DIRECT_COUNTER_ID, &msg_size_fn, &emit_fn, NULL, NULL);
        emit_p4_id(buffer.extend(sizeof(s_pi_p4_id_t)), counter_id);
        pi_counter_data_t counter_data;
        pibmv2::convert_to_counter_data(&counter_data, bytes, packets);
        auto msg_size = msg_size_fn(&counter_data);
        emit_uint32(buffer.extend(sizeof(uint32_t)), msg_size);
        emit_fn(buffer.extend(msg_size), &counter_data);
      }
      if (valid_meter) {
        PIDirectResMsgSizeFn msg_size_fn;
        PIDirectResEmitFn emit_fn;
        pi_direct_res_get_fns(
            PI_DIRECT_METER_ID, &msg_size_fn, &emit_fn, NULL, NULL);
        emit_p4_id(buffer.extend(sizeof(s_pi_p4_id_t)), meter_id);
        pi_meter_spec_t meter_spec;
        pibmv2::convert_to_meter_spec(p4info, meter_id, &meter_spec, rates);
        auto msg_size = msg_size_fn(&meter_spec);
        emit_uint32(buffer.extend(sizeof(uint32_t)), msg_size);
        emit_fn(buffer.extend(msg_size), &meter_spec);
      }
    }
  }

  res->entries_size = buffer.size();
  res->entries = buffer.copy();
}

template <
  typename M,
  typename std::vector<typename M::Entry> (bm::RuntimeInterface::*GetFn)(
      bm::cxt_id_t, const std::string &) const>
void get_entries_common(const pi_p4info_t *p4info, pi_p4_id_t table_id,
                        pi_table_fetch_res_t *res) {
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));
  const auto entries = std::bind(GetFn, pibmv2::switch_, 0, t_name)();
  emit_entries(p4info, table_id, entries.begin(), entries.end(), res);
}

void get_entries(const pi_p4info_t *p4info, pi_p4_id_t table_id,
                 pi_table_fetch_res_t *res) {
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  switch (pibmv2::switch_->mt_get_type(0, t_name)) {
    case bm::MatchTableType::NONE:
      throw bm_exception(bm::MatchErrorCode::INVALID_TABLE_NAME);
    case bm::MatchTableType::SIMPLE:
      get_entries_common<bm::MatchTable, &bm::RuntimeInterface::mt_get_entries>(
          p4info, table_id, res);
      break;
    case bm::MatchTableType::INDIRECT:
      get_entries_common<bm::MatchTableIndirect,
                         &bm::RuntimeInterface::mt_indirect_get_entries>(
                             p4info, table_id, res);
      break;
    case bm::MatchTableType::INDIRECT_WS:
      get_entries_common<bm::MatchTableIndirectWS,
                         &bm::RuntimeInterface::mt_indirect_ws_get_entries>(
                             p4info, table_id, res);
      break;
  }
}

void get_entry(const pi_p4info_t *p4info, pi_p4_id_t table_id,
               pi_entry_handle_t handle, pi_table_fetch_res_t *res) {
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  switch (pibmv2::switch_->mt_get_type(0, t_name)) {
    case bm::MatchTableType::NONE:
      throw bm_exception(bm::MatchErrorCode::INVALID_TABLE_NAME);
    case bm::MatchTableType::SIMPLE: {
      bm::MatchTable::Entry entry;
      auto error_code = pibmv2::switch_->mt_get_entry(
          0, t_name, handle, &entry);
      if (error_code != bm::MatchErrorCode::SUCCESS)
        throw bm_exception(error_code);
      emit_entries(p4info, table_id, &entry, &entry + 1, res);
      break;
    }
    case bm::MatchTableType::INDIRECT: {
      bm::MatchTableIndirect::Entry entry;
      auto error_code = pibmv2::switch_->mt_indirect_get_entry(
          0, t_name, handle, &entry);
      if (error_code != bm::MatchErrorCode::SUCCESS)
        throw bm_exception(error_code);
      emit_entries(p4info, table_id, &entry, &entry + 1, res);
      break;
    }
    case bm::MatchTableType::INDIRECT_WS: {
      bm::MatchTableIndirectWS::Entry entry;
      auto error_code = pibmv2::switch_->mt_indirect_ws_get_entry(
          0, t_name, handle, &entry);
      if (error_code != bm::MatchErrorCode::SUCCESS)
        throw bm_exception(error_code);
      emit_entries(p4info, table_id, &entry, &entry + 1, res);
      break;
    }
  }
}

bm::MatchTableAbstract::EntryCommon get_match_entry(
    const pi_p4info_t *p4info, pi_p4_id_t table_id, pi_entry_handle_t handle) {
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  switch (pibmv2::switch_->mt_get_type(0, t_name)) {
    case bm::MatchTableType::NONE:
      throw bm_exception(bm::MatchErrorCode::INVALID_TABLE_NAME);
    case bm::MatchTableType::SIMPLE: {
      bm::MatchTable::Entry entry;
      auto error_code = pibmv2::switch_->mt_get_entry(
          0, t_name, handle, &entry);
      if (error_code != bm::MatchErrorCode::SUCCESS)
        throw bm_exception(error_code);
      return entry;
    }
    case bm::MatchTableType::INDIRECT: {
      bm::MatchTableIndirect::Entry entry;
      auto error_code = pibmv2::switch_->mt_indirect_get_entry(
          0, t_name, handle, &entry);
      if (error_code != bm::MatchErrorCode::SUCCESS)
        throw bm_exception(error_code);
      return entry;
    }
    case bm::MatchTableType::INDIRECT_WS: {
      bm::MatchTableIndirectWS::Entry entry;
      auto error_code = pibmv2::switch_->mt_indirect_ws_get_entry(
          0, t_name, handle, &entry);
      if (error_code != bm::MatchErrorCode::SUCCESS)
        throw bm_exception(error_code);
      return entry;
    }
  }
  return {};
}

void set_direct_resources(const pi_p4info_t *p4info, pi_dev_id_t dev_id,
                          const std::string &t_name,
                          pi_entry_handle_t entry_handle,
                          const pi_direct_res_config_t *direct_res_config) {
  _BM_UNUSED(p4info);
  _BM_UNUSED(dev_id);
  if (!direct_res_config) return;
  for (size_t i = 0; i < direct_res_config->num_configs; i++) {
    pi_direct_res_config_one_t *config = &direct_res_config->configs[i];
    pi_res_type_id_t type = PI_GET_TYPE_ID(config->res_id);
    bm::MatchErrorCode error_code;
    switch (type) {
      case PI_DIRECT_COUNTER_ID:
        {
          uint64_t bytes, packets;
          pibmv2::convert_from_counter_data(
              reinterpret_cast<pi_counter_data_t *>(config->config),
              &bytes, &packets);
          error_code = pibmv2::switch_->mt_write_counters(
              0, t_name, entry_handle, bytes, packets);
        }
        break;
      case PI_DIRECT_METER_ID:
        {
          auto rates = pibmv2::convert_from_meter_spec(
              reinterpret_cast<pi_meter_spec_t *>(config->config));
          error_code = pibmv2::switch_->mt_set_meter_rates(
              0, t_name, entry_handle, rates);
        }
        break;
      default:  // TODO(antonin): what to do?
        assert(0);
        return;
    }
    if (error_code != bm::MatchErrorCode::SUCCESS)
      throw bm_exception(error_code);
  }
}

template <typename M,
          bm::MatchErrorCode (bm::RuntimeInterface::*GetFn)(
              bm::cxt_id_t, const std::string &,
              const std::vector<bm::MatchKeyParam> &,
              typename M::Entry *, int) const>
pi_entry_handle_t get_entry_handle_from_key_common(
    pi_dev_id_t dev_id, const pi_p4info_t *p4info, pi_p4_id_t table_id,
    const pi_match_key_t *match_key) {
  _BM_UNUSED(dev_id);
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));
  int priority;
  auto key = build_key(table_id, match_key, p4info, &priority);
  typename M::Entry entry;
  auto error_code = std::bind(
      GetFn, pibmv2::switch_, 0, t_name, key, &entry, priority)();
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
  return entry.handle;
}

pi_entry_handle_t get_entry_handle_from_key(
    pi_dev_id_t dev_id, pi_p4_id_t table_id, const pi_match_key_t *match_key) {
  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));
  switch (pibmv2::switch_->mt_get_type(0, t_name)) {
    case bm::MatchTableType::NONE:
      throw bm_exception(bm::MatchErrorCode::INVALID_TABLE_NAME);
    case bm::MatchTableType::SIMPLE:
      return get_entry_handle_from_key_common<
        bm::MatchTable, &bm::RuntimeInterface::mt_get_entry_from_key>(
            dev_id, p4info, table_id, match_key);
    case bm::MatchTableType::INDIRECT:
      return get_entry_handle_from_key_common<
        bm::MatchTableIndirect,
        &bm::RuntimeInterface::mt_indirect_get_entry_from_key>(
            dev_id, p4info, table_id, match_key);
    case bm::MatchTableType::INDIRECT_WS:
      return get_entry_handle_from_key_common<
        bm::MatchTableIndirectWS,
        &bm::RuntimeInterface::mt_indirect_ws_get_entry_from_key>(
            dev_id, p4info, table_id, match_key);
  }
  assert(0);  // unreachable
  return 0;
}

}  // namespace


extern "C" {

pi_status_t _pi_table_entry_add(pi_session_handle_t session_handle,
                                pi_dev_tgt_t dev_tgt,
                                pi_p4_id_t table_id,
                                const pi_match_key_t *match_key,
                                const pi_table_entry_t *table_entry,
                                int overwrite,
                                pi_entry_handle_t *entry_handle) {
  _BM_UNUSED(overwrite);  // TODO(antonin)
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);

  if (match_key->priority > BM_MAX_PRIORITY)
    return PI_STATUS_UNSUPPORTED_ENTRY_PRIORITY;
  int priority;
  auto mkey = build_key(table_id, match_key, p4info, &priority);

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  // TODO(antonin): entry timeout
  try {
    switch (table_entry->entry_type) {
      case PI_ACTION_ENTRY_TYPE_DATA:
        *entry_handle = add_entry(
            p4info, dev_tgt, t_name, std::move(mkey), priority, table_entry);
        break;
      case PI_ACTION_ENTRY_TYPE_INDIRECT:
        *entry_handle = add_indirect_entry(
            p4info, dev_tgt, t_name, std::move(mkey), priority, table_entry);
        break;
      default:
        assert(0);
    }
    // direct resources
    set_direct_resources(p4info, dev_tgt.dev_id, t_name, *entry_handle,
                         table_entry->direct_res_config);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_set(pi_session_handle_t session_handle,
                                         pi_dev_tgt_t dev_tgt,
                                         pi_p4_id_t table_id,
                                         const pi_table_entry_t *table_entry) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  try {
    if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA) {
      const pi_action_data_t *adata = table_entry->entry.action_data;

      // TODO(antonin): equivalent for indirect?
      // TODO(antonin): move to common PI code?

      if (pi_p4info_table_has_const_default_action(p4info, table_id)) {
        bool has_mutable_action_params;
        auto default_action_id = pi_p4info_table_get_const_default_action(
            p4info, table_id, &has_mutable_action_params);
        if (default_action_id != adata->action_id)
          return PI_STATUS_CONST_DEFAULT_ACTION;
        (void)has_mutable_action_params;
      }

      set_default_entry(p4info, dev_tgt, t_name, adata);
    } else if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_INDIRECT) {
      set_default_indirect_entry(p4info, dev_tgt, t_name,
                                 table_entry->entry.indirect_handle);
    } else {
      assert(0);
    }
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_reset(pi_session_handle_t session_handle,
                                           pi_dev_tgt_t dev_tgt,
                                           pi_p4_id_t table_id) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  bm::MatchErrorCode error_code = bm::MatchErrorCode::SUCCESS;
  switch (pibmv2::switch_->mt_get_type(0, t_name)) {
    case bm::MatchTableType::NONE:
      error_code = bm::MatchErrorCode::INVALID_TABLE_NAME;
      break;
    case bm::MatchTableType::SIMPLE:
      error_code = pibmv2::switch_->mt_reset_default_entry(0, t_name);
      break;
    case bm::MatchTableType::INDIRECT:
    case bm::MatchTableType::INDIRECT_WS:
      error_code = pibmv2::switch_->mt_indirect_reset_default_entry(0, t_name);
      break;
  }

  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_get(pi_session_handle_t session_handle,
                                         pi_dev_tgt_t dev_tgt,
                                         pi_p4_id_t table_id,
                                         pi_table_entry_t *table_entry) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  try {
    get_default_entry(p4info, t_name, table_entry);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_done(pi_session_handle_t session_handle,
                                          pi_table_entry_t *table_entry) {
  _BM_UNUSED(session_handle);

  if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA) {
    pi_action_data_t *action_data = table_entry->entry.action_data;
    if (action_data) delete[] action_data;
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_get_handle(
    pi_session_handle_t session_handle, pi_dev_tgt_t dev_tgt,
    pi_p4_id_t table_id, pi_entry_handle_t *entry_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_tgt);
  _BM_UNUSED(table_id);
  *entry_handle = pibmv2::get_default_handle();
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entry_delete(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   pi_entry_handle_t entry_handle) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));
  auto ap_id = pi_p4info_table_get_implementation(p4info, table_id);

  auto error_code = (ap_id == PI_INVALID_ID) ?
      pibmv2::switch_->mt_delete_entry(0, t_name, entry_handle) :
      pibmv2::switch_->mt_indirect_delete_entry(0, t_name, entry_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

// for the _wkey functions (delete and modify), we first retrieve the handle,
// then call the "usual" method. We release the Thrift session lock in between
// the 2, which may not be ideal. This can be improved later if needed.

pi_status_t _pi_table_entry_delete_wkey(pi_session_handle_t session_handle,
                                        pi_dev_tgt_t dev_tgt,
                                        pi_p4_id_t table_id,
                                        const pi_match_key_t *match_key) {
  if (match_key->priority > BM_MAX_PRIORITY)
    return PI_STATUS_UNSUPPORTED_ENTRY_PRIORITY;
  try {
    auto h = get_entry_handle_from_key(dev_tgt.dev_id, table_id, match_key);
    return _pi_table_entry_delete(session_handle, dev_tgt.dev_id, table_id, h);
  } catch (const bm_exception &e) {
    return e.get();
  }
}

pi_status_t _pi_table_entry_modify(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   pi_entry_handle_t entry_handle,
                                   const pi_table_entry_t *table_entry) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  try {
    if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA) {
      modify_entry(p4info, dev_id, t_name, entry_handle,
                   table_entry->entry.action_data);
      set_ttl(t_name, table_entry, entry_handle);
    } else if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_INDIRECT) {
      modify_indirect_entry(p4info, dev_id, t_name, entry_handle,
                            table_entry->entry.indirect_handle);
      set_indirect_ttl(t_name, table_entry, entry_handle);
    } else {
      assert(0);
    }
    set_direct_resources(p4info, dev_id, t_name, entry_handle,
                         table_entry->direct_res_config);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entry_modify_wkey(pi_session_handle_t session_handle,
                                        pi_dev_tgt_t dev_tgt,
                                        pi_p4_id_t table_id,
                                        const pi_match_key_t *match_key,
                                        const pi_table_entry_t *table_entry) {
  if (match_key->priority > BM_MAX_PRIORITY)
    return PI_STATUS_UNSUPPORTED_ENTRY_PRIORITY;
  try {
    auto h = get_entry_handle_from_key(dev_tgt.dev_id, table_id, match_key);
    return _pi_table_entry_modify(session_handle, dev_tgt.dev_id, table_id, h,
                                  table_entry);
  } catch (const bm_exception &e) {
    return e.get();
  }
}

pi_status_t _pi_table_entries_fetch(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t table_id,
                                    pi_table_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);

  try {
    get_entries(p4info, table_id, res);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entries_fetch_one(pi_session_handle_t session_handle,
                                        pi_dev_id_t dev_id, pi_p4_id_t table_id,
                                        pi_entry_handle_t entry_handle,
                                        pi_table_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);

  try {
    get_entry(p4info, table_id, entry_handle, res);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entries_fetch_wkey(pi_session_handle_t session_handle,
                                         pi_dev_tgt_t dev_tgt,
                                         pi_p4_id_t table_id,
                                         const pi_match_key_t *match_key,
                                         pi_table_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);

  pi_entry_handle_t entry_handle;
  try {
    entry_handle = get_entry_handle_from_key(
        dev_tgt.dev_id, table_id, match_key);
  } catch (const bm_exception &e) {
    // match key does not correspond to any entry
    res->num_entries = 0;
    return PI_STATUS_SUCCESS;
  }

  try {
    get_entry(p4info, table_id, entry_handle, res);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entries_fetch_done(pi_session_handle_t session_handle,
                                         pi_table_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  if (res->num_entries > 0) delete[] res->entries;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_idle_timeout_config_set(
    pi_session_handle_t session_handle, pi_dev_id_t dev_id, pi_p4_id_t table_id,
    const pi_idle_timeout_config_t *config) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  _BM_UNUSED(table_id);
  _BM_UNUSED(config);
  // The PI library is expected to check that idle timeout is enabled on the
  // table.

  // Do not do anything at the moment. The ageing sweep_interval for bmv2 is
  // global and not per table, so it would be too much trouble to maintain a
  // per-table map here and use the minimum value.
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entry_get_remaining_ttl(
    pi_session_handle_t session_handle, pi_dev_id_t dev_id, pi_p4_id_t table_id,
    pi_entry_handle_t entry_handle, uint64_t *ttl_ns) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);

  // The PI library is expected to check that idle timeout is enabled on the
  // table.

  try {
    auto entry = get_match_entry(p4info, table_id, entry_handle);
    if (entry.timeout_ms == 0) {
      *ttl_ns = std::numeric_limits<uint64_t>::max();
    } else {
      *ttl_ns = (entry.timeout_ms > entry.time_since_hit_ms) ?
          ((entry.timeout_ms - entry.time_since_hit_ms) * 1000 * 1000) : 0;
    }
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

}

namespace bm {

namespace pi {

pi_status_t table_idle_timeout_notify(pi_dev_id_t dev_id, pi_p4_id_t table_id,
                                      pi_entry_handle_t entry_handle) {
  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);

  pi_match_key_t match_key;
  match_key.p4info = p4info;
  match_key.table_id = table_id;
  Buffer buffer(128);
  try {
    auto entry = get_match_entry(p4info, table_id, entry_handle);
    emit_match_key(entry.match_key, &buffer);
    match_key.priority = (entry.priority == -1) ?
        0 : PriorityInverter::bm_to_pi(entry.priority);
    match_key.data_size = buffer.size();
    // non-owning pointer, the application must make a copy if needed before the
    // callback returns
    match_key.data = buffer.data();
    return pi_table_idle_timeout_notify(
        dev_id, table_id, &match_key, entry_handle);
  } catch (const bm_exception &e) {
    return e.get();
  }
}

}  // namespace pi

}  // namespace bm
