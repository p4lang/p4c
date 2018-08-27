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

#include <PI/pi_clone.h>
#include <PI/pi_mc.h>
#include <PI/target/pi_clone_imp.h>

// TODO(antonin): it would probably make more sense to move this code to
// targets/simple_switch, but for now it will have to do.
#if WITH_SIMPLE_SWITCH
#include "simple_switch.h"

#include "common.h"

extern "C" {

pi_status_t _pi_clone_session_set(
    pi_session_handle_t session_handle,
    pi_dev_tgt_t dev_tgt,
    pi_clone_session_id_t clone_session_id,
    const pi_clone_session_config_t *clone_session_config) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_tgt);
  auto *sswitch = dynamic_cast<SimpleSwitch *>(pibmv2::switch_);
  if (sswitch == nullptr) return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
  SimpleSwitch::MirroringSessionConfig config = {};
  if (clone_session_config->direction != PI_CLONE_DIRECTION_BOTH)
    return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
  config.egress_port = clone_session_config->eg_port;
  config.egress_port_valid = clone_session_config->eg_port_valid;
  config.mgid = clone_session_config->mc_grp_id;
  config.mgid_valid = clone_session_config->mc_grp_id_valid;
  // TODO(antonin): add support for truncation in bmv2
  if (clone_session_config->max_packet_length != 0)
    return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
  auto success = sswitch->mirroring_add_session(
      clone_session_id, config);
  return success ? PI_STATUS_SUCCESS : PI_STATUS_TARGET_ERROR;
}

pi_status_t _pi_clone_session_reset(
    pi_session_handle_t session_handle,
    pi_dev_tgt_t dev_tgt,
    pi_clone_session_id_t clone_session_id) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_tgt);
  auto *sswitch = dynamic_cast<SimpleSwitch *>(pibmv2::switch_);
  if (sswitch == nullptr) return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
  auto success = sswitch->mirroring_delete_session(clone_session_id);
  return success ? PI_STATUS_SUCCESS : PI_STATUS_TARGET_ERROR;
}

}

#else  // WITH_SIMPLE_SWITCH

extern "C" {

pi_status_t _pi_clone_session_set(
    pi_session_handle_t session_handle,
    pi_dev_tgt_t dev_tgt,
    pi_clone_session_id_t clone_session_id,
    const pi_clone_session_config_t *clone_session_config) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_tgt);
  _BM_UNUSED(clone_session_id);
  _BM_UNUSED(clone_session_config);
  return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
}

pi_status_t _pi_clone_session_reset(
    pi_session_handle_t session_handle,
    pi_dev_tgt_t dev_tgt,
    pi_clone_session_id_t clone_session_id) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_tgt);
  _BM_UNUSED(clone_session_id);
  return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
}

}

#endif  // WITH_SIMPLE_SWITCH
