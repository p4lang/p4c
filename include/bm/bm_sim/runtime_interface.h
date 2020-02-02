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
#include <iosfwd>

#include "action_profile.h"
#include "match_tables.h"
#include "device_id.h"

namespace bm {

class RuntimeInterface {
 public:
  using mbr_hdl_t = MatchTableIndirect::mbr_hdl_t;
  using grp_hdl_t = MatchTableIndirectWS::grp_hdl_t;

  using MeterErrorCode = Meter::MeterErrorCode;
  using RegisterErrorCode = Register::RegisterErrorCode;

  enum ErrorCode {
    SUCCESS = 0,
    CONFIG_SWAP_DISABLED,
    ONGOING_SWAP,
    NO_ONGOING_SWAP
  };

 public:
  virtual ~RuntimeInterface() { }

  // common to all tables

  virtual MatchErrorCode
  mt_get_num_entries(cxt_id_t cxt_id,
                     const std::string &table_name,
                     size_t *num_entries) const = 0;

  virtual MatchErrorCode
  mt_clear_entries(cxt_id_t cxt_id,
                   const std::string &table_name,
                   bool reset_default_entry) = 0;

  // direct tables

  virtual MatchErrorCode
  mt_add_entry(cxt_id_t cxt_id,
               const std::string &table_name,
               const std::vector<MatchKeyParam> &match_key,
               const std::string &action_name,
               ActionData action_data,  // will be moved
               entry_handle_t *handle,
               int priority = -1  /*only used for ternary*/) = 0;

  virtual MatchErrorCode
  mt_set_default_action(cxt_id_t cxt_id,
                        const std::string &table_name,
                        const std::string &action_name,
                        ActionData action_data) = 0;

  virtual MatchErrorCode
  mt_reset_default_entry(cxt_id_t cxt_id,
                         const std::string &table_name) = 0;

  virtual MatchErrorCode
  mt_delete_entry(cxt_id_t cxt_id,
                  const std::string &table_name,
                  entry_handle_t handle) = 0;

  virtual MatchErrorCode
  mt_modify_entry(cxt_id_t cxt_id,
                  const std::string &table_name,
                  entry_handle_t handle,
                  const std::string &action_name,
                  ActionData action_data) = 0;

  virtual MatchErrorCode
  mt_set_entry_ttl(cxt_id_t cxt_id,
                   const std::string &table_name,
                   entry_handle_t handle,
                   unsigned int ttl_ms) = 0;

  // action profiles

  virtual MatchErrorCode
  mt_act_prof_add_member(cxt_id_t cxt_id,
                         const std::string &act_prof_name,
                         const std::string &action_name,
                         ActionData action_data,
                         mbr_hdl_t *mbr) = 0;

