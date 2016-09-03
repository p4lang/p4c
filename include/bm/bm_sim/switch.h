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

//! @file switch.h
//! This file contains 2 classes: bm::SwitchWContexts and bm::Switch. When
//! implementing your target, you need to subclass one of them. By subclassing
//! bm::SwitchWContexts, you will be able to write a target containing an
//! arbitary number of bm::Context objects. For a detailed description of what a
//! bm::Context is, please read the context.h documentation. However, many
//! targets don't require the notion of bm::Context, which is why we also
//! provide the bm::Switch class. The bm::Switch class inherits from
//! bm::SwitchWContexts. Because it assumes that your switch will only use a
//! single bm::Context, the very notion of context can be removed from the
//! bm::Switch class and its dataplane APIs. However, because we offer unified
//! runtime APIs, you will have to use a context id of `0` when programming the
//! tables, even when your switch class inherits from bm::Switch and not
//! bm::SwitchWContexts.
//! The simple switch target only supports one bm::Context and inherits from
//! bm::Switch.
//!
//! When subclassing on of these two classes, you need to remember to implement
//! the two pure virtual functions:
//! bm::SwitchWContexts::receive(int port_num, const char *buffer, int len) and
//! bm::SwitchWContexts::start_and_return(). Your receive() implementation will
//! be called for you every time a new packet is received by the device. In your
//! start_and_return() function, you are supposed to start the different
//! processing threads of your target switch and return immediately. Note that
//! start_and_return() should not be mandatory per se (the target designer could
//! do the initialization any way he wants, even potentially in the
//! constructor). However, we have decided to keep it around for now.
//!
//! Both switch classes support live swapping of P4-JSON configurations. To
//! enable it you need to provide the correct flag to the constructor (see
//! bm::SwitchWContexts::SwitchWContexts()). Swaps are ordered through the
//! runtime interfaces. However, it is the target switch responsibility to
//! decide when to "commit" the swap. This is because swapping configurations
//! invalidate certain pointers that you may still be using. We plan on trying
//! to make this more target-friendly in the future but it is not trivial. Here
//! is an example of how the simple router target implements swapping:
//! @code
//! // swap is enabled, so update pointers if needed
//! if (this->do_swap() == 0) {  // a swap took place
//!   ingress_mau = this->get_pipeline("ingress");
//!   egress_mau = this->get_pipeline("egress");
//!   parser = this->get_parser("parser");
//!   deparser = this->get_deparser("deparser");
//! }
//! @endcode

#ifndef BM_BM_SIM_SWITCH_H_
#define BM_BM_SIM_SWITCH_H_

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
#include "lookup_structures.h"
#include "target_parser.h"

namespace bm {

// multiple inheritance in accordance with Google C++ guidelines:
// "Multiple inheritance is allowed only when all superclasses, with the
// possible exception of the first one, are pure interfaces. In order to ensure
// that they remain pure interfaces, they must end with the Interface suffix."
//
//! Base class for a switch implemenattion where multi-context support is
//! required.
class SwitchWContexts : public DevMgr, public RuntimeInterface {
  friend class Switch;

 public:
  //! To enable live swapping of P4-JSON configurations, enable_swap needs to be
  //! set to `true`. See switch.h documentation for more information on
  //! configuration swap.
  explicit SwitchWContexts(size_t nb_cxts = 1u, bool enable_swap = false);

  // TODO(antonin): return reference instead?
  //! Access a Context by context id, throws a std::out_of_range exception if
  //! \p cxt_id is invalid.
  Context *get_context(size_t cxt_id = 0u) {
    return &contexts.at(cxt_id);
  }

  //! Your implementation will be called every time a new packet is received by
  //! the device.
  virtual int receive(int port_num, const char *buffer, int len) = 0;

  //! Do all your initialization in this function (e.g. start processing
  //! threads) and call this function when you are ready to process packets.
  virtual void start_and_return() = 0;

