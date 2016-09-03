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

#ifndef BM_BM_SIM_RUNTIME_INTERFACE_H_
#define BM_BM_SIM_RUNTIME_INTERFACE_H_

#include <string>
#include <vector>

#include "match_tables.h"

namespace bm {

class RuntimeInterface {
 public:
  typedef MatchTableIndirect::mbr_hdl_t mbr_hdl_t;
  typedef MatchTableIndirectWS::grp_hdl_t grp_hdl_t;

  typedef Meter::MeterErrorCode MeterErrorCode;
  typedef Register::RegisterErrorCode RegisterErrorCode;

  enum ErrorCode {
    SUCCESS = 0,
    CONFIG_SWAP_DISABLED,
    ONGOING_SWAP,
    NO_ONGOING_SWAP
  };

 public:
  virtual ~RuntimeInterface() { }

  virtual MatchErrorCode
  mt_add_entry(size_t cxt_id,
               const std::string &table_name,
               const std::vector<MatchKeyParam> &match_key,
               const std::string &action_name,
               ActionData action_data,  // will be moved
               entry_handle_t *handle,
               int priority = -1  /*only used for ternary*/) = 0;

  virtual MatchErrorCode
  mt_set_default_action(size_t cxt_id,
                        const std::string &table_name,
                        const std::string &action_name,
                        ActionData action_data) = 0;

  virtual MatchErrorCode
  mt_delete_entry(size_t cxt_id,
                  const std::string &table_name,
                  entry_handle_t handle) = 0;

  virtual MatchErrorCode
  mt_modify_entry(size_t cxt_id,
                  const std::string &table_name,
                  entry_handle_t handle,
                  const std::string &action_name,
                  ActionData action_data) = 0;

  virtual MatchErrorCode
  mt_set_entry_ttl(size_t cxt_id,
                   const std::string &table_name,
                   entry_handle_t handle,
                   unsigned int ttl_ms) = 0;

