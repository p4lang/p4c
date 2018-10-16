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
#include <bm/bm_sim/learning.h>
#include <bm/bm_sim/switch.h>

#include <PI/p4info/digests.h>
#include <PI/target/pi_learn_imp.h>

#include "common.h"

namespace {

pi_status_t get_bm_list_id(const pi_p4info_t *p4info,
                           bm::LearnEngineIface *learn_engine,
                           pi_p4_id_t learn_id,
                           bm::LearnEngineIface::list_id_t *list_id) {
  const char *name = pi_p4info_digest_name_from_id(p4info, learn_id);
  if (name == nullptr) return PI_STATUS_TARGET_ERROR;
  auto rc = learn_engine->list_get_id_from_name(name, list_id);
  if (rc != bm::LearnEngineIface::LearnErrorCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

}  // namespace

extern "C" {

pi_status_t _pi_learn_config_set(pi_session_handle_t session_handle,
                                 pi_dev_id_t dev_id, pi_p4_id_t learn_id,
                                 const pi_learn_config_t *config) {
  _BM_UNUSED(session_handle);
  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  auto *learn_engine = pibmv2::switch_->get_learn_engine(0);
  assert(learn_engine != nullptr);
  bm::LearnEngineIface::list_id_t list_id;
  auto status = get_bm_list_id(p4info, learn_engine, learn_id, &list_id);
  if (status != PI_STATUS_SUCCESS) return status;
  // TODO(antonin): here we do not anything when we should be disabling
  // learning, we rely on the PI client to ignore notifications
  if (config == nullptr) return PI_STATUS_SUCCESS;
  auto max_samples = static_cast<size_t>(
      (config->max_size == 0) ? 1024 : config->max_size);
  auto timeout_ms = static_cast<unsigned int>(
      config->max_timeout_ns / 1000000.);
  {
    auto rc = learn_engine->list_set_max_samples(list_id, max_samples);
    if (rc != bm::LearnEngineIface::LearnErrorCode::SUCCESS)
      return pibmv2::convert_error_code(rc);
  }
  {
    auto rc = learn_engine->list_set_timeout(list_id, timeout_ms);
    if (rc != bm::LearnEngineIface::LearnErrorCode::SUCCESS)
      return pibmv2::convert_error_code(rc);
  }
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_learn_msg_ack(pi_session_handle_t session_handle,
                              pi_dev_id_t dev_id,
                              pi_p4_id_t learn_id,
                              pi_learn_msg_id_t msg_id) {
  _BM_UNUSED(session_handle);
  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  auto *learn_engine = pibmv2::switch_->get_learn_engine(0);
  assert(learn_engine != nullptr);
  bm::LearnEngineIface::list_id_t list_id;
  auto status = get_bm_list_id(p4info, learn_engine, learn_id, &list_id);
  if (status != PI_STATUS_SUCCESS) return status;
  auto rc = learn_engine->ack_buffer(list_id, msg_id);
  if (rc != bm::LearnEngineIface::LearnErrorCode::SUCCESS)
    return pibmv2::convert_error_code(rc);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_learn_msg_done(pi_learn_msg_t *msg) {
  delete[] msg->entries;
  delete msg;
  return PI_STATUS_SUCCESS;
}

}