  //! You can override this method in your target. It will be called whenever
  //! reset_state() is invoked by the control plane. For example, the
  //! simple_switch target uses this to reset PRE state.
  virtual void reset_target_state() { }

  //! Returns the Thrift port used for the runtime RPC server.
  int get_runtime_port() const { return thrift_port; }

  //! Returns the device id for this switch instance.
  int get_device_id() const { return device_id; }

  //! Returns the nanomsg IPC address for this switch.
  std::string get_notifications_addr() const { return notifications_addr; }

  // Returns empty string ("") if debugger disabled
  std::string get_debugger_addr() const;

  // Returns empty string ("") if event logger disabled
  std::string get_event_logger_addr() const;

  //! Enable JSON config swapping for the switch.
  void enable_config_swap();

  //! Disable JSON config swapping for the switch.
  void disable_config_swap();

  //! Specify that the field is required for this target switch, i.e. the field
  //! needs to be defined in the input JSON. This function is purely meant as a
  //! safeguard and you should use it for error checking. For example, the
  //! following can be found in the simple switch target constructor:
  //! @code
  //! add_required_field("standard_metadata", "ingress_port");
  //! add_required_field("standard_metadata", "packet_length");
  //! add_required_field("standard_metadata", "instance_type");
  //! add_required_field("standard_metadata", "egress_spec");
  //! add_required_field("standard_metadata", "clone_spec");
  //! @endcode
  void add_required_field(const std::string &header_name,
                          const std::string &field_name);

  //! Checks that the given field exists for context \p cxt_id, i.e. checks that
  //! the field was defined in the input JSON used to configure that context.
  bool field_exists(size_t cxt_id, const std::string &header_name,
                    const std::string &field_name) const {
    return contexts.at(cxt_id).field_exists(header_name, field_name);
  }

  //! Force arithmetic on field. No effect if field is not defined in the input
  //! JSON. For optimization reasons, only fields on which arithmetic will be
  //! performed receive the ability to perform arithmetic operations. These
  //! special fields are determined by analyzing the P4 program / the JSON
  //! input. For example, if a field is used in a primitive action call,
  //! arithmetic will be automatically enabled for this field in bmv2. Calling
  //! Field::get() on a Field instance for which arithmetic has not been abled
  //! will result in a segfault or an assert. If your target needs to enable
  //! arithmetic on a field for which arithmetic was not automatically enabled
  //! (could happen in some rare cases), you can enable it manually by calling
  //! this method.
  void force_arith_field(const std::string &header_name,
                         const std::string &field_name);

  //! Force arithmetic on all the fields of header \p header_name. No effect if
  //! the header is not defined in the input JSON. Is equivalent to calling
  //! force_arith_field() on all fields in the header. See force_arith_field()
  //! for more information.
  void force_arith_header(const std::string &header_name);

  //! Get the number of contexts included in this switch
  size_t get_nb_cxts() { return nb_cxts; }

  int init_objects(const std::string &json_path, int device_id = 0,
                   std::shared_ptr<TransportIface> notif_transport = nullptr);

  //! Initialize the switch using command line options. This function is meant
  //! to be called right after your switch instance has been constructed. For
  //! example, in the case of the standard simple switch target:
  //! @code
  //! simple_switch = new SimpleSwitch();
  //! int status = simple_switch->init_from_command_line_options(argc, argv);
  //! if (status != 0) std::exit(status);
  //! @endcode
  //! If your target has custom CLI options, you can provide a pointer \p tp to
  //! a secondary parser which implements the TargetParserIface interface. The
  //! bm::TargetParserIface::parse method will be called with the unrecognized
  //! options. Target specific options need to appear after bmv2 general
  //! options on the command line, and be separated from them by `--`. For
  //! example:
  //! @code
  //! <my_target_exe> prog.json -i 0@eth0 -- --my-option v
  //! @endcode
  int init_from_command_line_options(int argc, char *argv[],
                                     TargetParserIface *tp = nullptr);

