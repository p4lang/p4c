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

#include <bm/bm_sim/switch.h>

#include <PI/int/pi_int.h>
#include <PI/int/serialize.h>
#include <PI/p4info.h>
#include <PI/pi.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <cstring>

#include "action_helpers.h"
#include "direct_res_spec.h"
#include "common.h"

namespace {

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
    pi_p4info_match_field_info_t finfo;
    pi_p4info_table_match_field_info(p4info, table_id, i, &finfo);
    size_t f_bw = finfo.bitwidth;
    size_t nbytes = (f_bw + 7) / 8;

    switch (finfo.match_type) {
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
        {
          std::string k(mk_data, nbytes);
          mk_data += nbytes;
          std::string mask(mk_data, nbytes);
          mk_data += nbytes;
          key.emplace_back(bm::MatchKeyParam::Type::TERNARY, k, mask);
        }
        *priority = match_key->priority;
        break;
      case PI_P4INFO_MATCH_TYPE_RANGE:
        {
          std::string start(mk_data, nbytes);
          mk_data += nbytes;
          std::string end(mk_data, nbytes);
          mk_data += nbytes;
          key.emplace_back(bm::MatchKeyParam::Type::RANGE, start, end);
        }
        *priority = match_key->priority;
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

pi_entry_handle_t add_entry(const pi_p4info_t *p4info,
                            pi_dev_tgt_t dev_tgt,
                            const std::string &t_name,
                            std::vector<bm::MatchKeyParam> &&match_key,
                            int priority,
                            const pi_action_data_t *adata) {
  (void) dev_tgt;
  auto action_data = pibmv2::build_action_data(adata, p4info);
  pi_p4_id_t action_id = adata->action_id;
  std::string a_name(pi_p4info_action_name_from_id(p4info, action_id));
  bm::entry_handle_t entry_handle;
  auto error_code = pibmv2::switch_->mt_add_entry(
      0, t_name, std::move(match_key), a_name,
      std::move(action_data), &entry_handle, priority);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    throw bm_exception(error_code);
  return static_cast<pi_entry_handle_t>(entry_handle);
}

pi_entry_handle_t add_indirect_entry(const pi_p4info_t *p4info,
                                     pi_dev_tgt_t dev_tgt,
                                     const std::string &t_name,
                                     std::vector<bm::MatchKeyParam> &&match_key,
                                     int priority,
                                     pi_indirect_handle_t h) {
  (void) p4info;  // needed later?
  (void) dev_tgt;
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
  return static_cast<pi_entry_handle_t>(entry_handle);
}

void set_default_entry(const pi_p4info_t *p4info,
                       pi_dev_tgt_t dev_tgt,
                       const std::string &t_name,
                       const pi_action_data_t *adata) {
  (void) dev_tgt;
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
  (void) p4info;  // needed later?
  (void) dev_tgt;
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
  (void)dev_id;
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
  (void) p4info;  // needed later?
  (void) dev_id;
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

  const auto adata_size = get_action_data_size(p4info, action_id);
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
  (void) p4info;
  table_entry->entry_type = PI_ACTION_ENTRY_TYPE_INDIRECT;
  auto indirect_handle = static_cast<pi_indirect_handle_t>(entry.mbr);
  table_entry->entry.indirect_handle = indirect_handle;
}

template <>
void build_action_entry<bm::MatchTableIndirectWS::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTableIndirectWS::Entry &entry,
    pi_table_entry_t *table_entry) {
  (void) p4info;
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
              size_t, const std::string &, typename M::Entry *) const>
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

// The difference with build_action_entry is the putput format.
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

  const auto adata_size = get_action_data_size(p4info, action_id);

  emit_p4_id(buffer->extend(sizeof(s_pi_p4_id_t)), action_id);
  emit_uint32(buffer->extend(sizeof(uint32_t)), adata_size);
  pibmv2::dump_action_data(p4info, buffer->extend(adata_size), action_id,
                           entry.action_data);
}

