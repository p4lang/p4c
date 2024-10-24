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
