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

#include <PI/int/pi_int.h>
#include <PI/int/serialize.h>
#include <PI/p4info.h>
#include <PI/pi.h>
#include <PI/target/pi_act_prof_imp.h>

#include <iostream>
#include <string>
#include <vector>

#include "action_helpers.h"
#include "common.h"
#include "device_state.h"

using mbr_hdl_t = bm::RuntimeInterface::mbr_hdl_t;
using grp_hdl_t = bm::RuntimeInterface::grp_hdl_t;

namespace {

template <typename It>
void emit_members(const pi_p4info_t *p4info, const It first, const It last,
                  pi_act_prof_fetch_res_t *res) {
  pibmv2::Buffer buffer;
  for (auto it = first; it != last; it++) {
    const auto &mbr = *it;
    emit_indirect_handle(
        buffer.extend(sizeof(s_pi_indirect_handle_t)), mbr.mbr);
    const pi_p4_id_t action_id = pi_p4info_action_id_from_name(
        p4info, mbr.action_fn->get_name().c_str());
    const auto adata_size = pi_p4info_action_data_size(p4info, action_id);
    emit_p4_id(buffer.extend(sizeof(s_pi_p4_id_t)), action_id);
    emit_uint32(buffer.extend(sizeof(uint32_t)), adata_size);
    if (adata_size > 0) {
      pibmv2::dump_action_data(p4info, buffer.extend(adata_size), action_id,
                               mbr.action_data);
    }
  }
  res->entries_members_size = buffer.size();
  res->entries_members = buffer.copy();
}

template <typename It>
void emit_groups(const pi_p4info_t *p4info, const It first, const It last,
                 pi_act_prof_fetch_res_t *res) {
  _BM_UNUSED(p4info);
  pibmv2::Buffer buffer;
  size_t num_member_handles = 0;
  for (auto it = first; it != last; it++) {
    const auto &grp = *it;
    num_member_handles += grp.mbr_handles.size();
  }
  res->num_cumulated_mbr_handles = num_member_handles;
  res->mbr_handles = new pi_indirect_handle_t[num_member_handles];

  size_t handle_offset = 0;
  for (auto it = first; it != last; it++) {
    const auto &grp = *it;
    emit_indirect_handle(buffer.extend(sizeof(s_pi_indirect_handle_t)),
                         pibmv2::IndirectHMgr::make_grp_h(grp.grp));
    const auto num_mbrs = grp.mbr_handles.size();
    emit_uint32(buffer.extend(sizeof(uint32_t)), num_mbrs);
    emit_uint32(buffer.extend(sizeof(uint32_t)), handle_offset);
    for (const auto mbr_h : grp.mbr_handles)
      res->mbr_handles[handle_offset++] = mbr_h;
  }

  res->entries_groups_size = buffer.size();
  res->entries_groups = buffer.copy();
}

}  // namespace