  virtual MatchErrorCode
  mt_indirect_add_member(size_t cxt_id,
                         const std::string &table_name,
                         const std::string &action_name,
                         ActionData action_data,
                         mbr_hdl_t *mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_delete_member(size_t cxt_id,
                            const std::string &table_name,
                            mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_modify_member(size_t cxt_id,
                            const std::string &table_name,
                            mbr_hdl_t mbr_hdl,
                            const std::string &action_name,
                            ActionData action_data) = 0;

  virtual MatchErrorCode
  mt_indirect_add_entry(size_t cxt_id,
                        const std::string &table_name,
                        const std::vector<MatchKeyParam> &match_key,
                        mbr_hdl_t mbr,
                        entry_handle_t *handle,
                        int priority = 1) = 0;

  virtual MatchErrorCode
  mt_indirect_modify_entry(size_t cxt_id,
                           const std::string &table_name,
                           entry_handle_t handle,
                           mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_delete_entry(size_t cxt_id,
                           const std::string &table_name,
                           entry_handle_t handle) = 0;

  virtual MatchErrorCode
  mt_indirect_set_entry_ttl(size_t cxt_id,
                            const std::string &table_name,
                            entry_handle_t handle,
                            unsigned int ttl_ms) = 0;

  virtual MatchErrorCode
  mt_indirect_set_default_member(size_t cxt_id,
                                 const std::string &table_name,
                                 mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_create_group(size_t cxt_id,
                              const std::string &table_name,
                              grp_hdl_t *grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_delete_group(size_t cxt_id,
                              const std::string &table_name,
                              grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_add_member_to_group(size_t cxt_id,
                                     const std::string &table_name,
                                     mbr_hdl_t mbr, grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_remove_member_from_group(size_t cxt_id,
                                          const std::string &table_name,
                                          mbr_hdl_t mbr, grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_add_entry(size_t cxt_id,
                           const std::string &table_name,
                           const std::vector<MatchKeyParam> &match_key,
                           grp_hdl_t grp,
                           entry_handle_t *handle,
                           int priority = 1) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_modify_entry(size_t cxt_id,
                              const std::string &table_name,
                              entry_handle_t handle,
                              grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_set_default_group(size_t cxt_id,
                                   const std::string &table_name,
                                   grp_hdl_t grp) = 0;


  virtual MatchErrorCode
  mt_read_counters(size_t cxt_id,
                   const std::string &table_name,
                   entry_handle_t handle,
                   MatchTableAbstract::counter_value_t *bytes,
                   MatchTableAbstract::counter_value_t *packets) = 0;

  virtual MatchErrorCode
  mt_reset_counters(size_t cxt_id,
                    const std::string &table_name) = 0;

  virtual MatchErrorCode
  mt_write_counters(size_t cxt_id,
                    const std::string &table_name,
                    entry_handle_t handle,
                    MatchTableAbstract::counter_value_t bytes,
                    MatchTableAbstract::counter_value_t packets) = 0;

  virtual MatchErrorCode
  mt_set_meter_rates(size_t cxt_id,
                     const std::string &table_name,
                     entry_handle_t handle,
                     const std::vector<Meter::rate_config_t> &configs) = 0;

  virtual MatchErrorCode
  mt_get_meter_rates(size_t cxt_id,
                     const std::string &table_name,
                     entry_handle_t handle,
                     std::vector<Meter::rate_config_t> *configs) = 0;

  virtual MatchTableType
  mt_get_type(size_t cxt_id, const std::string &table_name) const = 0;

  virtual std::vector<MatchTable::Entry>
  mt_get_entries(size_t cxt_id, const std::string &table_name) const = 0;

  virtual std::vector<MatchTableIndirect::Entry>
  mt_indirect_get_entries(size_t cxt_id,
                          const std::string &table_name) const = 0;

  virtual std::vector<MatchTableIndirectWS::Entry>
  mt_indirect_ws_get_entries(size_t cxt_id,
                             const std::string &table_name) const = 0;

  virtual MatchErrorCode
  mt_get_entry(size_t cxt_id, const std::string &table_name,
               entry_handle_t handle, MatchTable::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_get_entry(size_t cxt_id, const std::string &table_name,
                        entry_handle_t handle,
                        MatchTableIndirect::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_ws_get_entry(size_t cxt_id, const std::string &table_name,
                           entry_handle_t handle,
                           MatchTableIndirectWS::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_get_default_entry(size_t cxt_id, const std::string &table_name,
                       MatchTable::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_get_default_entry(size_t cxt_id, const std::string &table_name,
                                MatchTableIndirect::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_ws_get_default_entry(
      size_t cxt_id, const std::string &table_name,
      MatchTableIndirectWS::Entry *entry) const = 0;

  virtual std::vector<MatchTableIndirect::Member>
  mt_indirect_get_members(size_t cxt_id,
                          const std::string &table_name) const = 0;

  virtual MatchErrorCode
  mt_indirect_get_member(size_t cxt_id, const std::string &table_name,
                         mbr_hdl_t mbr,
                         MatchTableIndirect::Member *member) const = 0;

  virtual std::vector<MatchTableIndirectWS::Group>
  mt_indirect_ws_get_groups(size_t cxt_id,
                            const std::string &table_name) const = 0;

  virtual MatchErrorCode
  mt_indirect_ws_get_group(size_t cxt_id, const std::string &table_name,
                           grp_hdl_t grp,
                           MatchTableIndirectWS::Group *group) const = 0;

  virtual Counter::CounterErrorCode
  read_counters(size_t cxt_id,
                const std::string &counter_name,
                size_t index,
                MatchTableAbstract::counter_value_t *bytes,
                MatchTableAbstract::counter_value_t *packets) = 0;

  virtual Counter::CounterErrorCode
  reset_counters(size_t cxt_id,
                 const std::string &counter_name) = 0;

  virtual Counter::CounterErrorCode
  write_counters(size_t cxt_id,
                 const std::string &counter_name,
                 size_t index,
                 MatchTableAbstract::counter_value_t bytes,
                 MatchTableAbstract::counter_value_t packets) = 0;


  virtual MeterErrorCode
  meter_array_set_rates(size_t cxt_id,
                        const std::string &meter_name,
                        const std::vector<Meter::rate_config_t> &configs) = 0;

  virtual MeterErrorCode
  meter_set_rates(size_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  const std::vector<Meter::rate_config_t> &configs) = 0;

  virtual MeterErrorCode
  meter_get_rates(size_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  std::vector<Meter::rate_config_t> *configs) = 0;

  virtual RegisterErrorCode
  register_read(size_t cxt_id,
                const std::string &register_name,
                const size_t idx, Data *value) = 0;

  virtual RegisterErrorCode
  register_write(size_t cxt_id,
                 const std::string &register_name,
                 const size_t idx, Data value) = 0;  // to be moved

  virtual RegisterErrorCode
  register_write_range(size_t cxt_id,
                       const std::string &register_name,
                       const size_t start, const size_t end,
                       Data value) = 0;

  virtual RegisterErrorCode
  register_reset(size_t cxt_id, const std::string &register_name) = 0;

  virtual ParseVSet::ErrorCode
  parse_vset_add(size_t cxt_id, const std::string &parse_vset_name,
                 const ByteContainer &value) = 0;

  virtual ParseVSet::ErrorCode
  parse_vset_remove(size_t cxt_id, const std::string &parse_vset_name,
                    const ByteContainer &value) = 0;

  virtual ErrorCode
  load_new_config(const std::string &new_config) = 0;

  virtual ErrorCode
  swap_configs() = 0;

  virtual std::string
  get_config() const = 0;

  virtual std::string
  get_config_md5() const = 0;

  virtual CustomCrcErrorCode
  set_crc16_custom_parameters(
      size_t cxt_id, const std::string &calc_name,
      const CustomCrcMgr<uint16_t>::crc_config_t &crc16_config) = 0;

  virtual CustomCrcErrorCode
  set_crc32_custom_parameters(
      size_t cxt_id, const std::string &calc_name,
      const CustomCrcMgr<uint32_t>::crc_config_t &crc32_config) = 0;

  virtual ErrorCode
  reset_state() = 0;

  virtual ErrorCode
  serialize(std::ostream *out) = 0;
};

}  // namespace bm

#endif  // BM_BM_SIM_RUNTIME_INTERFACE_H_
