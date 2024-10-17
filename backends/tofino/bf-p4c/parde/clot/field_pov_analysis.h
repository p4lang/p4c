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

#ifndef EXTENSIONS_BF_P4C_PARDE_CLOT_FIELD_POV_ANALYSIS_H_
#define EXTENSIONS_BF_P4C_PARDE_CLOT_FIELD_POV_ANALYSIS_H_

#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/parde/clot/clot_info.h"

/* This pass is a parser data flow analysis to find if a field is extracted from
 * the packet in every path that sets the field's valid bit. If there exists a path
 * where valid bit is set but no fields of the corresponding header is extracted
 * from the packet then such fields should not be placed in clots.
 *
 * The analysis proceeds in topological sorted order.
 * Every state has two bitvectors :
 *    1. field_vec : Contains fields that are definitely extracted from the packet before
 *                   the current state
 *    2. pov_vec : Valid bits that are set before the current state and are waiting for
                   corresponding fields to be extracted.
 *
 * The current state will add a field in its field_vec if any of these conditions are met:
 *    1. Field is extracted from the packet in the current state
 *    2. The field is present in all the immediate predecessors' field_vec
 *
 * Subsequently, the current state will add a valid bit in its pov_vec if
 * any of these conditions are met:
 *     1. The valid bit is set in current state and the field is not present in the
          field_vec of the current state
 *     2. Valid bit is set in any immediate predecessors' pov_vec and the field is not present
 *        in the field_vec for the current state.
 *
 * If the current state transitions to end, then the povbits that are set in pov_vec
 * are added in pov_extracted_without_fields.
 */
class FieldPovAnalysis : public Inspector {
    ClotInfo& clotInfo;
    const PhvInfo& phv;
    std::set<const IR::BFN::Parser*> parsers;
    std::map<const PHV::Field*, std::set<const PHV::Field*>> pov_to_fields;

    /// Mapping of state to `<field_vec, pov_vec>`. Refer to above comment for more info.
    std::map<cstring, std::pair<bitvec, bitvec>> state_to_field_pov;
    bool preorder(const IR::BFN::Parser* parser) override;
    bool preorder(const IR::BFN::EmitField* emit) override;
    void end_apply() override;

 public:
    std::set<const PHV::Field*> pov_extracted_without_fields;
    FieldPovAnalysis(ClotInfo& clotInfo, const PhvInfo& phv) :
    clotInfo(clotInfo), phv(phv) { }
    Visitor::profile_t init_apply(const IR::Node* root) override {
        profile_t rv = Inspector::init_apply(root);
        pov_to_fields.clear();
        state_to_field_pov.clear();
        parsers.clear();
        return rv;
    }
};

#endif /* EXTENSIONS_BF_P4C_PARDE_CLOT_FIELD_POV_ANALYSIS_H_ */