extern "C" {

pi_status_t _pi_act_prof_mbr_create(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t act_prof_id,
                                    const pi_action_data_t *action_data,
                                    pi_indirect_handle_t *mbr_handle) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  auto adata = pibmv2::build_action_data(action_data, p4info);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));
  std::string a_name(pi_p4info_action_name_from_id(p4info,
                                                   action_data->action_id));

  mbr_hdl_t h;
  auto error_code = pibmv2::switch_->mt_act_prof_add_member(
      0, ap_name, a_name, std::move(adata), &h);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  *mbr_handle = static_cast<pi_indirect_handle_t>(h);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_mbr_delete(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t act_prof_id,
                                    pi_indirect_handle_t mbr_handle) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  auto error_code = pibmv2::switch_->mt_act_prof_delete_member(
      0, ap_name, mbr_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_mbr_modify(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t act_prof_id,
                                    pi_indirect_handle_t mbr_handle,
                                    const pi_action_data_t *action_data) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);

  auto adata = pibmv2::build_action_data(action_data, p4info);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));
  std::string a_name(pi_p4info_action_name_from_id(p4info,
                                                   action_data->action_id));

  auto error_code = pibmv2::switch_->mt_act_prof_modify_member(
      0, ap_name, mbr_handle, a_name, std::move(adata));
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_create(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t act_prof_id,
                                    size_t max_size,
                                    pi_indirect_handle_t *grp_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(max_size);  // no bound needed / supported in bmv2

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  grp_hdl_t h;
  auto error_code = pibmv2::switch_->mt_act_prof_create_group(
      0, ap_name, &h);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  *grp_handle = pibmv2::IndirectHMgr::make_grp_h(h);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_delete(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t act_prof_id,
                                    pi_indirect_handle_t grp_handle) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  grp_handle = pibmv2::IndirectHMgr::clear_grp_h(grp_handle);
  auto error_code = pibmv2::switch_->mt_act_prof_delete_group(
      0, ap_name, grp_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_add_mbr(pi_session_handle_t session_handle,
                                     pi_dev_id_t dev_id,
                                     pi_p4_id_t act_prof_id,
                                     pi_indirect_handle_t grp_handle,
                                     pi_indirect_handle_t mbr_handle) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  grp_handle = pibmv2::IndirectHMgr::clear_grp_h(grp_handle);
  auto error_code = pibmv2::switch_->mt_act_prof_add_member_to_group(
      0, ap_name, mbr_handle, grp_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_remove_mbr(pi_session_handle_t session_handle,
                                        pi_dev_id_t dev_id,
                                        pi_p4_id_t act_prof_id,
                                        pi_indirect_handle_t grp_handle,
                                        pi_indirect_handle_t mbr_handle) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  grp_handle = pibmv2::IndirectHMgr::clear_grp_h(grp_handle);
  auto error_code = pibmv2::switch_->mt_act_prof_remove_member_from_group(
      0, ap_name, mbr_handle, grp_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_set_mbrs(pi_session_handle_t session_handle,
                                      pi_dev_id_t dev_id,
                                      pi_p4_id_t act_prof_id,
                                      pi_indirect_handle_t grp_handle,
                                      size_t num_mbrs,
                                      const pi_indirect_handle_t *mbr_handles,
                                      const bool *activate) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  _BM_UNUSED(act_prof_id);
  _BM_UNUSED(grp_handle);
  _BM_UNUSED(num_mbrs);
  _BM_UNUSED(mbr_handles);
  _BM_UNUSED(activate);
  // TODO(antonin)
  return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
}

pi_status_t _pi_act_prof_grp_activate_mbr(pi_session_handle_t session_handle,
                                          pi_dev_id_t dev_id,
                                          pi_p4_id_t act_prof_id,
                                          pi_indirect_handle_t grp_handle,
                                          pi_indirect_handle_t mbr_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  grp_handle = pibmv2::IndirectHMgr::clear_grp_h(grp_handle);
  auto lock = pibmv2::device_lock->shared_lock();
  auto error_code = pibmv2::device_state->act_prof_selection.at(act_prof_id)
      ->activate_member(grp_handle, mbr_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_deactivate_mbr(pi_session_handle_t session_handle,
                                            pi_dev_id_t dev_id,
                                            pi_p4_id_t act_prof_id,
                                            pi_indirect_handle_t grp_handle,
                                            pi_indirect_handle_t mbr_handle) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(dev_id);
  grp_handle = pibmv2::IndirectHMgr::clear_grp_h(grp_handle);
  auto lock = pibmv2::device_lock->shared_lock();
  auto error_code = pibmv2::device_state->act_prof_selection.at(act_prof_id)
      ->deactivate_member(grp_handle, mbr_handle);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_entries_fetch(pi_session_handle_t session_handle,
                                       pi_dev_tgt_t dev_tgt,
                                       pi_p4_id_t act_prof_id,
                                       pi_act_prof_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_tgt.dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  const auto members = pibmv2::switch_->mt_act_prof_get_members(0, ap_name);
  const auto groups = pibmv2::switch_->mt_act_prof_get_groups(0, ap_name);

  // members
  res->num_members = members.size();
  if (members.size() > 0)
    emit_members(p4info, members.begin(), members.end(), res);

  // groups
  res->num_groups = groups.size();
  if (groups.size() > 0)
    emit_groups(p4info, groups.begin(), groups.end(), res);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_mbr_fetch(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id, pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t mbr_handle,
                                   pi_act_prof_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  res->num_members = 0;
  res->num_groups = 0;

  bm::ActionProfile::Member member;
  auto error_code = pibmv2::switch_->mt_act_prof_get_member(
      0, ap_name, mbr_handle, &member);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  res->num_members = 1;
  emit_members(p4info, &member, &member + 1, res);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_fetch(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id, pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t grp_handle,
                                   pi_act_prof_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  const auto *p4info = pibmv2::get_device_info(dev_id);
  assert(p4info != nullptr);
  std::string ap_name(pi_p4info_act_prof_name_from_id(p4info, act_prof_id));

  res->num_members = 0;
  res->num_groups = 0;

  bm::ActionProfile::Group group;
  auto error_code = pibmv2::switch_->mt_act_prof_get_group(
      0, ap_name, grp_handle, &group);
  if (error_code != bm::MatchErrorCode::SUCCESS)
    return pibmv2::convert_error_code(error_code);
  res->num_groups = 1;
  emit_groups(p4info, &group, &group + 1, res);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_entries_fetch_done(pi_session_handle_t session_handle,
                                            pi_act_prof_fetch_res_t *res) {
  _BM_UNUSED(session_handle);

  if (res->num_members > 0) delete[] res->entries_members;
  if (res->num_groups > 0) {
    delete[] res->entries_groups;
    delete[] res->mbr_handles;
  }
  return PI_STATUS_SUCCESS;
}

int _pi_act_prof_api_support(pi_dev_id_t dev_id) {
  _BM_UNUSED(dev_id);
  return PI_ACT_PROF_API_SUPPORT_GRP_ADD_AND_REMOVE_MBR;
}

}
