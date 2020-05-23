/* Copyright 2019-present Barefoot Networks, Inc.
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

#include "group_selection.h"

#include <bm/bm_sim/action_profile.h>
#include <bm/bm_sim/match_error_codes.h>

namespace pibmv2 {

using bm::MatchErrorCode;
using mbr_hdl_t = bm::ActionProfile::mbr_hdl_t;
using grp_hdl_t = bm::ActionProfile::grp_hdl_t;

MatchErrorCode
GroupSelection::activate_member(grp_hdl_t grp, mbr_hdl_t mbr) {
  std::lock_guard<std::mutex> lock(mutex);
  auto it = groups.find(grp);
  if (it == groups.end()) return MatchErrorCode::MBR_NOT_IN_GRP;
  return it->second.activate_member(mbr);
}

MatchErrorCode
GroupSelection::deactivate_member(grp_hdl_t grp, mbr_hdl_t mbr) {
  std::lock_guard<std::mutex> lock(mutex);
  auto it = groups.find(grp);
  if (it == groups.end()) return MatchErrorCode::MBR_NOT_IN_GRP;
  return it->second.deactivate_member(mbr);
}

void
GroupSelection::add_member_to_group(grp_hdl_t grp, mbr_hdl_t mbr) {
  std::lock_guard<std::mutex> lock(mutex);
  auto &group = groups[grp];
  group.add_member(mbr);
}

void
GroupSelection::remove_member_from_group(grp_hdl_t grp, mbr_hdl_t mbr) {
  std::lock_guard<std::mutex> lock(mutex);
  auto &group = groups[grp];
  group.remove_member(mbr);
  // Remove group from map when it is empty to avoid using up memory.
  if (group.size() == 0) groups.erase(grp);
}

mbr_hdl_t
GroupSelection::get_from_hash(grp_hdl_t grp, hash_t h) const {
  std::lock_guard<std::mutex> lock(mutex);
  return groups.at(grp).get_from_hash(h);
}

void
GroupSelection::reset() {
  std::lock_guard<std::mutex> lock(mutex);
  groups.clear();
}

MatchErrorCode
GroupSelection::GroupInfo::activate_member(mbr_hdl_t mbr) {
  if (members.count(mbr) == 0) return MatchErrorCode::MBR_NOT_IN_GRP;
  return (activated_members.add(mbr) == 1) ?
      MatchErrorCode::SUCCESS : MatchErrorCode::ERROR;
}

MatchErrorCode
GroupSelection::GroupInfo::deactivate_member(mbr_hdl_t mbr) {
  if (members.count(mbr) == 0) return MatchErrorCode::MBR_NOT_IN_GRP;
  return (activated_members.remove(mbr) == 1) ?
      MatchErrorCode::SUCCESS : MatchErrorCode::ERROR;
}

void
GroupSelection::GroupInfo::add_member(mbr_hdl_t mbr) {
  members.insert(mbr);
  activated_members.add(mbr);
}

void
GroupSelection::GroupInfo::remove_member(mbr_hdl_t mbr) {
  members.erase(mbr);
  activated_members.remove(mbr);
}

mbr_hdl_t
GroupSelection::GroupInfo::get_from_hash(hash_t h) const {
  auto active_count = activated_members.count();
  auto index = static_cast<size_t>(h % active_count);
  return activated_members.get_nth(index);
}

size_t
GroupSelection::GroupInfo::size() const {
  return members.size();
}

}  // namespace pibmv2
