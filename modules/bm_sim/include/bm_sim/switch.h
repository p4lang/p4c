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

#ifndef BM_SIM_INCLUDE_BM_SIM_SWITCH_H_
#define BM_SIM_INCLUDE_BM_SIM_SWITCH_H_

#include <boost/thread/shared_mutex.hpp>

#include <memory>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <set>
#include <vector>

#include "context.h"
#include "queue.h"
#include "packet.h"
#include "learning.h"
#include "runtime_interface.h"
#include "dev_mgr.h"
#include "phv_source.h"

namespace bm {

// multiple inheritance in accordance with Google C++ guidelines:
// "Multiple inheritance is allowed only when all superclasses, with the
// possible exception of the first one, are pure interfaces. In order to ensure
// that they remain pure interfaces, they must end with the Interface suffix."
class SwitchWContexts : public DevMgr, public RuntimeInterface {
  friend class Switch;

 public:
  explicit SwitchWContexts(size_t nb_cxts = 1u, bool enable_swap = false);

  Context *get_context(size_t cxt_id = 0u) {
    return &contexts.at(cxt_id);
  }

  virtual int receive(int port_num, const char *buffer, int len) = 0;

  virtual void start_and_return() = 0;

  // returns the Thrift port if one was specified on the command line
  int get_runtime_port() { return thrift_port; }

  /* Specify that the field is required for this target switch, i.e. the field
     needs to be defined in the input json */
  void add_required_field(const std::string &header_name,
                          const std::string &field_name);

  /* Force arithmetic on field. No effect if field is not defined in the
     input json */
  void force_arith_field(const std::string &header_name,
                         const std::string &field_name);

  size_t get_nb_cxts() { return nb_cxts; }

  int init_objects(const std::string &json_path, int device_id = 0,
                   std::shared_ptr<TransportIface> notif_transport = nullptr);

  int init_from_command_line_options(int argc, char *argv[]);

  template<typename T>
  std::shared_ptr<T> get_component() {
    const auto &search = components.find(std::type_index(typeid(T)));
    if (search == components.end()) return nullptr;
    return std::static_pointer_cast<T>(search->second);
  }

  template<typename T>
  std::shared_ptr<T> get_cxt_component(size_t cxt_id) {
    return contexts.at(cxt_id).get_component<T>();
  }

  int swap_requested();

  // invalidates all pointers obtained with get_pipeline(), get_parser(),...
  int do_swap();

  std::unique_ptr<Packet> new_packet_ptr(size_t cxt_id, int ingress_port,
                                         packet_id_t id, int ingress_length,
                                         // cpplint false positive
                                         // NOLINTNEXTLINE(whitespace/operators)
                                         PacketBuffer &&buffer) {
    return std::unique_ptr<Packet>(new Packet(
        cxt_id, ingress_port, id, 0u, ingress_length, std::move(buffer),
        phv_source.get()));
  }

  Packet new_packet(size_t cxt_id, int ingress_port, packet_id_t id,
                    // cpplint false positive
                    // NOLINTNEXTLINE(whitespace/operators)
                    int ingress_length, PacketBuffer &&buffer) {
    return Packet(cxt_id, ingress_port, id, 0u, ingress_length,
                  std::move(buffer), phv_source.get());
  }

  LearnEngine *get_learn_engine(size_t cxt_id) {
    return contexts.at(cxt_id).get_learn_engine();
  }

  AgeingMonitor *get_ageing_monitor(size_t cxt_id) {
    return contexts.at(cxt_id).get_ageing_monitor();
  }

  // ---------- RuntimeInterface ----------

  MatchErrorCode
  mt_add_entry(size_t cxt_id,
               const std::string &table_name,
               const std::vector<MatchKeyParam> &match_key,
               const std::string &action_name,
               ActionData action_data,
               entry_handle_t *handle,
               int priority = -1  /*only used for ternary*/) override {
    return contexts.at(cxt_id).mt_add_entry(
        table_name, match_key, action_name,
        std::move(action_data), handle, priority);
  }

  MatchErrorCode
  mt_set_default_action(size_t cxt_id,
                        const std::string &table_name,
                        const std::string &action_name,
                        ActionData action_data) override {
    return contexts.at(cxt_id).mt_set_default_action(
        table_name, action_name, std::move(action_data));
  }

