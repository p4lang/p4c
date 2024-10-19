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

#ifndef BF_P4C_MAU_REDUCTION_OR_H_
#define BF_P4C_MAU_REDUCTION_OR_H_

#include <map>

#include "bf-p4c/mau/mau_visitor.h"
#include "lib/ordered_set.h"

using namespace P4;

/**
 * A reduction or is an optimization where an binary OR operation is implemented by ORing action
 * data on the action data bus, and using the results of this in an ALU operation.  The action
 * data bus is a large OXBar, so the sending two action data sources to the same bytes on
 * the action bus results in the ORing of these two sources.  This can ORed by up to as many
 * sources are possible per stage.
 *
 * The standard use case for this is a bloom filter.  In order to reduce the dependency
 * chain, multiple stateful ALUs are ORed to the same place in the action data bus.  The
 * current way this is delineated is through an extra property on a stateful ALU,
 * "reduction_or_group".  Stateful ALUs that are in the same reduction_or_group use this
 * optimization, and thus have no penalty.
 *
 * TODO: In order to fully gain the power of this optimization, instead of looking for
 * a "reduction_or_group", the compiler could do this lookup automatically based on the P4
 * code, and the use of stateful ALU outputs in OR based AssignmentStatements.  This is a
 * holdover until we have the time to implement this.
 */
struct ReductionOrInfo {
    using SaluReductionOrGroup = std::map<cstring, ordered_set<const IR::MAU::StatefulAlu *>>;
    using TblReductionOrGroup = std::map<cstring, ordered_set<const IR::MAU::Table *>>;

    SaluReductionOrGroup salu_reduction_or_group;
    TblReductionOrGroup tbl_reduction_or_group;
    bool is_reduction_or(const IR::MAU::Instruction *, const IR::MAU::Table *,
                         cstring &red_or_key) const;

    void clear() {
        salu_reduction_or_group.clear();
        tbl_reduction_or_group.clear();
    }
};

class GatherReductionOrReqs : public MauInspector {
    Visitor::profile_t init_apply(const IR::Node *node) override;
    bool preorder(const IR::MAU::StatefulAlu *) override;
    bool preorder(const IR::MAU::Action *) override { return false; }

    ReductionOrInfo &red_or_info;

 public:
    explicit GatherReductionOrReqs(ReductionOrInfo &red_or_info) : red_or_info(red_or_info) {}
};

#endif /* BF_P4C_MAU_REDUCTION_OR_H_ */
