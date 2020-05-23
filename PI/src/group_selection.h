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

#ifndef SRC_GROUP_SELECTION_H_
#define SRC_GROUP_SELECTION_H_

#include <bm/bm_sim/action_profile.h>
#include <bm/bm_sim/match_error_codes.h>
#include <bm/bm_sim/ras.h>

#include <unordered_map>
#include <unordered_set>

namespace pibmv2 {

class GroupSelection : public bm::ActionProfile::GroupSelectionIface {
 public:
  using mbr_hdl_t = bm::ActionProfile::mbr_hdl_t;
  using grp_hdl_t = bm::ActionProfile::grp_hdl_t;
  using MatchErrorCode = bm::MatchErrorCode;

  MatchErrorCode activate_member(grp_hdl_t grp, mbr_hdl_t mbr);
  MatchErrorCode deactivate_member(grp_hdl_t grp, mbr_hdl_t mbr);

 private:
  using hash_t = bm::ActionProfile::hash_t;

  void add_member_to_group(grp_hdl_t grp, mbr_hdl_t mbr) override;
  void remove_member_from_group(grp_hdl_t grp, mbr_hdl_t mbr) override;

  mbr_hdl_t get_from_hash(grp_hdl_t grp, hash_t h) const override;

  void reset() override;

  class GroupInfo {
   public:
    MatchErrorCode activate_member(mbr_hdl_t mbr);
    MatchErrorCode deactivate_member(mbr_hdl_t mbr);
    void add_member(mbr_hdl_t mbr);
    void remove_member(mbr_hdl_t mbr);
    mbr_hdl_t get_from_hash(hash_t h) const;
    size_t size() const;

   private:
    bm::RandAccessUIntSet activated_members{};
    std::unordered_set<mbr_hdl_t> members;
  };

  mutable std::mutex mutex{};
  std::unordered_map<grp_hdl_t, GroupInfo> groups{};
};

}  // namespace pibmv2

#endif  // SRC_GROUP_SELECTION_H_