  MatchErrorCode
  mt_delete_entry(size_t cxt_id,
                  const std::string &table_name,
                  entry_handle_t handle) override {
    return contexts.at(cxt_id).mt_delete_entry(table_name, handle);
  }

  MatchErrorCode
  mt_modify_entry(size_t cxt_id,
                  const std::string &table_name,
                  entry_handle_t handle,
                  const std::string &action_name,
                  ActionData action_data) override {
    return contexts.at(cxt_id).mt_modify_entry(
        table_name, handle, action_name, std::move(action_data));
  }

  MatchErrorCode
  mt_set_entry_ttl(size_t cxt_id,
                   const std::string &table_name,
                   entry_handle_t handle,
                   unsigned int ttl_ms) override {
    return contexts.at(cxt_id).mt_set_entry_ttl(table_name, handle, ttl_ms);
  }

  MatchErrorCode
  mt_indirect_add_member(size_t cxt_id,
                         const std::string &table_name,
                         const std::string &action_name,
                         ActionData action_data,
                         mbr_hdl_t *mbr) override {
    return contexts.at(cxt_id).mt_indirect_add_member(
        table_name, action_name, std::move(action_data), mbr);
  }

  MatchErrorCode
  mt_indirect_delete_member(size_t cxt_id,
                            const std::string &table_name,
                            mbr_hdl_t mbr) override {
    return contexts.at(cxt_id).mt_indirect_delete_member(table_name, mbr);
  }

  MatchErrorCode
  mt_indirect_modify_member(size_t cxt_id,
                            const std::string &table_name,
                            mbr_hdl_t mbr_hdl,
                            const std::string &action_name,
                            ActionData action_data) override {
    return contexts.at(cxt_id).mt_indirect_modify_member(
        table_name, mbr_hdl, action_name, std::move(action_data));
  }

  MatchErrorCode
  mt_indirect_add_entry(size_t cxt_id,
                        const std::string &table_name,
                        const std::vector<MatchKeyParam> &match_key,
                        mbr_hdl_t mbr,
                        entry_handle_t *handle,
                        int priority = 1) override {
    return contexts.at(cxt_id).mt_indirect_add_entry(
        table_name, match_key, mbr, handle, priority);
  }

  MatchErrorCode
  mt_indirect_modify_entry(size_t cxt_id,
                           const std::string &table_name,
                           entry_handle_t handle,
                           mbr_hdl_t mbr) override {
    return contexts.at(cxt_id).mt_indirect_modify_entry(
        table_name, handle, mbr);
  }

  MatchErrorCode
  mt_indirect_delete_entry(size_t cxt_id,
                           const std::string &table_name,
                           entry_handle_t handle) override {
    return contexts.at(cxt_id).mt_indirect_delete_entry(table_name, handle);
  }

  MatchErrorCode
  mt_indirect_set_entry_ttl(size_t cxt_id,
                            const std::string &table_name,
                            entry_handle_t handle,
                            unsigned int ttl_ms) override {
    return contexts.at(cxt_id).mt_indirect_set_entry_ttl(
        table_name, handle, ttl_ms);
  }

  MatchErrorCode
  mt_indirect_set_default_member(size_t cxt_id,
                                 const std::string &table_name,
                                 mbr_hdl_t mbr) override {
    return contexts.at(cxt_id).mt_indirect_set_default_member(table_name, mbr);
  }

  MatchErrorCode
  mt_indirect_ws_create_group(size_t cxt_id,
                              const std::string &table_name,
                              grp_hdl_t *grp) override {
    return contexts.at(cxt_id).mt_indirect_ws_create_group(table_name, grp);
  }

  MatchErrorCode
  mt_indirect_ws_delete_group(size_t cxt_id,
                              const std::string &table_name,
                              grp_hdl_t grp) override {
    return contexts.at(cxt_id).mt_indirect_ws_delete_group(table_name, grp);
  }

  MatchErrorCode
  mt_indirect_ws_add_member_to_group(size_t cxt_id,
                                     const std::string &table_name,
                                     mbr_hdl_t mbr, grp_hdl_t grp) override {
    return contexts.at(cxt_id).mt_indirect_ws_add_member_to_group(
        table_name, mbr, grp);
  }

