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
#include <bm/bm_sim/action_profile.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/packet.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace bm {

void
ActionProfile::IndirectIndexRefCount::serialize(std::ostream *out) const {
  (*out) << mbr_count.size() << "\n";
  for (const auto c : mbr_count) (*out) << c << "\n";
  (*out) << grp_count.size() << "\n";
  for (const auto c : grp_count) (*out) << c << "\n";
}

void
ActionProfile::IndirectIndexRefCount::deserialize(std::istream *in) {
  size_t s;
  (*in) >> s;
  mbr_count.resize(s);
  for (auto &c : mbr_count) (*in) >> c;
  (*in) >> s;
  grp_count.resize(s);
  for (auto &c : grp_count) (*in) >> c;
}


void
ActionProfile::IndirectIndex::serialize(std::ostream *out) const {
  (*out) << index << "\n";
}

void
ActionProfile::IndirectIndex::deserialize(std::istream *in,
                                          const P4Objects &objs) {
  (void) objs;
  (*in) >> index;
}


MatchErrorCode
ActionProfile::GroupInfo::add_member(mbr_hdl_t mbr) {
  if (!mbrs.add(mbr)) return MatchErrorCode::MBR_ALREADY_IN_GRP;
  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
ActionProfile::GroupInfo::delete_member(mbr_hdl_t mbr) {
  if (!mbrs.remove(mbr)) return MatchErrorCode::MBR_NOT_IN_GRP;
  return MatchErrorCode::SUCCESS;
}

bool
ActionProfile::GroupInfo::contains_member(mbr_hdl_t mbr) const {
  return mbrs.contains(mbr);
}

size_t
ActionProfile::GroupInfo::size() const {
  return mbrs.count();
}

ActionProfile::mbr_hdl_t
ActionProfile::GroupInfo::get_nth(size_t n) const {
  return mbrs.get_nth(n);
}

void
ActionProfile::GroupInfo::serialize(std::ostream *out) const {
  (*out) << size() << "\n";
  for (const auto mbr : mbrs) (*out) << mbr << "\n";
}

void
ActionProfile::GroupInfo::deserialize(std::istream *in) {
  size_t s; (*in) >> s;
  for (size_t i = 0; i < s; i++) {
    mbr_hdl_t mbr; (*in) >> mbr;
    add_member(mbr);
  }
}


void
ActionProfile::GroupMgr::add_member_to_group(grp_hdl_t grp, mbr_hdl_t mbr) {
  (void) grp;
  (void) mbr;
}

void
ActionProfile::GroupMgr::remove_member_from_group(grp_hdl_t grp,
                                                  mbr_hdl_t mbr) {
  (void) grp;
  (void) mbr;
}

void
ActionProfile::GroupMgr::reset() { }

ActionProfile::mbr_hdl_t
ActionProfile::GroupMgr::get_from_hash(grp_hdl_t grp, hash_t h) const {
  const auto &group_info = groups.at(grp);
  auto s = group_info.size();
  return group_info.get_nth(h % s);
}

void
ActionProfile::GroupMgr::insert_group(grp_hdl_t grp) {
  assert(grp <= groups.size());

  if (grp == groups.size())
    groups.emplace_back();
  else
    groups[grp] = GroupInfo();
}

size_t
ActionProfile::GroupMgr::group_size(grp_hdl_t grp) const {
  return groups.at(grp).size();
}

ActionProfile::GroupInfo &
ActionProfile::GroupMgr::at(grp_hdl_t grp) {
  return groups.at(grp);
}

const ActionProfile::GroupInfo &
ActionProfile::GroupMgr::at(grp_hdl_t grp) const {
  return groups.at(grp);
}

void
ActionProfile::GroupMgr::clear() {
  groups.clear();
}


ActionProfile::ActionProfile(const std::string &name, p4object_id_t id,
                             bool with_selection)
    : NamedP4Object(name, id),
      with_selection(with_selection) { }

const ActionEntry &
ActionProfile::lookup(const Packet &pkt, const IndirectIndex &index) const {
  assert(index.is_mbr() || with_selection);

  mbr_hdl_t mbr;
  if (index.is_mbr()) {
    mbr = index.get_mbr();
  } else {
    grp_hdl_t grp = index.get_grp();
    assert(is_valid_grp(grp));
    mbr = choose_from_group(grp, pkt);
    // TODO(antonin): change to trace?
    BMLOG_DEBUG_PKT(pkt, "Choosing member {} from group {}", mbr, grp);
  }
  assert(is_valid_mbr(mbr));

  return action_entries[mbr];
}

bool
ActionProfile::has_selection() const { return with_selection; }

void
ActionProfile::entries_insert(mbr_hdl_t mbr, ActionEntry &&entry) {
  assert(mbr <= action_entries.size());

  if (mbr == action_entries.size())
    action_entries.push_back(std::move(entry));
  else
    action_entries[mbr] = std::move(entry);

  index_ref_count.set(IndirectIndex::make_mbr_index(mbr), 0);
}

MatchErrorCode
ActionProfile::add_member(const ActionFn *action_fn, ActionData action_data,
                          mbr_hdl_t *mbr) {
  auto rc = MatchErrorCode::SUCCESS;

  if (action_data.size() != action_fn->get_num_params())
    return MatchErrorCode::BAD_ACTION_DATA;
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));

  {
    auto lock = lock_write();

    uintptr_t mbr_;
    if (mbr_handles.get_handle(&mbr_)) {
      rc = MatchErrorCode::ERROR;
    } else {
      *mbr = static_cast<mbr_hdl_t>(mbr_);
      // next node is nullptr at this stage (resolved by MatchTableIndirect
      // during lookup)
      entries_insert(*mbr, ActionEntry(std::move(action_fn_entry), nullptr));
      num_members++;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Added member {} to action profile '{}'", *mbr, get_name());
  } else {
    BMLOG_ERROR("Error when trying to add member to action profile '{}'",
                get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::delete_member(mbr_hdl_t mbr) {
  auto rc = MatchErrorCode::SUCCESS;

  {
    auto lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else if (index_ref_count.get(IndirectIndex::make_mbr_index(mbr)) > 0) {
      rc = MatchErrorCode::MBR_STILL_USED;
    } else {
      int error = mbr_handles.release_handle(mbr);
      _BM_UNUSED(error);
      assert(!error);
      num_members--;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed member {} from action profile '{}'", mbr, get_name());
  } else {
    BMLOG_ERROR(
        "Error when trying to remove member {} from action profile '{}'",
        mbr, get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::modify_member(mbr_hdl_t mbr, const ActionFn *action_fn,
                                  ActionData action_data) {
  auto rc = MatchErrorCode::SUCCESS;

  if (action_data.size() != action_fn->get_num_params())
    return MatchErrorCode::BAD_ACTION_DATA;
  ActionFnEntry action_fn_entry(action_fn, std::move(action_data));

  {
    auto lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else {
      // next node is nullptr at this stage (resolved by MatchTableIndirect
      // during lookup)
      action_entries[mbr] = ActionEntry(std::move(action_fn_entry), nullptr);
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Modified member {} from action profile '{}'", mbr, get_name());
  } else {
    BMLOG_ERROR(
        "Error when trying to modify member {} from action profile '{}'",
        mbr, get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::create_group(grp_hdl_t *grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    uintptr_t grp_;
    if (grp_handles.get_handle(&grp_)) {
      rc = MatchErrorCode::ERROR;
    } else {
      *grp = static_cast<grp_hdl_t>(grp_);
      grp_mgr.insert_group(*grp);
      index_ref_count.set(IndirectIndex::make_grp_index(*grp), 0);
      num_groups++;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Created group {} in action profile '{}'", *grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to create group in action profile '{}'",
                get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::delete_group(grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else if (index_ref_count.get(IndirectIndex::make_grp_index(grp)) > 0) {
      rc = MatchErrorCode::GRP_STILL_USED;
    } else {
      // we allow deletion of non-empty groups, but we must remember to decrease
      // the ref count for the members. Note that we do not allow deletion of a
      // member which is in a group
      GroupInfo &group_info = grp_mgr.at(grp);
      for (auto mbr : group_info) {
        index_ref_count.decrease(IndirectIndex::make_mbr_index(mbr));

        grp_selector->remove_member_from_group(grp, mbr);
      }

      int error = grp_handles.release_handle(grp);
      _BM_UNUSED(error);
      assert(!error);

      num_groups--;
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed group {} from action profile '{}'", grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove group {} from action profile '{}'",
                grp, get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::add_member_to_group(mbr_hdl_t mbr, grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else {
      GroupInfo &group_info = grp_mgr.at(grp);
      rc = group_info.add_member(mbr);
      if (rc == MatchErrorCode::SUCCESS) {
        index_ref_count.increase(IndirectIndex::make_mbr_index(mbr));

        // TODO(antonin): is it an overkill to hold the lock here?
        grp_selector->add_member_to_group(grp, mbr);
      }
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Added member {} to group {} in action profile '{}'",
                mbr, grp, get_name());
  } else {
    BMLOG_ERROR(
        "Error when trying to add member {} to group {} in action profile '{}'",
        mbr, grp, get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::remove_member_from_group(mbr_hdl_t mbr, grp_hdl_t grp) {
  MatchErrorCode rc = MatchErrorCode::SUCCESS;

  {
    WriteLock lock = lock_write();

    if (!is_valid_mbr(mbr)) {
      rc = MatchErrorCode::INVALID_MBR_HANDLE;
    } else if (!is_valid_grp(grp)) {
      rc = MatchErrorCode::INVALID_GRP_HANDLE;
    } else {
      GroupInfo &group_info = grp_mgr.at(grp);
      rc = group_info.delete_member(mbr);
      if (rc == MatchErrorCode::SUCCESS) {
        index_ref_count.decrease(IndirectIndex::make_mbr_index(mbr));

        // TODO(antonin): is it an overkill to hold the lock here?
        grp_selector->remove_member_from_group(grp, mbr);
      }
    }
  }

  if (rc == MatchErrorCode::SUCCESS) {
    BMLOG_DEBUG("Removed member {} from group {} in table '{}'",
                mbr, grp, get_name());
  } else {
    BMLOG_ERROR("Error when trying to remove member {} from group {} "
                "in table '{}'", mbr, grp, get_name());
  }

  return rc;
}

MatchErrorCode
ActionProfile::get_member_(mbr_hdl_t mbr, Member *member) const {
  const ActionEntry &entry = action_entries.at(mbr);
  member->mbr = mbr;
  member->action_fn = entry.action_fn.get_action_fn();
  member->action_data = entry.action_fn.get_action_data();

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
ActionProfile::get_member(mbr_hdl_t mbr, Member *member) const {
  ReadLock lock = lock_read();

  if (!is_valid_mbr(mbr)) return MatchErrorCode::INVALID_MBR_HANDLE;
  return get_member_(mbr, member);
}

std::vector<ActionProfile::Member>
ActionProfile::get_members() const {
  ReadLock lock = lock_read();
  std::vector<Member> members(num_members);
  size_t idx = 0;
  for (const auto h : mbr_handles) {
    MatchErrorCode rc = get_member_(h, &members[idx++]);
    _BM_UNUSED(rc);
    assert(rc == MatchErrorCode::SUCCESS);
  }
  return members;
}

MatchErrorCode
ActionProfile::get_group_(grp_hdl_t grp, Group *group) const {
  group->grp = grp;
  group->mbr_handles.clear();

  for (const auto &mbr : grp_mgr.at(grp))
    group->mbr_handles.push_back(mbr);

  return MatchErrorCode::SUCCESS;
}

MatchErrorCode
ActionProfile::get_group(grp_hdl_t grp, Group *group) const {
  ReadLock lock = lock_read();
  if (!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;
  return get_group_(grp, group);
}

std::vector<ActionProfile::Group>
ActionProfile::get_groups() const {
  ReadLock lock = lock_read();
  std::vector<Group> groups(num_groups);
  size_t idx = 0;
  for (const auto h : grp_handles) {
    MatchErrorCode rc = get_group_(h, &groups[idx++]);
    _BM_UNUSED(rc);
    assert(rc == MatchErrorCode::SUCCESS);
  }
  return groups;
}

MatchErrorCode
ActionProfile::get_num_members_in_group(grp_hdl_t grp, size_t *nb) const {
  ReadLock lock = lock_read();

  if (!is_valid_grp(grp)) return MatchErrorCode::INVALID_GRP_HANDLE;

  const GroupInfo &group_info = grp_mgr.at(grp);
  *nb = group_info.size();

  return MatchErrorCode::SUCCESS;
}

size_t
ActionProfile::get_num_members() const {
  ReadLock lock = lock_read();
  return num_members;
}

size_t
ActionProfile::get_num_groups() const {
  ReadLock lock = lock_read();
  return num_groups;
}

void
ActionProfile::set_group_selector(
    std::shared_ptr<GroupSelectionIface> selector) {
  WriteLock lock = lock_write();
  grp_selector_ = selector;
  grp_selector = grp_selector_.get();
}

bool
ActionProfile::group_is_empty(grp_hdl_t grp) const {
  return grp_mgr.group_size(grp) == 0;
}

void
ActionProfile::reset_state() {
  WriteLock lock = lock_write();
  index_ref_count = IndirectIndexRefCount();
  mbr_handles.clear();
  action_entries.clear();
  num_members = 0;
  num_groups = 0;
  grp_mgr.clear();
  grp_selector->reset();
}

void
ActionProfile::serialize(std::ostream *out) const {
  ReadLock lock = lock_read();
  (*out) << action_entries.size() << "\n";
  (*out) << num_members << "\n";
  for (const auto h : mbr_handles) {
    (*out) << h << "\n";
    action_entries.at(h).serialize(out);
  }
  index_ref_count.serialize(out);
  (*out) << num_groups << "\n";
  for (const auto h : grp_handles) {
    (*out) << h << "\n";
    grp_mgr.at(h).serialize(out);
  }
}

void
ActionProfile::deserialize(std::istream *in, const P4Objects &objs) {
  ReadLock lock = lock_read();
  size_t action_entries_size; (*in) >> action_entries_size;
  action_entries.resize(action_entries_size);
  (*in) >> num_members;
  for (size_t i = 0; i < num_members; i++) {
    mbr_hdl_t mbr_hdl; (*in) >> mbr_hdl;
    int error = mbr_handles.set_handle(mbr_hdl);
    _BM_UNUSED(error);
    assert(!error);
    action_entries.at(mbr_hdl).deserialize(in, objs);
  }
  index_ref_count.deserialize(in);
  (*in) >> num_groups;
  for (size_t i = 0; i < num_groups; i++) {
    grp_hdl_t grp_hdl; (*in) >> grp_hdl;
    int error = grp_handles.set_handle(grp_hdl);
    _BM_UNUSED(error);
    assert(!error);
    grp_mgr.insert_group(grp_hdl);
    grp_mgr.at(grp_hdl).deserialize(in);
    for (const auto mbr : grp_mgr.at(grp_hdl))
      grp_selector->add_member_to_group(grp_hdl, mbr);
  }
}

void
ActionProfile::dump_entry(std::ostream *out, const IndirectIndex &index) const {
  if (index.is_mbr()) {
    *out << "Action entry: " << action_entries[index.get_mbr()] << "\n";
  } else {
    *out << "Group members:\n";
    for (const auto mbr : grp_mgr.at(index.get_grp()))
      *out << "  " << "mbr " << mbr << ": " << action_entries[mbr] << "\n";
  }
}

void
ActionProfile::ref_count_increase(const IndirectIndex &index) {
  index_ref_count.increase(index);
}

void
ActionProfile::ref_count_decrease(const IndirectIndex &index) {
  index_ref_count.decrease(index);
}

ActionProfile::mbr_hdl_t
ActionProfile::choose_from_group(grp_hdl_t grp, const Packet &pkt) const {
  if (!hash) return grp_selector->get_from_hash(grp, 0);
  hash_t h = static_cast<hash_t>(hash->output(pkt));
  return grp_selector->get_from_hash(grp, h);
}

}  // namespace bm
