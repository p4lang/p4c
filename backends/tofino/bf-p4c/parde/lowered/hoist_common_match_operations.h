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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_HOIST_COMMON_MATCH_OPERATIONS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_HOIST_COMMON_MATCH_OPERATIONS_H_

#include "bf-p4c/device.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "compute_lowered_parser_ir.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief After parser lowering, we have converted the parser IR from P4 semantic (action->match)
 * to hardware semantic (match->action), there may still be opportunities where we can merge
 * matches where we could not before lowering (without breaking the P4 semantic).
 */
class HoistCommonMatchOperations : public ParserTransform {
    CollectLoweredParserInfo &parser_info;
    const ComputeLoweredParserIR &computed;
    ClotInfo &clot_info;

    assoc::map<const IR::BFN::LoweredParserState *, const IR::BFN::LoweredParserState *>
        original_to_modified;

    profile_t init_apply(const IR::Node *root) override;

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

    std::set<PHV::Container> get_extract_constant_dests(const IR::BFN::LoweredParserMatch *match);
    std::set<PHV::Container> get_extract_inbuf_dests(const IR::BFN::LoweredParserMatch *match);

    std::map<unsigned, unsigned> extractors_used(const IR::BFN::LoweredParserMatch *match);

    bool can_hoist(const IR::BFN::LoweredParserMatch *a, const IR::BFN::LoweredParserMatch *b);

    void do_hoist(IR::BFN::LoweredParserMatch *match, const IR::BFN::LoweredParserMatch *next);

    // do not merge loopback state for now, need to maitain loopback pointer TODO
    bool is_loopback_state(cstring state);

    IR::Node *preorder(IR::BFN::LoweredParserMatch *match) override;

 public:
    explicit HoistCommonMatchOperations(CollectLoweredParserInfo &parser_info,
                                        const ComputeLoweredParserIR &computed, ClotInfo &clot_info)
        : parser_info(parser_info), computed(computed), clot_info(clot_info) {}
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_HOIST_COMMON_MATCH_OPERATIONS_H_ */