  MatchErrorCode
  mt_indirect_ws_remove_member_from_group(size_t cxt_id,
                                          const std::string &table_name,
                                          mbr_hdl_t mbr,
                                          grp_hdl_t grp) override {
    return contexts.at(cxt_id).mt_indirect_ws_remove_member_from_group(
        table_name, mbr, grp);
  }

  MatchErrorCode
  mt_indirect_ws_add_entry(size_t cxt_id,
                           const std::string &table_name,
                           const std::vector<MatchKeyParam> &match_key,
                           grp_hdl_t grp,
                           entry_handle_t *handle,
                           int priority = 1) override {
    return contexts.at(cxt_id).mt_indirect_ws_add_entry(
        table_name, match_key, grp, handle, priority);
  }

  MatchErrorCode
  mt_indirect_ws_modify_entry(size_t cxt_id,
                              const std::string &table_name,
                              entry_handle_t handle,
                              grp_hdl_t grp) override {
    return contexts.at(cxt_id).mt_indirect_ws_modify_entry(
        table_name, handle, grp);
  }

  MatchErrorCode
  mt_indirect_ws_set_default_group(size_t cxt_id,
                                   const std::string &table_name,
                                   grp_hdl_t grp) override {
    return contexts.at(cxt_id).mt_indirect_ws_set_default_group(
        table_name, grp);
  }

  MatchErrorCode
  mt_read_counters(size_t cxt_id,
                   const std::string &table_name,
                   entry_handle_t handle,
                   MatchTableAbstract::counter_value_t *bytes,
                   MatchTableAbstract::counter_value_t *packets) override {
    return contexts.at(cxt_id).mt_read_counters(
        table_name, handle, bytes, packets);
  }

  MatchErrorCode
  mt_reset_counters(size_t cxt_id,
                    const std::string &table_name) override {
    return contexts.at(cxt_id).mt_reset_counters(table_name);
  }

  MatchErrorCode
  mt_write_counters(size_t cxt_id,
                    const std::string &table_name,
                    entry_handle_t handle,
                    MatchTableAbstract::counter_value_t bytes,
                    MatchTableAbstract::counter_value_t packets) override {
    return contexts.at(cxt_id).mt_write_counters(
        table_name, handle, bytes, packets);
  }

  MatchErrorCode
  mt_set_meter_rates(
      size_t cxt_id, const std::string &table_name, entry_handle_t handle,
      const std::vector<Meter::rate_config_t> &configs) override {
    return contexts.at(cxt_id).mt_set_meter_rates(table_name, handle, configs);
  }

  Counter::CounterErrorCode
  read_counters(size_t cxt_id,
                const std::string &counter_name,
                size_t index,
                MatchTableAbstract::counter_value_t *bytes,
                MatchTableAbstract::counter_value_t *packets) override {
    return contexts.at(cxt_id).read_counters(
        counter_name, index, bytes, packets);
  }

  Counter::CounterErrorCode
  reset_counters(size_t cxt_id,
                 const std::string &counter_name) override {
    return contexts.at(cxt_id).reset_counters(counter_name);
  }

  Counter::CounterErrorCode
  write_counters(size_t cxt_id,
                 const std::string &counter_name,
                 size_t index,
                 MatchTableAbstract::counter_value_t bytes,
                 MatchTableAbstract::counter_value_t packets) override {
    return contexts.at(cxt_id).write_counters(
        counter_name, index, bytes, packets);
  }

  MeterErrorCode
  meter_array_set_rates(
      size_t cxt_id, const std::string &meter_name,
      const std::vector<Meter::rate_config_t> &configs) override {
    return contexts.at(cxt_id).meter_array_set_rates(meter_name, configs);
  }

  MeterErrorCode
  meter_set_rates(size_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  const std::vector<Meter::rate_config_t> &configs) override {
    return contexts.at(cxt_id).meter_set_rates(meter_name, idx, configs);
  }

  RegisterErrorCode
  register_read(size_t cxt_id,
                const std::string &register_name,
                const size_t idx, Data *value) override {
    return contexts.at(cxt_id).register_read(register_name, idx, value);
  }

  RegisterErrorCode
  register_write(size_t cxt_id,
                 const std::string &register_name,
                 const size_t idx, Data value) override {
    return contexts.at(cxt_id).register_write(
        register_name, idx, std::move(value));
  }

