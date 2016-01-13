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

#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <set>

#include "P4Objects.h"
#include "match_tables.h"
#include "runtime_interface.h"

#ifndef BM_SIM_INCLUDE_BM_SIM_CONTEXT_H_
#define BM_SIM_INCLUDE_BM_SIM_CONTEXT_H_

class Context final {
  friend class SwitchWContexts;

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
  // needs to be default constructible if I want to put it in a std::vector
  Context();

  template<typename T>
  std::shared_ptr<T> get_component() {
    const auto &search = components.find(std::type_index(typeid(T)));
    if (search == components.end()) return nullptr;
    return std::static_pointer_cast<T>(search->second);
  }

  // do these methods need any protection?
  // TODO(antonin): should I return shared_ptrs instead of raw_ptrs?
  Pipeline *get_pipeline(const std::string &name) {
    return p4objects->get_pipeline(name);
  }

  Parser *get_parser(const std::string &name) {
    return p4objects->get_parser(name);
  }

  Deparser *get_deparser(const std::string &name) {
    return p4objects->get_deparser(name);
  }

  FieldList *get_field_list(const p4object_id_t field_list_id) {
    return p4objects->get_field_list(field_list_id);
  }

  // Added for testing, other "object types" can be added if needed
  p4object_id_t get_table_id(const std::string &name) {
    return p4objects->get_match_action_table(name)->get_id();
  }

  p4object_id_t get_action_id(const std::string &name) {
    return p4objects->get_action(name)->get_id();
  }

  template<typename T>
  bool add_component(std::shared_ptr<T> ptr) {
    std::shared_ptr<void> ptr_ = std::static_pointer_cast<void>(ptr);
    const auto &r = components.insert({std::type_index(typeid(T)), ptr_});
    return r.second;
  }

 private:
  // ---------- runtime interfaces ----------

  MatchErrorCode
  mt_add_entry(const std::string &table_name,
               const std::vector<MatchKeyParam> &match_key,
               const std::string &action_name,
               ActionData action_data,
               entry_handle_t *handle,
               int priority = -1  /*only used for ternary*/);

  MatchErrorCode
  mt_set_default_action(const std::string &table_name,
                        const std::string &action_name,
                        ActionData action_data);

  MatchErrorCode
  mt_delete_entry(const std::string &table_name,
                  entry_handle_t handle);

  MatchErrorCode
  mt_modify_entry(const std::string &table_name,
                  entry_handle_t handle,
                  const std::string &action_name,
                  ActionData action_data);

  MatchErrorCode
  mt_set_entry_ttl(const std::string &table_name,
                   entry_handle_t handle,
                   unsigned int ttl_ms);

  MatchErrorCode
  mt_indirect_add_member(const std::string &table_name,
                         const std::string &action_name,
                         ActionData action_data,
                         mbr_hdl_t *mbr);

  MatchErrorCode
  mt_indirect_delete_member(const std::string &table_name,
                            mbr_hdl_t mbr);

  MatchErrorCode
  mt_indirect_modify_member(const std::string &table_name,
                            mbr_hdl_t mbr_hdl,
                            const std::string &action_name,
                            ActionData action_data);

  MatchErrorCode
  mt_indirect_add_entry(const std::string &table_name,
                        const std::vector<MatchKeyParam> &match_key,
                        mbr_hdl_t mbr,
                        entry_handle_t *handle,
                        int priority = 1);

  MatchErrorCode
  mt_indirect_modify_entry(const std::string &table_name,
                           entry_handle_t handle,
                           mbr_hdl_t mbr);

  MatchErrorCode
  mt_indirect_delete_entry(const std::string &table_name,
                           entry_handle_t handle);

  MatchErrorCode
  mt_indirect_set_entry_ttl(const std::string &table_name,
                            entry_handle_t handle,
                            unsigned int ttl_ms);

  MatchErrorCode
  mt_indirect_set_default_member(const std::string &table_name,
                                 mbr_hdl_t mbr);

  MatchErrorCode
  mt_indirect_ws_create_group(const std::string &table_name,
                              grp_hdl_t *grp);

  MatchErrorCode
  mt_indirect_ws_delete_group(const std::string &table_name,
                              grp_hdl_t grp);

  MatchErrorCode
  mt_indirect_ws_add_member_to_group(const std::string &table_name,
                                     mbr_hdl_t mbr, grp_hdl_t grp);

  MatchErrorCode
  mt_indirect_ws_remove_member_from_group(
      const std::string &table_name,
      mbr_hdl_t mbr, grp_hdl_t grp);

