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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_ADJUST_EXTRACT_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_ADJUST_EXTRACT_H_

#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

/**
 * @ingroup parde
 * @brief Adjusts extractions that extract from fields that are serialized from phv container,
 *        i.e. marshaled, because there might be some junk bits before and after the field.
 *
 * This is used for mirror/resubmit engine that directly pull data from phv container
 * and then send to ingress/egress parser.
 */
class AdjustExtract : public PardeModifier {
    const PhvInfo &phv;

    /// Adjust extract and shift.
    void postorder(IR::BFN::ParserState *state) override;

    /// For a marshaled field, calculate the junk bits that is also serialized.
    /// This requires that the field can be serialized consecutively, so that
    /// junk bits can only show up before and after the field.
    /// @returns a pair of size_t where the first is the size of pre_padding
    /// and the second is post_padding, in nw_order.
    std::pair<size_t, size_t> calcPrePadding(const PHV::Field *field);

    /// Whether a state has any extraction that extracts from malshaled fields.
    bool hasMarshaled(const IR::BFN::ParserState *state);

 public:
    explicit AdjustExtract(const PhvInfo &phv) : phv(phv) {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_ADJUST_EXTRACT_H_ */
