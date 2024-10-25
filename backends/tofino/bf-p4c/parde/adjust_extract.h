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
