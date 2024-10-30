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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_MERGE_LOWERED_PARSER_STATES_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_MERGE_LOWERED_PARSER_STATES_H_

#include "bf-p4c/logging/phv_logging.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/lowered/compute_lowered_parser_ir.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief After parser lowering, we have converted the parser IR from P4 semantic (action->match)
 *        to HW semantic (match->action), there may still be opportunities where we can merge
 *        states where we couldn't before lowering (without breaking the P4 semantic).
 */
struct MergeLoweredParserStates : public ParserTransform {
    const CollectLoweredParserInfo &parser_info;
    const ComputeLoweredParserIR &computed;
    ClotInfo &clot;
    PhvLogging::CollectDefUseInfo *defuseInfo;

    explicit MergeLoweredParserStates(const CollectLoweredParserInfo &pi,
                                      const ComputeLoweredParserIR &computed, ClotInfo &c,
                                      PhvLogging::CollectDefUseInfo *defuseInfo)
        : parser_info(pi), computed(computed), clot(c), defuseInfo(defuseInfo) {}

    // Compares all fields in two LoweredParserMatch objects
    // except for the match values.  Essentially returns if both
    // LoweredParserMatch do the same things when matching.
    //
    // Equivalent to IR::BFN::LoweredParserMatch::operator==(IR::BFN::LoweredParserMatch const & a)
    // but considering the loop fields and with the value fields masked off.
    bool compare_match_operations(const IR::BFN::LoweredParserMatch *a,
                                  const IR::BFN::LoweredParserMatch *b);

    const IR::BFN::LoweredParserMatch *get_unconditional_match(
        const IR::BFN::LoweredParserState *state);

    struct RightShiftPacketRVal : public Modifier {
        int byteDelta = 0;
        bool oob = false;

        explicit RightShiftPacketRVal(int byteDelta) : byteDelta(byteDelta) {}

        bool preorder(IR::BFN::LoweredPacketRVal *rval) override;
    };

    // Checksum also needs the masks to be shifted
    struct RightShiftCsumMask : public Modifier {
        int byteDelta = 0;
        bool oob = false;
        bool swapMalform = false;

        explicit RightShiftCsumMask(int byteDelta) : byteDelta(byteDelta) {}

        bool preorder(IR::BFN::LoweredParserChecksum *csum) override;
    };

    bool can_merge(const IR::BFN::LoweredParserMatch *a, const IR::BFN::LoweredParserMatch *b);

    void do_merge(IR::BFN::LoweredParserMatch *match, const IR::BFN::LoweredParserMatch *next);

    // do not merge loopback state for now, need to maitain loopback pointer TODO
    bool is_loopback_state(cstring state);

    IR::Node *preorder(IR::BFN::LoweredParserMatch *match) override;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_MERGE_LOWERED_PARSER_STATES_H_ */