  //! Retrieve the shared pointer to an object of type `T` previously added to
  //! the switch using add_component().
  template<typename T>
  std::shared_ptr<T> get_component() {
    const auto &search = components.find(std::type_index(typeid(T)));
    if (search == components.end()) return nullptr;
    return std::static_pointer_cast<T>(search->second);
  }

  //! Retrieve the shared pointer to an object of type `T` previously added to
  //! one of the switch contexts using add_cxt_component().
  template<typename T>
  std::shared_ptr<T> get_cxt_component(size_t cxt_id) {
    return contexts.at(cxt_id).get_component<T>();
  }

  //! Returns true if a configuration swap was requested by the control
  //! plane. See switch.h documentation for more information.
  int swap_requested();

  //! Performs a configuration swap if one was requested by the control
  //! plane. Returns `0` if a swap had indeed been requested, `1` otherwise. If
  //! a swap was requested, the method will prevent new Packet instances from
  //! being created and will block until all existing instances have been
  //! destroyed. It will then perform the swap. Care should be taken when using
  //! this function, as it invalidates some pointers that your target may still
  //! be using. See switch.h documentation for more information.
  int do_swap();

  //! Construct and return a Packet instance for the given \p cxt_id.
  std::unique_ptr<Packet> new_packet_ptr(size_t cxt_id, int ingress_port,
                                         packet_id_t id, int ingress_length,
                                         // cpplint false positive
                                         // NOLINTNEXTLINE(whitespace/operators)
                                         PacketBuffer &&buffer) {
    boost::shared_lock<boost::shared_mutex> lock(ongoing_swap_mutex);
    return std::unique_ptr<Packet>(new Packet(
        cxt_id, ingress_port, id, 0u, ingress_length, std::move(buffer),
        phv_source.get()));
  }

  //! @copydoc new_packet_ptr
  Packet new_packet(size_t cxt_id, int ingress_port, packet_id_t id,
                    // cpplint false positive
                    // NOLINTNEXTLINE(whitespace/operators)
                    int ingress_length, PacketBuffer &&buffer) {
    boost::shared_lock<boost::shared_mutex> lock(ongoing_swap_mutex);
    return Packet(cxt_id, ingress_port, id, 0u, ingress_length,
                  std::move(buffer), phv_source.get());
  }

  //! Obtain a pointer to the LearnEngine for a given Context
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

  MatchTableType
  mt_get_type(size_t cxt_id, const std::string &table_name) const override {
    return contexts.at(cxt_id).mt_get_type(table_name);
  }

  std::vector<MatchTable::Entry>
  mt_get_entries(size_t cxt_id, const std::string &table_name) const override {
    return contexts.at(cxt_id).mt_get_entries<MatchTable>(table_name);
  }

  std::vector<MatchTableIndirect::Entry>
  mt_indirect_get_entries(size_t cxt_id,
                          const std::string &table_name) const override {
    return contexts.at(cxt_id).mt_get_entries<MatchTableIndirect>(table_name);
  }

  std::vector<MatchTableIndirectWS::Entry>
  mt_indirect_ws_get_entries(size_t cxt_id,
                             const std::string &table_name) const override {
    return contexts.at(cxt_id).mt_get_entries<MatchTableIndirectWS>(table_name);
  }

  MatchErrorCode
  mt_get_entry(size_t cxt_id, const std::string &table_name,
               entry_handle_t handle, MatchTable::Entry *entry) const override {
    return contexts.at(cxt_id).mt_get_entry<MatchTable>(
        table_name, handle, entry);
  }

  MatchErrorCode
  mt_indirect_get_entry(size_t cxt_id, const std::string &table_name,
                        entry_handle_t handle,
                        MatchTableIndirect::Entry *entry) const override {
    return contexts.at(cxt_id).mt_get_entry<MatchTableIndirect>(
        table_name, handle, entry);
  }

