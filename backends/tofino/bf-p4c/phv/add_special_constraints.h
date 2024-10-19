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

#ifndef BF_P4C_PHV_ADD_SPECIAL_CONSTRAINTS_H_
#define BF_P4C_PHV_ADD_SPECIAL_CONSTRAINTS_H_

#include "bf-p4c/parde/decaf.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "ir/ir.h"

/** This class is for adding all special constraints (sometimes the result of workarounds to get
 * around limitations of other parts of the compiler).
 * Current special constraints are:
 * 1. Mirror ID and mirror source fields are set to no_split, deparsed_bottom_bits() and also
 *    assigned to 16-bit containers.
 * 2. All destinations of meter color are pinned to 8-bit containers.
 *
 * Note that the above special constraints induce two artificial (overly conservative) constraints.
 * The true constraints are:
 * 1. Mirror ID field can be put into the lower bits of either a 16-bit or a 32-bit container.
 * 2. Destinations of meter color do not need to be in 8-bit containers.
 */
class AddSpecialConstraints : public Inspector {
 private:
    PhvInfo &phv_i;
    /// Pragma Object reference to set container size constraints.
    PHV::Pragmas &pragmas_i;
    /// ActionPhvConstraints reference, used to extract destinations of meter color.
    const ActionPhvConstraints &actions_i;
    const DeparserCopyOpt &decaf_i;

    /// List of metadata instances that we've already processed
    std::set<cstring> seen_hdr_i;

    profile_t init_apply(const IR::Node *root) override;

    /// Checksum related fields can go either in an 8-bit container or a 16-bit container.
    /// Currently, we do not have the infrastructure to specify multiple options for
    /// pa_container_size pragmas. Therefore, we are just allocating these fields to 16-bit
    /// containers.
    bool preorder(const IR::BFN::ChecksumVerify *verify) override;
    bool preorder(const IR::BFN::ChecksumResidualDeposit *get) override;

    /// Track upcasting
    bool preorder(const IR::Concat *) override;
    bool preorder(const IR::Cast *) override;

    bool preorder(const IR::ConcreteHeaderRef *hr) override;

    void end_apply() override;

 public:
    explicit AddSpecialConstraints(PhvInfo &phv, PHV::Pragmas &pragmas,
                                   const ActionPhvConstraints &actions,
                                   const DeparserCopyOpt &decaf)
        : phv_i(phv), pragmas_i(pragmas), actions_i(actions), decaf_i(decaf) {}
};

#endif /*  BF_P4C_PHV_ADD_SPECIAL_CONSTRAINTS_H_    */
