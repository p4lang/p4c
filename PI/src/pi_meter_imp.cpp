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
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/switch.h>

#include <PI/p4info.h>
#include <PI/pi.h>
#include <PI/target/pi_meter_imp.h>

#include <iostream>
#include <string>
#include <vector>

#include "common.h"
#include "direct_res_spec.h"

namespace {

std::string get_direct_t_name(const pi_p4info_t *p4info, pi_p4_id_t m_id) {
  pi_p4_id_t t_id = pi_p4info_meter_get_direct(p4info, m_id);
  // guaranteed by PI common code
  assert(t_id != PI_INVALID_ID);
  return std::string(pi_p4info_table_name_from_id(p4info, t_id));
}

pi_status_t convert_error_code(bm::Meter::MeterErrorCode error_code) {
  return static_cast<pi_status_t>(
      PI_STATUS_TARGET_ERROR + static_cast<int>(error_code));
}

}  // namespace

extern "C" {

pi_status_t _pi_meter_read(pi_session_handle_t session_handle,
                           pi_dev_tgt_t dev_tgt,
                           pi_p4_id_t meter_id,
                           size_t index,
                           pi_meter_spec_t *meter_spec) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string m_name(pi_p4info_meter_name_from_id(p4info, meter_id));

  std::vector<bm::Meter::rate_config_t> rates;
  auto error_code = pibmv2::switch_->meter_get_rates(0, m_name, index, &rates);
  if (error_code != bm::Meter::MeterErrorCode::SUCCESS)
    return convert_error_code(error_code);
  if (rates.empty()) return PI_STATUS_METER_SPEC_NOT_SET;
  pibmv2::convert_to_meter_spec(p4info, meter_id, meter_spec, rates);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_meter_set(pi_session_handle_t session_handle,
                          pi_dev_tgt_t dev_tgt,
                          pi_p4_id_t meter_id,
                          size_t index,
                          const pi_meter_spec_t *meter_spec) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string m_name(pi_p4info_meter_name_from_id(p4info, meter_id));

  auto rates = pibmv2::convert_from_meter_spec(meter_spec);
  auto error_code = pibmv2::switch_->meter_set_rates(0, m_name, index, rates);
  if (error_code != bm::Meter::MeterErrorCode::SUCCESS)
    return convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_meter_read_direct(pi_session_handle_t session_handle,
                                  pi_dev_tgt_t dev_tgt, pi_p4_id_t meter_id,
                                  pi_entry_handle_t entry_handle,
                                  pi_meter_spec_t *meter_spec) {
  _BM_UNUSED(session_handle);

  // bmv2 currently does not support direct neters for default entries
  if (entry_handle == pibmv2::get_default_handle()) {
    bm::Logger::get()->error(
        "[PI] bmv2 does not support direct meters for default entries");
    return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
  }

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string t_name = get_direct_t_name(p4info, meter_id);

  std::vector<bm::Meter::rate_config_t> rates;
  auto error_code = pibmv2::switch_->mt_get_meter_rates(
      0, t_name, entry_handle, &rates);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  if (rates.empty()) return PI_STATUS_METER_SPEC_NOT_SET;
  pibmv2::convert_to_meter_spec(p4info, meter_id, meter_spec, rates);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_meter_set_direct(pi_session_handle_t session_handle,
                                 pi_dev_tgt_t dev_tgt, pi_p4_id_t meter_id,
                                 pi_entry_handle_t entry_handle,
                                 const pi_meter_spec_t *meter_spec) {
  _BM_UNUSED(session_handle);

  // bmv2 currently does not support direct neters for default entries
  if (entry_handle == pibmv2::get_default_handle()) {
    bm::Logger::get()->error(
        "[PI] bmv2 does not support direct meters for default entries");
    return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
  }

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string t_name = get_direct_t_name(p4info, meter_id);

  auto rates = pibmv2::convert_from_meter_spec(meter_spec);
  auto error_code = pibmv2::switch_->mt_set_meter_rates(
      0, t_name, entry_handle, rates);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

}