  MatchErrorCode
  mt_indirect_ws_get_entry(size_t cxt_id, const std::string &table_name,
                           entry_handle_t handle,
                           MatchTableIndirectWS::Entry *entry) const override {
    return contexts.at(cxt_id).mt_get_entry<MatchTableIndirectWS>(
        table_name, handle, entry);
  }

  MatchErrorCode
  mt_get_default_entry(size_t cxt_id, const std::string &table_name,
                       MatchTable::Entry *entry) const override {
    return contexts.at(cxt_id).mt_get_default_entry<MatchTable>(
        table_name, entry);
  }

  MatchErrorCode
  mt_indirect_get_default_entry(
      size_t cxt_id, const std::string &table_name,
      MatchTableIndirect::Entry *entry) const override {
    return contexts.at(cxt_id).mt_get_default_entry<MatchTableIndirect>(
        table_name, entry);
  }

  MatchErrorCode
  mt_indirect_ws_get_default_entry(
      size_t cxt_id, const std::string &table_name,
      MatchTableIndirectWS::Entry *entry) const override {
    return contexts.at(cxt_id).mt_get_default_entry<MatchTableIndirectWS>(
        table_name, entry);
  }

  std::vector<MatchTableIndirect::Member>
  mt_indirect_get_members(size_t cxt_id,
                          const std::string &table_name) const override {
    return contexts.at(cxt_id).mt_indirect_get_members(table_name);
  }

  MatchErrorCode
  mt_indirect_get_member(size_t cxt_id, const std::string &table_name,
                         mbr_hdl_t mbr,
                         MatchTableIndirect::Member *member) const override {
    return contexts.at(cxt_id).mt_indirect_get_member(table_name, mbr, member);
  }

  std::vector<MatchTableIndirectWS::Group>
  mt_indirect_ws_get_groups(size_t cxt_id,
                            const std::string &table_name) const override {
    return contexts.at(cxt_id).mt_indirect_ws_get_groups(table_name);
  }