  virtual MatchErrorCode
  mt_act_prof_delete_member(cxt_id_t cxt_id,
                            const std::string &act_prof_name,
                            mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_act_prof_modify_member(cxt_id_t cxt_id,
                            const std::string &act_prof_name,
                            mbr_hdl_t mbr,
                            const std::string &action_name,
                            ActionData action_data) = 0;

  virtual MatchErrorCode
  mt_act_prof_create_group(cxt_id_t cxt_id,
                           const std::string &act_prof_name,
                           grp_hdl_t *grp) = 0;

  virtual MatchErrorCode
  mt_act_prof_delete_group(cxt_id_t cxt_id,
                           const std::string &act_prof_name,
                           grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_act_prof_add_member_to_group(cxt_id_t cxt_id,
                                  const std::string &act_prof_name,
                                  mbr_hdl_t mbr, grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_act_prof_remove_member_from_group(cxt_id_t cxt_id,
                                       const std::string &act_prof_name,
                                       mbr_hdl_t mbr, grp_hdl_t grp) = 0;

  virtual std::vector<ActionProfile::Member>
  mt_act_prof_get_members(cxt_id_t cxt_id,
                          const std::string &act_prof_name) const = 0;

  virtual MatchErrorCode
  mt_act_prof_get_member(cxt_id_t cxt_id, const std::string &act_prof_name,
                         mbr_hdl_t mbr,
                         ActionProfile::Member *member) const = 0;

  virtual std::vector<ActionProfile::Group>
  mt_act_prof_get_groups(cxt_id_t cxt_id,
                         const std::string &act_prof_name) const = 0;

  virtual MatchErrorCode
  mt_act_prof_get_group(cxt_id_t cxt_id, const std::string &act_prof_name,
                        grp_hdl_t grp,
                        ActionProfile::Group *group) const = 0;

  // indirect tables

  virtual MatchErrorCode
  mt_indirect_add_entry(cxt_id_t cxt_id,
                        const std::string &table_name,
                        const std::vector<MatchKeyParam> &match_key,
                        mbr_hdl_t mbr,
                        entry_handle_t *handle,
                        int priority = 1) = 0;

  virtual MatchErrorCode
  mt_indirect_modify_entry(cxt_id_t cxt_id,
                           const std::string &table_name,
                           entry_handle_t handle,
                           mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_delete_entry(cxt_id_t cxt_id,
                           const std::string &table_name,
                           entry_handle_t handle) = 0;

  virtual MatchErrorCode
  mt_indirect_set_entry_ttl(cxt_id_t cxt_id,
                            const std::string &table_name,
                            entry_handle_t handle,
                            unsigned int ttl_ms) = 0;

  virtual MatchErrorCode
  mt_indirect_set_default_member(cxt_id_t cxt_id,
                                 const std::string &table_name,
                                 mbr_hdl_t mbr) = 0;

  virtual MatchErrorCode
  mt_indirect_reset_default_entry(cxt_id_t cxt_id,
                                  const std::string &table_name) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_add_entry(cxt_id_t cxt_id,
                           const std::string &table_name,
                           const std::vector<MatchKeyParam> &match_key,
                           grp_hdl_t grp,
                           entry_handle_t *handle,
                           int priority = 1) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_modify_entry(cxt_id_t cxt_id,
                              const std::string &table_name,
                              entry_handle_t handle,
                              grp_hdl_t grp) = 0;

  virtual MatchErrorCode
  mt_indirect_ws_set_default_group(cxt_id_t cxt_id,
                                   const std::string &table_name,
                                   grp_hdl_t grp) = 0;


  virtual MatchErrorCode
  mt_read_counters(cxt_id_t cxt_id,
                   const std::string &table_name,
                   entry_handle_t handle,
                   MatchTableAbstract::counter_value_t *bytes,
                   MatchTableAbstract::counter_value_t *packets) = 0;

  virtual MatchErrorCode
  mt_reset_counters(cxt_id_t cxt_id,
                    const std::string &table_name) = 0;

  virtual MatchErrorCode
  mt_write_counters(cxt_id_t cxt_id,
                    const std::string &table_name,
                    entry_handle_t handle,
                    MatchTableAbstract::counter_value_t bytes,
                    MatchTableAbstract::counter_value_t packets) = 0;

  virtual MatchErrorCode
  mt_set_meter_rates(cxt_id_t cxt_id,
                     const std::string &table_name,
                     entry_handle_t handle,
                     const std::vector<Meter::rate_config_t> &configs) = 0;

  virtual MatchErrorCode
  mt_get_meter_rates(cxt_id_t cxt_id,
                     const std::string &table_name,
                     entry_handle_t handle,
                     std::vector<Meter::rate_config_t> *configs) = 0;

  virtual MatchTableType
  mt_get_type(cxt_id_t cxt_id, const std::string &table_name) const = 0;

  virtual std::vector<MatchTable::Entry>
  mt_get_entries(cxt_id_t cxt_id, const std::string &table_name) const = 0;

  virtual std::vector<MatchTableIndirect::Entry>
  mt_indirect_get_entries(cxt_id_t cxt_id,
                          const std::string &table_name) const = 0;

  virtual std::vector<MatchTableIndirectWS::Entry>
  mt_indirect_ws_get_entries(cxt_id_t cxt_id,
                             const std::string &table_name) const = 0;

  virtual MatchErrorCode
  mt_get_entry(cxt_id_t cxt_id, const std::string &table_name,
               entry_handle_t handle, MatchTable::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_get_entry(cxt_id_t cxt_id, const std::string &table_name,
                        entry_handle_t handle,
                        MatchTableIndirect::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_ws_get_entry(cxt_id_t cxt_id, const std::string &table_name,
                           entry_handle_t handle,
                           MatchTableIndirectWS::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_get_default_entry(cxt_id_t cxt_id, const std::string &table_name,
                       MatchTable::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_get_default_entry(cxt_id_t cxt_id, const std::string &table_name,
                                MatchTableIndirect::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_indirect_ws_get_default_entry(
      cxt_id_t cxt_id, const std::string &table_name,
      MatchTableIndirectWS::Entry *entry) const = 0;

  virtual MatchErrorCode
  mt_get_entry_from_key(cxt_id_t cxt_id, const std::string &table_name,
                        const std::vector<MatchKeyParam> &match_key,
                        MatchTable::Entry *entry, int priority = 1) const = 0;

  virtual MatchErrorCode
  mt_indirect_get_entry_from_key(cxt_id_t cxt_id, const std::string &table_name,
                                 const std::vector<MatchKeyParam> &match_key,
                                 MatchTableIndirect::Entry *entry,
                                 int priority = 1) const = 0;

  virtual MatchErrorCode
  mt_indirect_ws_get_entry_from_key(cxt_id_t cxt_id,
                                    const std::string &table_name,
                                    const std::vector<MatchKeyParam> &match_key,
                                    MatchTableIndirectWS::Entry *entry,
                                    int priority = 1) const = 0;

  virtual Counter::CounterErrorCode
  read_counters(cxt_id_t cxt_id,
                const std::string &counter_name,
                size_t index,
                MatchTableAbstract::counter_value_t *bytes,
                MatchTableAbstract::counter_value_t *packets) = 0;

  virtual Counter::CounterErrorCode
  reset_counters(cxt_id_t cxt_id,
                 const std::string &counter_name) = 0;

  virtual Counter::CounterErrorCode
  write_counters(cxt_id_t cxt_id,
                 const std::string &counter_name,
                 size_t index,
                 MatchTableAbstract::counter_value_t bytes,
                 MatchTableAbstract::counter_value_t packets) = 0;


  virtual MeterErrorCode
  meter_array_set_rates(cxt_id_t cxt_id,
                        const std::string &meter_name,
                        const std::vector<Meter::rate_config_t> &configs) = 0;

  virtual MeterErrorCode
  meter_set_rates(cxt_id_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  const std::vector<Meter::rate_config_t> &configs) = 0;

  virtual MeterErrorCode
  meter_get_rates(cxt_id_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  std::vector<Meter::rate_config_t> *configs) = 0;

  virtual RegisterErrorCode
  register_read(cxt_id_t cxt_id,
                const std::string &register_name,
                const size_t idx, Data *value) = 0;

  virtual std::vector<Data>
  register_read_all(cxt_id_t cxt_id, const std::string &register_name) = 0;

  virtual RegisterErrorCode
  register_write(cxt_id_t cxt_id,
                 const std::string &register_name,
                 const size_t idx, Data value) = 0;  // to be moved

  virtual RegisterErrorCode
  register_write_range(cxt_id_t cxt_id,
                       const std::string &register_name,
                       const size_t start, const size_t end,
                       Data value) = 0;

  virtual RegisterErrorCode
  register_reset(cxt_id_t cxt_id, const std::string &register_name) = 0;

  virtual ParseVSet::ErrorCode
  parse_vset_add(cxt_id_t cxt_id, const std::string &parse_vset_name,
                 const ByteContainer &value) = 0;

  virtual ParseVSet::ErrorCode
  parse_vset_remove(cxt_id_t cxt_id, const std::string &parse_vset_name,
                    const ByteContainer &value) = 0;

  virtual ParseVSet::ErrorCode
  parse_vset_get(cxt_id_t cxt_id, const std::string &parse_vset_name,
                 std::vector<ByteContainer> *values) = 0;

  virtual ParseVSet::ErrorCode
  parse_vset_clear(cxt_id_t cxt_id, const std::string &parse_vset_name) = 0;

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
      cxt_id_t cxt_id, const std::string &calc_name,
      const CustomCrcMgr<uint16_t>::crc_config_t &crc16_config) = 0;

  virtual CustomCrcErrorCode
  set_crc32_custom_parameters(
      cxt_id_t cxt_id, const std::string &calc_name,
      const CustomCrcMgr<uint32_t>::crc_config_t &crc32_config) = 0;

  virtual ErrorCode
  reset_state() = 0;

  virtual ErrorCode
  serialize(std::ostream *out) = 0;
};

}  // namespace bm

#endif  // BM_BM_SIM_RUNTIME_INTERFACE_H_
