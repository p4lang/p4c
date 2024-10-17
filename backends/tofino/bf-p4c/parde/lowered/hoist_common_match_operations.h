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

#ifndef EXTENSIONS_BF_P4C_PARDE_LOWERED_HOIST_COMMON_MATCH_OPERATIONS_H_
#define EXTENSIONS_BF_P4C_PARDE_LOWERED_HOIST_COMMON_MATCH_OPERATIONS_H_

#include "bf-p4c/device.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/clot/clot_info.h"

#include "compute_lowered_parser_ir.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief After parser lowering, we have converted the parser IR from P4 semantic (action->match)
 * to hardware semantic (match->action), there may still be opportunities where we can merge
 * matches where we could not before lowering (without breaking the P4 semantic).
 */
class HoistCommonMatchOperations : public ParserTransform {
    CollectLoweredParserInfo& parser_info;
    const ComputeLoweredParserIR& computed;
    ClotInfo& clot_info;

    assoc::map<const IR::BFN::LoweredParserState*, const IR::BFN::LoweredParserState*>
        original_to_modified;

    profile_t init_apply(const IR::Node* root) override;

    // Compares all fields in two LoweredParserMatch objects
    // except for the match values.  Essentially returns if both
    // LoweredParserMatch do the same things when matching.
    //
    // Equivalent to IR::BFN::LoweredParserMatch::operator==(IR::BFN::LoweredParserMatch const & a)
    // but considering the loop fields and with the value fields masked off.
    bool compare_match_operations(const IR::BFN::LoweredParserMatch* a,
                                  const IR::BFN::LoweredParserMatch* b);

    const IR::BFN::LoweredParserMatch* get_unconditional_match(
        const IR::BFN::LoweredParserState* state);

    std::set<PHV::Container> get_extract_constant_dests(const IR::BFN::LoweredParserMatch* match);
    std::set<PHV::Container> get_extract_inbuf_dests(const IR::BFN::LoweredParserMatch* match);

    std::map<unsigned, unsigned> extractors_used(const IR::BFN::LoweredParserMatch* match);

    bool can_hoist(const IR::BFN::LoweredParserMatch* a, const IR::BFN::LoweredParserMatch* b);

    void do_hoist(IR::BFN::LoweredParserMatch* match, const IR::BFN::LoweredParserMatch* next);

    // do not merge loopback state for now, need to maitain loopback pointer TODO
    bool is_loopback_state(cstring state);

    IR::Node* preorder(IR::BFN::LoweredParserMatch* match) override;

 public:
    explicit HoistCommonMatchOperations(CollectLoweredParserInfo& parser_info,
                                        const ComputeLoweredParserIR& computed,
                                        ClotInfo& clot_info)
        : parser_info(parser_info), computed(computed), clot_info(clot_info) {}
};

}  // namespace Parde::Lowered

#endif /* EXTENSIONS_BF_P4C_PARDE_LOWERED_HOIST_COMMON_MATCH_OPERATIONS_H_ */
