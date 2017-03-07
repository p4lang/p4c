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

#include <PI/p4info.h>
#include <PI/pi.h>
#include <PI/target/pi_counter_imp.h>

#include <iostream>
#include <string>
#include <thread>

#include "common.h"
#include "direct_res_spec.h"

namespace {

void convert_to_counter_data(pi_counter_data_t *to,
                             uint64_t bytes, uint64_t packets) {
  // with bmv2, both are always valid
  to->valid = PI_COUNTER_UNIT_PACKETS | PI_COUNTER_UNIT_BYTES;
  to->bytes = static_cast<pi_counter_value_t>(bytes);
  to->packets = static_cast<pi_counter_value_t>(packets);
}

bool are_both_values_set(const pi_counter_data_t *counter_data) {
  return (counter_data->valid & PI_COUNTER_UNIT_BYTES) &&
      (counter_data->valid & PI_COUNTER_UNIT_PACKETS);
}

void merge_current_value(pi_counter_data_t *desired_data,
                         const pi_counter_data_t *curr_data) {
  if (!(desired_data->valid & PI_COUNTER_UNIT_BYTES)) {
    assert(curr_data->valid & PI_COUNTER_UNIT_BYTES);
    desired_data->valid |= PI_COUNTER_UNIT_BYTES;
    desired_data->bytes = curr_data->bytes;
  }
  if (!(desired_data->valid & PI_COUNTER_UNIT_PACKETS)) {
    assert(curr_data->valid & PI_COUNTER_UNIT_PACKETS);
    desired_data->valid |= PI_COUNTER_UNIT_PACKETS;
    desired_data->packets = curr_data->packets;
  }
}

std::string get_direct_t_name(const pi_p4info_t *p4info, pi_p4_id_t c_id) {
  pi_p4_id_t t_id = pi_p4info_counter_get_direct(p4info, c_id);
  // guaranteed by PI common code
  assert(t_id != PI_INVALID_ID);
  return std::string(pi_p4info_table_name_from_id(p4info, t_id));
}

pi_status_t convert_error_code(bm::Counter::CounterErrorCode error_code) {
  return static_cast<pi_status_t>(
      PI_STATUS_TARGET_ERROR + static_cast<int>(error_code));
}

}  // namespace

extern "C" {

pi_status_t _pi_counter_read(pi_session_handle_t session_handle,
                             pi_dev_tgt_t dev_tgt, pi_p4_id_t counter_id,
                             size_t index, int flags,
                             pi_counter_data_t *counter_data) {
  (void)session_handle;
  (void)flags;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;
  std::string c_name(pi_p4info_counter_name_from_id(p4info, counter_id));

  uint64_t bytes, packets;
  auto error_code = pibmv2::switch_->read_counters(
      0, c_name, index, &bytes, &packets);
  if (error_code != bm::Counter::CounterErrorCode::SUCCESS)
    return convert_error_code(error_code);
  convert_to_counter_data(counter_data, bytes, packets);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_write(pi_session_handle_t session_handle,
                              pi_dev_tgt_t dev_tgt, pi_p4_id_t counter_id,
                              size_t index,
                              const pi_counter_data_t *counter_data) {
  (void)session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;
  std::string c_name(pi_p4info_counter_name_from_id(p4info, counter_id));

  // very poor man solution: bmv2 does not (yet) let us set only one of bytes /
  // packets, so we first retrieve the current data and use it
  pi_counter_data_t desired_data = *counter_data;
  if (!are_both_values_set(counter_data)) {
    pi_counter_data_t curr_data;
    pi_status_t status = _pi_counter_read(session_handle, dev_tgt,
                                          counter_id, index, 0, &curr_data);
    if (status != PI_STATUS_SUCCESS) return status;
    merge_current_value(&desired_data, &curr_data);
  }

  uint64_t bytes, packets;
  pibmv2::convert_from_counter_data(&desired_data, &bytes, &packets);
  auto error_code = pibmv2::switch_->write_counters(
      0, c_name, index, bytes, packets);
  if (error_code != bm::Counter::CounterErrorCode::SUCCESS)
    return convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_read_direct(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt, pi_p4_id_t counter_id,
                                    pi_entry_handle_t entry_handle, int flags,
                                    pi_counter_data_t *counter_data) {
  (void)session_handle;
  (void)flags;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;
  std::string t_name = get_direct_t_name(p4info, counter_id);

  uint64_t bytes, packets;
  auto error_code = pibmv2::switch_->mt_read_counters(
      0, t_name, entry_handle, &bytes, &packets);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  convert_to_counter_data(counter_data, bytes, packets);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_write_direct(pi_session_handle_t session_handle,
                                     pi_dev_tgt_t dev_tgt,
                                     pi_p4_id_t counter_id,
                                     pi_entry_handle_t entry_handle,
                                     const pi_counter_data_t *counter_data) {
  (void)session_handle;

  pibmv2::device_info_t *d_info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(d_info->assigned);
  const pi_p4info_t *p4info = d_info->p4info;
  std::string t_name = get_direct_t_name(p4info, counter_id);

  // very poor man solution: bmv2 does not (yet) let us set only one of bytes /
  // packets, so we first retrieve the current data and use it
  pi_counter_data_t desired_data = *counter_data;
  if (!are_both_values_set(counter_data)) {
    pi_counter_data_t curr_data;
    pi_status_t status = _pi_counter_read_direct(session_handle, dev_tgt,
                                                 counter_id, entry_handle, 0,
                                                 &curr_data);
    if (status != PI_STATUS_SUCCESS) return status;
    merge_current_value(&desired_data, &curr_data);
  }

  uint64_t bytes, packets;
  pibmv2::convert_from_counter_data(&desired_data, &bytes, &packets);
  auto error_code = pibmv2::switch_->mt_write_counters(
      0, t_name, entry_handle, bytes, packets);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_hw_sync(pi_session_handle_t session_handle,
                                pi_dev_tgt_t dev_tgt, pi_p4_id_t counter_id,
                                PICounterHwSyncCb cb, void *cb_cookie) {
  (void)session_handle;
  if (!cb) return PI_STATUS_SUCCESS;
  std::thread cb_thread(cb, dev_tgt.dev_id, counter_id, cb_cookie);
  cb_thread.detach();
  return PI_STATUS_SUCCESS;
}

}
