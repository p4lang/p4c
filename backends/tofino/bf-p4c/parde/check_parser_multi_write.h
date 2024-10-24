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

#ifndef BF_P4C_PARDE_CHECK_PARSER_MULTI_WRITE_H_
#define BF_P4C_PARDE_CHECK_PARSER_MULTI_WRITE_H_

#include <ir/ir.h>

#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/parde/parser_query.h"
#include "ir/pass_manager.h"

using namespace P4;

/**
 * @ingroup parde
 * @brief Checks multiple writes to the same field on non-mutually exclusive paths.
 */

struct CheckParserMultiWrite : public PassManager {
    const PhvInfo &phv;

    explicit CheckParserMultiWrite(const PhvInfo &phv);
};

class PhvInfo;
class MapFieldToParserStates;

/// Check if fields that share the same byte on the wire have conflicting
/// parser write semantics.
struct CheckWriteModeConsistency : public ParserTransform {
 protected:
    const PhvInfo &phv;
    const MapFieldToParserStates &field_to_states;
    const ParserQuery pq;

    std::map<const IR::BFN::ParserPrimitive *, IR::BFN::ParserWriteMode> extract_to_write_mode;
    std::map<std::pair<PHV::FieldSlice, PHV::FieldSlice>, bool> compatability;

    /**
     * @brief Check that the write modes of all extracts are consistent.
     */
    bool check(const std::vector<const IR::BFN::Extract *> extracts) const;

    /**
     * @brief Check and adjust the write modes of all extracts to be consistent.
     *
     * Checks whether the write modes of all extracts are consistent. If not, record the adjustments
     * necessary to make them consistent, or produce an error if they can't be made consistent.
     */
    void check_and_adjust(const std::vector<const IR::BFN::Extract *> extracts);

    /**
     * Find all extracts that stard/end in mid byte and add them to a map by the partial byte
     * they create.
     * Ideally, we would check together only fields that actually lead to the same PHV. But
     * this would require doing this analysis only after fields are sliced and PHV allocated.
     * That currently cannot be done as the PHV allocation uses write modes as input.
     * A proper solution might be to integrate this check/modification of write modes to PHV
     * allocation.
     *
     * Meanwhile, we approximate by looking for generations of extracted bits -- we go over
     * the extracts in the order they appear in the state and every time we encounter bit that
     * is not higher than the last already extracted, we start a new generation of extracts.
     * Then we only look for conflicts within the same generation. This should resolve code
     * like pkt.extract(hdrs.a)
     * meta.x = pkt.hdr.a.x
     * that does actually generate two extracts of `x` to different fields.
     * NOTE: the ranges coming from one P4 extract may not be consecutive due to dead-extract
     * elimination.
     */
    void check_pre_alloc(const ordered_set<const IR::BFN::ParserPrimitive *> &state_writes);

    /**
     * @brief Verify/unify parser write mode consistency post PHV allocation.
     *
     * Verify that the parser write modes are consistent across fields. If
     * not, then attempt to unify the write modes.
     */
    void check_post_alloc();

    profile_t init_apply(const IR::Node *root) override;
    IR::Node *preorder(IR::BFN::Extract *extract) override;
    IR::Node *preorder(IR::BFN::ParserChecksumWritePrimitive *pcw) override;

    template <typename T>
    IR::Node *set_write_mode(T *write);

 public:
    CheckWriteModeConsistency(const PhvInfo &p, const MapFieldToParserStates &fs,
                              const CollectParserInfo &pi)
        : phv(p), field_to_states(fs), pq(pi, fs) {}

    /**
     * Check if the extracts for two slices can be made compatible
     */
    bool check_compatability(const PHV::FieldSlice &slice_a, const PHV::FieldSlice &slice_b);
};

#endif /*BF_P4C_PARDE_CHECK_PARSER_MULTI_WRITE_H_*/