template <>
void build_action_entry_2<bm::MatchTableIndirect::Entry>(
    const pi_p4info_t *p4info, const bm::MatchTableIndirect::Entry &entry,
    Buffer *buffer) {
  (void) p4info;
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
  (void) p4info;
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

template <
  typename M,
  typename std::vector<typename M::Entry> (bm::RuntimeInterface::*GetFn)(
      size_t, const std::string &) const>
void get_entries_common(const pi_p4info_t *p4info, pi_p4_id_t table_id,
                        pi_table_fetch_res_t *res) {
  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  const auto entries = std::bind(GetFn, pibmv2::switch_, 0, t_name)();

  res->num_entries = entries.size();
  res->mkey_nbytes = get_match_key_size(p4info, table_id);

  Buffer buffer;
  for (const auto &e : entries) {
    emit_entry_handle(buffer.extend(sizeof(s_pi_entry_handle_t)), e.handle);
    emit_uint32(buffer.extend(sizeof(uint32_t)), e.priority);
    for (const auto &p : e.match_key) {
      switch (p.type) {
        case bm::MatchKeyParam::Type::EXACT:
          std::copy(p.key.begin(), p.key.end(), buffer.extend(p.key.size()));
          break;
        case bm::MatchKeyParam::Type::LPM:
          std::copy(p.key.begin(), p.key.end(), buffer.extend(p.key.size()));
          emit_uint32(buffer.extend(sizeof(uint32_t)), p.prefix_length);
          break;
        case bm::MatchKeyParam::Type::TERNARY:
        case bm::MatchKeyParam::Type::RANGE:
          std::copy(p.key.begin(), p.key.end(), buffer.extend(p.key.size()));
          std::copy(p.mask.begin(), p.mask.end(), buffer.extend(p.mask.size()));
          break;
        case bm::MatchKeyParam::Type::VALID:
          *buffer.extend(1) = (p.key == std::string("\x01", 1)) ? 1 : 0;
          break;
      }
    }
    build_action_entry_2(p4info, e, &buffer);

    // properties
    emit_uint32(buffer.extend(sizeof(uint32_t)), 0);
  }

  res->entries_size = buffer.size();
  res->entries = buffer.copy();
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

void set_direct_resources(const pi_p4info_t *p4info, pi_dev_id_t dev_id,
                          const std::string &t_name,
                          pi_entry_handle_t entry_handle,
                          const pi_direct_res_config_t *direct_res_config) {
  (void)p4info;
  (void)dev_id;
  if (!direct_res_config) return;
  for (size_t i = 0; i < direct_res_config->num_configs; i++) {
    pi_direct_res_config_one_t *config = &direct_res_config->configs[i];
    pi_res_type_id_t type = PI_GET_TYPE_ID(config->res_id);
    bm::MatchErrorCode error_code;
    switch (type) {
      case PI_COUNTER_ID:
        {
          uint64_t bytes, packets;
          pibmv2::convert_from_counter_data(
              reinterpret_cast<pi_counter_data_t *>(config->config),
              &bytes, &packets);
          error_code = pibmv2::switch_->mt_write_counters(
              0, t_name, entry_handle, bytes, packets);
        }
        break;
      case PI_METER_ID:
        {
          auto rates = pibmv2::convert_from_meter_spec(
              reinterpret_cast<pi_meter_spec_t *>(config->config));
          error_code = pibmv2::switch_->mt_set_meter_rates(
              0, t_name, entry_handle, rates);
        }
        break;
      default:  // TODO(antonin): what to do?
        assert(0);
    }
    if (error_code != bm::MatchErrorCode::SUCCESS)
      throw bm_exception(error_code);
  }
}

template <typename M,
          bm::MatchErrorCode (bm::RuntimeInterface::*GetFn)(
              size_t, const std::string &,
              const std::vector<bm::MatchKeyParam> &,
              typename M::Entry *, int) const>
pi_entry_handle_t get_entry_handle_from_key_common(
    pi_dev_id_t dev_id, const pi_p4info_t *p4info, pi_p4_id_t table_id,
    const pi_match_key_t *match_key) {
  (void) dev_id;
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
  const auto d_info = pibmv2::get_device_info();
  assert(d_info->assigned);
  const auto p4info = d_info->p4info;
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
  (void) overwrite;  // TODO(antonin)
  (void) session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;

  int priority;
  auto mkey = build_key(table_id, match_key, p4info, &priority);

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  // TODO(antonin): entry timeout
  try {
    switch (table_entry->entry_type) {
      case PI_ACTION_ENTRY_TYPE_DATA:
        *entry_handle = add_entry(
            p4info, dev_tgt, t_name, std::move(mkey), priority,
            table_entry->entry.action_data);
        break;
      case PI_ACTION_ENTRY_TYPE_INDIRECT:
        *entry_handle = add_indirect_entry(
            p4info, dev_tgt, t_name, std::move(mkey), priority,
            table_entry->entry.indirect_handle);
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
  (void) session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  try {
    if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA) {
      const pi_action_data_t *adata = table_entry->entry.action_data;

      // TODO(antonin): equivalent for indirect?
      // TODO(antonin): move to common PI code?
      if (pi_p4info_table_has_const_default_action(p4info, table_id)) {
        const pi_p4_id_t default_action_id =
            pi_p4info_table_get_const_default_action(p4info, table_id);
        if (default_action_id != adata->action_id)
          return PI_STATUS_CONST_DEFAULT_ACTION;
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

pi_status_t _pi_table_default_action_get(pi_session_handle_t session_handle,
                                         pi_dev_id_t dev_id,
                                         pi_p4_id_t table_id,
                                         pi_table_entry_t *table_entry) {
  (void) session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;

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
  (void) session_handle;

  if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA) {
    pi_action_data_t *action_data = table_entry->entry.action_data;
    if (action_data) delete[] action_data;
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entry_delete(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   pi_entry_handle_t entry_handle) {
  (void) session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;

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
                                        pi_dev_id_t dev_id, pi_p4_id_t table_id,
                                        const pi_match_key_t *match_key) {
  try {
    auto h = get_entry_handle_from_key(dev_id, table_id, match_key);
    return _pi_table_entry_delete(session_handle, dev_id, table_id, h);
  } catch (const bm_exception &e) {
    return e.get();
  }
}

pi_status_t _pi_table_entry_modify(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   pi_entry_handle_t entry_handle,
                                   const pi_table_entry_t *table_entry) {
  (void) session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;

  std::string t_name(pi_p4info_table_name_from_id(p4info, table_id));

  try {
    if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA) {
      modify_entry(p4info, dev_id, t_name, entry_handle,
                   table_entry->entry.action_data);
    } else if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_INDIRECT) {
      modify_indirect_entry(p4info, dev_id, t_name, entry_handle,
                            table_entry->entry.indirect_handle);
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
                                        pi_dev_id_t dev_id, pi_p4_id_t table_id,
                                        const pi_match_key_t *match_key,
                                        const pi_table_entry_t *table_entry) {
  try {
    auto h = get_entry_handle_from_key(dev_id, table_id, match_key);
    return _pi_table_entry_modify(session_handle, dev_id, table_id, h,
                                  table_entry);
  } catch (const bm_exception &e) {
    return e.get();
  }
}

pi_status_t _pi_table_entries_fetch(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t table_id,
                                    pi_table_fetch_res_t *res) {
  (void) session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;

  try {
    get_entries(p4info, table_id, res);
  } catch (const bm_exception &e) {
    return e.get();
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entries_fetch_done(pi_session_handle_t session_handle,
                                         pi_table_fetch_res_t *res) {
  (void) session_handle;

  delete[] res->entries;
  return PI_STATUS_SUCCESS;
}

}