  RuntimeInterface::ErrorCode
  reset_state() override;

  MatchErrorCode
  dump_table(size_t cxt_id,
             const std::string& table_name,
             std::ostream *stream) const override {
    return contexts.at(cxt_id).dump_table(table_name, stream);
  }

  RuntimeInterface::ErrorCode
  load_new_config(const std::string &new_config) override;

  RuntimeInterface::ErrorCode
  swap_configs() override;

  // ---------- End RuntimeInterface ----------

 protected:
  typedef Context::header_field_pair header_field_pair;

  const std::set<header_field_pair> &get_required_fields() const {
    return required_fields;
  }

  const std::set<header_field_pair> &get_arith_fields() const {
    return arith_fields;
  }

  template<typename T>
  bool add_component(std::shared_ptr<T> ptr) {
    std::shared_ptr<void> ptr_ = std::static_pointer_cast<void>(ptr);
    const auto &r = components.insert({std::type_index(typeid(T)), ptr_});
    return r.second;
  }

  template<typename T>
  bool add_cxt_component(size_t cxt_id, std::shared_ptr<T> ptr) {
    return contexts.at(cxt_id).add_component<T>(ptr);
  }

 private:
  size_t nb_cxts{};
  // TODO(antonin)
  // Context is not-movable, but is default-constructible, so I can put it in a
  // std::vector
  std::vector<Context> contexts{};

  bool enable_swap{false};

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  std::unordered_map<std::type_index, std::shared_ptr<void> > components{};

  std::set<header_field_pair> required_fields{};
  std::set<header_field_pair> arith_fields{};

  int thrift_port{};

  int device_id{};

  // same transport used for all notifications, irrespective of the thread, made
  // possible by multi-threading support in nanomsg
  std::string notifications_addr{};
  std::shared_ptr<TransportIface> notifications_transport{nullptr};
};

// convenience class for targets with a single "context"
class Switch : public SwitchWContexts {
 public:
  explicit Switch(bool enable_swap = false);

  std::unique_ptr<Packet> new_packet_ptr(int ingress_port,
                                         packet_id_t id, int ingress_length,
                                         // cpplint false positive
                                         // NOLINTNEXTLINE(whitespace/operators)
                                         PacketBuffer &&buffer) {
    return std::unique_ptr<Packet>(new Packet(
        0u, ingress_port, id, 0u, ingress_length, std::move(buffer),
        phv_source.get()));
  }

  Packet new_packet(int ingress_port, packet_id_t id, int ingress_length,
                    // cpplint false positive
                    // NOLINTNEXTLINE(whitespace/operators)
                    PacketBuffer &&buffer) {
    return Packet(0u, ingress_port, id, 0u, ingress_length, std::move(buffer),
                  phv_source.get());
  }

  Pipeline *get_pipeline(const std::string &name) {
    return get_context(0)->get_pipeline(name);
  }

  Parser *get_parser(const std::string &name) {
    return get_context(0)->get_parser(name);
  }

  Deparser *get_deparser(const std::string &name) {
    return get_context(0)->get_deparser(name);
  }

  FieldList *get_field_list(const p4object_id_t field_list_id) {
    return get_context(0)->get_field_list(field_list_id);
  }

  // Added for testing, other "object types" can be added if needed
  p4object_id_t get_table_id(const std::string &name) {
    return get_context(0)->get_table_id(name);
  }

  p4object_id_t get_action_id(const std::string &name) {
    return get_context(0)->get_action_id(name);
  }

  // to avoid C++ name hiding
  using SwitchWContexts::get_learn_engine;
  LearnEngine *get_learn_engine() {
    return get_learn_engine(0);
  }

  // to avoid C++ name hiding
  using SwitchWContexts::get_ageing_monitor;
  AgeingMonitor *get_ageing_monitor() {
    return get_ageing_monitor(0);
  }

  template<typename T>
  bool add_component(std::shared_ptr<T> ptr) {
    return add_cxt_component<T>(0, std::move(ptr));
  }

  template<typename T>
  std::shared_ptr<T> get_component() {
    return get_cxt_component<T>(0);
  }
};

}  // namespace bm

#endif  // BM_SIM_INCLUDE_BM_SIM_SWITCH_H_
