/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_PHV_ADD_ALIAS_ALLOCATION_H_
#define BF_P4C_PHV_ADD_ALIAS_ALLOCATION_H_

#include "bf-p4c/phv/phv_fields.h"

namespace PHV {

/**
 * @brief Create allocation objects (PHV::AllocSlice) for alias source
 * fields in preparation for assembly output
 * @pre PhvAnalysis_Pass has been run so that allocation objects are available.
 */
class AddAliasAllocation : public Inspector {
    PhvInfo &phv;
    ordered_set<const PHV::Field *> seen;

    /// Set @p source allocation to that of the @p range of @p dest.  The size of
    /// @p range must match the size of @p source.
    void addAllocation(PHV::Field *source, PHV::Field *dest, le_bitrange range);

    profile_t init_apply(const IR::Node *root) override {
        seen.clear();
        return Inspector::init_apply(root);
    }
    bool preorder(const IR::BFN::AliasMember *) override;
    bool preorder(const IR::BFN::AliasSlice *) override;
    void end_apply() override;

 public:
    explicit AddAliasAllocation(PhvInfo &p) : phv(p) {}
};

}  // namespace PHV

#endif /* BF_P4C_PHV_ADD_ALIAS_ALLOCATION_H_ */