  MatchErrorCode
  mt_indirect_ws_get_group(size_t cxt_id, const std::string &table_name,
                           grp_hdl_t grp,
                           MatchTableIndirectWS::Group *group) const override {
    return contexts.at(cxt_id).mt_indirect_ws_get_group(
        table_name, grp, group);
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

  MatchErrorCode
  mt_get_meter_rates(
      size_t cxt_id, const std::string &table_name, entry_handle_t handle,
      std::vector<Meter::rate_config_t> *configs) override {
    return contexts.at(cxt_id).mt_get_meter_rates(table_name, handle, configs);
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

  MeterErrorCode
  meter_get_rates(size_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  std::vector<Meter::rate_config_t> *configs) override {
    return contexts.at(cxt_id).meter_get_rates(meter_name, idx, configs);
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

  RegisterErrorCode
  register_write_range(size_t cxt_id,
                       const std::string &register_name,
                       const size_t start, const size_t end,
                       Data value) override {
    return contexts.at(cxt_id).register_write_range(
        register_name, start, end, std::move(value));
  }

  RegisterErrorCode
  register_reset(size_t cxt_id, const std::string &register_name) override {
    return contexts.at(cxt_id).register_reset(register_name);
  }

  ParseVSet::ErrorCode
  parse_vset_add(size_t cxt_id, const std::string &parse_vset_name,
                 const ByteContainer &value) override {
    return contexts.at(cxt_id).parse_vset_add(parse_vset_name, value);
  }

  ParseVSet::ErrorCode
  parse_vset_remove(size_t cxt_id, const std::string &parse_vset_name,
                    const ByteContainer &value) override {
    return contexts.at(cxt_id).parse_vset_remove(parse_vset_name, value);
  }

  RuntimeInterface::ErrorCode
  reset_state() override;

  RuntimeInterface::ErrorCode
  serialize(std::ostream *out) override;

  RuntimeInterface::ErrorCode
  load_new_config(const std::string &new_config) override;

  RuntimeInterface::ErrorCode
  swap_configs() override;

  std::string get_config() const override;
  std::string get_config_md5() const override;

  // conscious choice not to use templates here (or could not use virtual)
  CustomCrcErrorCode
  set_crc16_custom_parameters(
      size_t cxt_id, const std::string &calc_name,
      const CustomCrcMgr<uint16_t>::crc_config_t &crc16_config) override;

  CustomCrcErrorCode
  set_crc32_custom_parameters(
      size_t cxt_id, const std::string &calc_name,
      const CustomCrcMgr<uint32_t>::crc_config_t &crc32_config) override;

  // ---------- End RuntimeInterface ----------

 protected:
  typedef Context::header_field_pair header_field_pair;
  typedef Context::ForceArith ForceArith;

  const std::set<header_field_pair> &get_required_fields() const {
    return required_fields;
  }

  //! Add a component to this switch. Each switch maintains a map `T` ->
  //! `shared_ptr<T>`, which maps a type (using `typeid`) to a shared pointer to
  //! an object of the same type. The pointer can be retrieved at a later time
  //! by using get_component(). This method should be used for components which
  //! are global to the switch and not specific to a Context of the switch,
  //! otherwise you can use add_cxt_component(). The pointer can be retrieved at
  //! a later time by using get_component().
  template<typename T>
  bool add_component(std::shared_ptr<T> ptr) {
    std::shared_ptr<void> ptr_ = std::static_pointer_cast<void>(ptr);
    const auto &r = components.insert({std::type_index(typeid(T)), ptr_});
    return r.second;
  }

  //! Add a component to a context of the switch. Essentially calls
  //! Context::add_component() for the correct context. This method should be
  //! used for components which are specific to a Context (e.g. you can have one
  //! packet replication engine instance per context) and not global to the
  //! switch, otherwise you can use add_component(). The pointer can be
  //! retrieved at a later time by using get_cxt_component().
  template<typename T>
  bool add_cxt_component(size_t cxt_id, std::shared_ptr<T> ptr) {
    return contexts.at(cxt_id).add_component<T>(ptr);
  }

  void
  set_lookup_factory(
      const std::shared_ptr<LookupStructureFactory> &new_factory) {
    lookup_factory = new_factory;
  }

  int deserialize(std::istream *in);
  int deserialize_from_file(const std::string &state_dump_path);

 private:
  size_t nb_cxts{};
  // TODO(antonin)
  // Context is not-movable, but is default-constructible, so I can put it in a
  // std::vector
  std::vector<Context> contexts{};

  LookupStructureFactory *get_lookup_factory() const {
    return lookup_factory ? lookup_factory.get() : &default_lookup_factory;
  }

  // internal version of get_config_md5(), which does not acquire config_lock
  std::string get_config_md5_() const;

  // Create an instance of the default lookup factory
  static LookupStructureFactory default_lookup_factory;
  // All Switches will refer to that instance unless explicitly
  // given a factory
  std::shared_ptr<LookupStructureFactory> lookup_factory{nullptr};

  bool enable_swap{false};

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  std::unordered_map<std::type_index, std::shared_ptr<void> > components{};

  std::set<header_field_pair> required_fields{};
  ForceArith arith_objects{};

  int thrift_port{};

  int device_id{};

  // same transport used for all notifications, irrespective of the thread, made
  // possible by multi-threading support in nanomsg
  std::string notifications_addr{};
  std::shared_ptr<TransportIface> notifications_transport{nullptr};

  mutable boost::shared_mutex ongoing_swap_mutex{};

  std::string current_config{};
  mutable std::mutex config_mutex{};

  std::string event_logger_addr{};
};


//! Convenience subclass of SwitchWContexts for targets with a single
//! Context. This is the base class for the standard simple switch target
//! implementation.
class Switch : public SwitchWContexts {
 public:
  //! See SwitchWContexts::SwitchWContexts()
  explicit Switch(bool enable_swap = false);

  // to avoid C++ name hiding
  using SwitchWContexts::field_exists;
  //! Checks that the given field was defined in the input JSON used to
  //! configure the switch
  bool field_exists(const std::string &header_name,
                    const std::string &field_name) const {
    return field_exists(0, header_name, field_name);
  }

  // to avoid C++ name hiding
  using SwitchWContexts::new_packet_ptr;
  //! Convenience wrapper around SwitchWContexts::new_packet_ptr() for a single
  //! context switch.
  std::unique_ptr<Packet> new_packet_ptr(int ingress_port,
                                         packet_id_t id, int ingress_length,
                                         // cpplint false positive
                                         // NOLINTNEXTLINE(whitespace/operators)
                                         PacketBuffer &&buffer) {
    return new_packet_ptr(0u, ingress_port, id, ingress_length,
                          std::move(buffer));
  }

  // to avoid C++ name hiding
  using SwitchWContexts::new_packet;
  //! Convenience wrapper around SwitchWContexts::new_packet() for a single
  //! context switch.
  Packet new_packet(int ingress_port, packet_id_t id, int ingress_length,
                    // cpplint false positive
                    // NOLINTNEXTLINE(whitespace/operators)
                    PacketBuffer &&buffer) {
    return new_packet(0u, ingress_port, id, ingress_length, std::move(buffer));
  }

  //! Return a raw, non-owning pointer to Pipeline \p name. This pointer will be
  //! invalidated if a configuration swap is performed by the target. See
  //! switch.h documentation for details.
  Pipeline *get_pipeline(const std::string &name) {
    return get_context(0)->get_pipeline(name);
  }

  //! Return a raw, non-owning pointer to Parser \p name. This pointer will be
  //! invalidated if a configuration swap is performed by the target. See
  //! switch.h documentation for details.
  Parser *get_parser(const std::string &name) {
    return get_context(0)->get_parser(name);
  }

  //! Return a raw, non-owning pointer to Deparser \p name. This pointer will be
  //! invalidated if a configuration swap is performed by the target. See
  //! switch.h documentation for details.
  Deparser *get_deparser(const std::string &name) {
    return get_context(0)->get_deparser(name);
  }

  //! Return a raw, non-owning pointer to the FieldList with id \p
  //! field_list_id. This pointer will be invalidated if a configuration swap is
  //! performed by the target. See switch.h documentation for details.
  FieldList *get_field_list(const p4object_id_t field_list_id) {
    return get_context(0)->get_field_list(field_list_id);
  }

  // Added for testing, other "object types" can be added if needed
  p4object_id_t get_table_id(const std::string &name) {
    return get_context(0)->get_table_id(name);
  }

  p4object_id_t get_action_id(const std::string &table_name,
                              const std::string &action_name) {
    return get_context(0)->get_action_id(table_name, action_name);
  }

  // to avoid C++ name hiding
  using SwitchWContexts::get_learn_engine;
  //! Obtain a pointer to the LearnEngine for this Switch instance
  LearnEngine *get_learn_engine() {
    return get_learn_engine(0);
  }

  // to avoid C++ name hiding
  using SwitchWContexts::get_ageing_monitor;
  AgeingMonitor *get_ageing_monitor() {
    return get_ageing_monitor(0);
  }

  //! Add a component to this Switch. Each Switch maintains a map `T` ->
  //! `shared_ptr<T>`, which maps a type (using `typeid`) to a shared pointer to
  //! an object of the same type. The pointer can be retrieved at a later time
  //! by using get_component().
  template<typename T>
  bool add_component(std::shared_ptr<T> ptr) {
    return add_cxt_component<T>(0, std::move(ptr));
  }

  //! Retrieve the shared pointer to an object of type `T` previously added to
  //! the Switch using add_component().
  template<typename T>
  std::shared_ptr<T> get_component() {
    return get_cxt_component<T>(0);
  }
};

}  // namespace bm

#endif  // BM_BM_SIM_SWITCH_H_
