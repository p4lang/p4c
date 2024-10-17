/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
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
    PhvInfo& phv;
    ordered_set<const PHV::Field*> seen;

    /// Set @p source allocation to that of the @p range of @p dest.  The size of
    /// @p range must match the size of @p source.
    void addAllocation(PHV::Field* source, PHV::Field* dest, le_bitrange range);

    profile_t init_apply(const IR::Node* root) override {
        seen.clear();
        return Inspector::init_apply(root);
    }
    bool preorder(const IR::BFN::AliasMember*) override;
    bool preorder(const IR::BFN::AliasSlice*) override;
    void end_apply() override;

 public:
    explicit AddAliasAllocation(PhvInfo& p) : phv(p) { }
};

}  // namespace PHV

#endif /* BF_P4C_PHV_ADD_ALIAS_ALLOCATION_H_ */