  MatchErrorCode
  mt_indirect_ws_add_entry(const std::string &table_name,
                           const std::vector<MatchKeyParam> &match_key,
                           grp_hdl_t grp,
                           entry_handle_t *handle,
                           int priority = 1);

  MatchErrorCode
  mt_indirect_ws_modify_entry(const std::string &table_name,
                              entry_handle_t handle,
                              grp_hdl_t grp);

  MatchErrorCode
  mt_indirect_ws_set_default_group(const std::string &table_name,
                                   grp_hdl_t grp);

  MatchErrorCode
  mt_read_counters(const std::string &table_name,
                   entry_handle_t handle,
                   MatchTableAbstract::counter_value_t *bytes,
                   MatchTableAbstract::counter_value_t *packets);

  MatchErrorCode
  mt_reset_counters(const std::string &table_name);

  MatchErrorCode
  mt_write_counters(const std::string &table_name,
                    entry_handle_t handle,
                    MatchTableAbstract::counter_value_t bytes,
                    MatchTableAbstract::counter_value_t packets);

  MatchErrorCode
  mt_set_meter_rates(const std::string &table_name, entry_handle_t handle,
                     const std::vector<Meter::rate_config_t> &configs);

  Counter::CounterErrorCode
  read_counters(const std::string &counter_name,
                size_t index,
                MatchTableAbstract::counter_value_t *bytes,
                MatchTableAbstract::counter_value_t *packets);

  Counter::CounterErrorCode
  reset_counters(const std::string &counter_name);

  Counter::CounterErrorCode
  write_counters(const std::string &counter_name,
                 size_t index,
                 MatchTableAbstract::counter_value_t bytes,
                 MatchTableAbstract::counter_value_t packets);

  MeterErrorCode
  meter_array_set_rates(
      const std::string &meter_name,
      const std::vector<Meter::rate_config_t> &configs);

  MeterErrorCode
  meter_set_rates(const std::string &meter_name, size_t idx,
                  const std::vector<Meter::rate_config_t> &configs);

  RegisterErrorCode
  register_read(const std::string &register_name,
                const size_t idx, Data *value);

  RegisterErrorCode
  register_write(const std::string &register_name,
                 const size_t idx, Data value);

  MatchErrorCode
  dump_table(const std::string& table_name,
             std::ostream *stream) const;

  // ---------- End runtime interfaces ----------

  using ReadLock = std::unique_lock<std::mutex>;
  using WriteLock = std::unique_lock<std::mutex>;

  Context(const Context &other) = delete;
  Context &operator=(const Context &other) = delete;

 private:
  MatchErrorCode get_mt_indirect(const std::string &table_name,
                                 MatchTableIndirect **table);
  MatchErrorCode get_mt_indirect_ws(const std::string &table_name,
                                    MatchTableIndirectWS **table);

  PHVFactory &get_phv_factory();

  LearnEngine *get_learn_engine();

  AgeingMonitor *get_ageing_monitor();

  void set_notifications_transport(std::shared_ptr<TransportIface> transport);

  void set_device_id(int device_id);

  void set_cxt_id(int cxt_id);

  void set_force_arith(bool force_arith);

  typedef P4Objects::header_field_pair header_field_pair;
  int init_objects(std::istream *is,
                   const std::set<header_field_pair> &required_fields =
                     std::set<header_field_pair>(),
                   const std::set<header_field_pair> &arith_fields =
                     std::set<header_field_pair>());

  ErrorCode load_new_config(
      std::istream *is,
      const std::set<header_field_pair> &required_fields =
        std::set<header_field_pair>(),
      const std::set<header_field_pair> &arith_fields =
        std::set<header_field_pair>());

  ErrorCode swap_configs();

  ErrorCode reset_state();

  int do_swap();

  int swap_requested() { return swap_ordered; }

 private:  // data members
  size_t cxt_id{};

  int device_id{0};

  std::shared_ptr<P4Objects> p4objects{nullptr};
  std::shared_ptr<P4Objects> p4objects_rt{nullptr};

  std::unordered_map<std::type_index, std::shared_ptr<void> > components{};

  std::shared_ptr<TransportIface> notifications_transport{nullptr};

  mutable boost::shared_mutex request_mutex{};

  std::atomic<bool> swap_ordered{false};

  bool force_arith{false};
};

#endif  // BM_SIM_INCLUDE_BM_SIM_CONTEXT_H_
