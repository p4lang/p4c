/* Copyright 2018-present Barefoot Networks, Inc.
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
#include <bm/bm_sim/simple_pre.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/switch.h>

#include <PI/pi_mc.h>
#include <PI/target/pi_mc_imp.h>

#include <algorithm>  // for std::reverse
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "common.h"

using ::bm::McSimplePre;
using ::bm::McSimplePreLAG;

namespace {

// returns nullptr if the switch doesn't have a registered PRE
std::shared_ptr<McSimplePre> get_pre() {
  // simple_switch and simple_switch_grpc useMcSimplePreLAG but we do not
  // support LAG functionality at the moment in PI, so we only need a
  // McSimplePre pointer.
  {
    // TODO(antonin): we should be able to call get_component here; however the
    // add_component method in the Switch class actually hides the base class
    // method and instead registers the PRE as a context component. This needs
    // to be fixes, probably by removing add_component and get_component in the
    // Switch class.
    auto pre = pibmv2::switch_->get_cxt_component<McSimplePreLAG>(0);
    if (pre != nullptr) return pre;
  }
  return pibmv2::switch_->get_cxt_component<McSimplePre>(0);
}

// From https://stackoverflow.com/a/17251989/4538702
template <typename T, typename V>
bool type_can_fit_value(const V value) {
  const auto botT = intmax_t(std::numeric_limits<T>::min());
  const auto botV = intmax_t(std::numeric_limits<V>::min());
  const auto topT = uintmax_t(std::numeric_limits<T>::max());
  const auto topV = uintmax_t(std::numeric_limits<V>::max());
  return !((botT > botV && value < static_cast<V>(botT)) ||
           (topT < topV && value > static_cast<V>(topT)));
}

// TODO(antonin): Unfortunately ports outside of the supported range
// (PORT_MAP_SIZE) will simply be silently ignored when building the PortMap
// object.
McSimplePre::PortMap convert_map(const pi_mc_port_t *eg_ports,
                                 size_t eg_ports_count) {
  std::string output;
  for (size_t i = 0; i < eg_ports_count; i++) {
    output.resize(eg_ports[i] + 1, '0');
    output[eg_ports[i]] = '1';
  }
  std::reverse(output.begin(), output.end());
  return McSimplePre::PortMap(output);
}

}  // namespace

extern "C" {

pi_status_t _pi_mc_session_init(pi_mc_session_handle_t *session_handle) {
  *session_handle = 0;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_session_cleanup(pi_mc_session_handle_t session_handle) {
  (void)session_handle;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_create(pi_mc_session_handle_t session_handle,
                              pi_dev_id_t dev_id,
                              pi_mc_grp_id_t grp_id,
                              pi_mc_grp_handle_t *grp_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  McSimplePre::mgrp_hdl_t grp_h;
  auto rc = pre->mc_mgrp_create(grp_id, &grp_h);
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  // This relies on knowledge of how the bmv2 PRE is implemented: even though
  // McSimplePre::mgrp_hdl_t is uintptr_t, the PRE always sets the handle to the
  // provided group id value.
  assert(type_can_fit_value<pi_mc_grp_handle_t>(grp_h));
  *grp_handle = grp_h;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_delete(pi_mc_session_handle_t session_handle,
                              pi_dev_id_t dev_id,
                              pi_mc_grp_handle_t grp_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  auto rc = pre->mc_mgrp_destroy(grp_handle);
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_node_create(pi_mc_session_handle_t session_handle,
                               pi_dev_id_t dev_id,
                               pi_mc_rid_t rid,
                               size_t eg_ports_count,
                               const pi_mc_port_t *eg_ports,
                               pi_mc_node_handle_t *node_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  McSimplePre::l1_hdl_t node_h;
  auto rc = pre->mc_node_create(
      rid, convert_map(eg_ports, eg_ports_count), &node_h);
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  // This relies on knowledge of how the bmv2 PRE is implemented: even though
  // McSimplePre::l1_hdl_t is uintptr_t, the PRE always uses the smallest
  // available value for node handles.
  assert(type_can_fit_value<pi_mc_node_handle_t>(node_h));
  *node_handle = node_h;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_node_modify(pi_mc_session_handle_t session_handle,
                               pi_dev_id_t dev_id,
                               pi_mc_node_handle_t node_handle,
                               size_t eg_ports_count,
                               const pi_mc_port_t *eg_ports) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  auto rc = pre->mc_node_update(
      node_handle, convert_map(eg_ports, eg_ports_count));
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_node_delete(pi_mc_session_handle_t session_handle,
                               pi_dev_id_t dev_id,
                               pi_mc_node_handle_t node_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  auto rc = pre->mc_node_destroy(node_handle);
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_attach_node(pi_mc_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_mc_grp_handle_t grp_handle,
                                   pi_mc_node_handle_t node_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  auto rc = pre->mc_node_associate(grp_handle, node_handle);
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_detach_node(pi_mc_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_mc_grp_handle_t grp_handle,
                                   pi_mc_node_handle_t node_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  auto pre = get_pre();
  if (pre == nullptr) return PI_STATUS_TARGET_ERROR;

  auto rc = pre->mc_node_dissociate(grp_handle, node_handle);
  if (rc != McSimplePre::McReturnCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

}
