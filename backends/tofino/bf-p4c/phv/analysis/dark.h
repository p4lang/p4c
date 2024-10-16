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

#ifndef EXTENSIONS_BF_P4C_PHV_ANALYSIS_DARK_H_
#define EXTENSIONS_BF_P4C_PHV_ANALYSIS_DARK_H_

#include "ir/ir.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/analysis/mocha.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"

/** This pass analyses all fields used in the program and marks fields that have nondark uses in the
  * MAU (used on the input crossbar). This pass is used by two different passes:
  * CollectDarkCandidates.
  */
class CollectNonDarkUses : public MauInspector {
 private:
    const PhvInfo&    phv;
    const ReductionOrInfo &red_info;
    bitvec            nonDarkMauUses;

    profile_t init_apply(const IR::Node* root) override;
    bool preorder(const IR::MAU::Table*) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::MAU::Action*) override;
    bool contextNeedsIXBar();

 public:
    CollectNonDarkUses(const PhvInfo& p, const ReductionOrInfo &ri) : phv(p), red_info(ri) { }

    bool hasNonDarkUse(const PHV::Field* f) const;
};

/** This pass analyzes all fields used in the program and marks fields suitable for allocation in
  * dark containers by setting the `is_dark_candidate()` property of the corresponding PHV::Field
  * object to true. Fields amenable for allocation to dark containers must satisfy all the
  * requirements for allocation to mocha containers, and also the following additional requirements:
  * 1. Dark fields cannot be used in the parser/deparser.
  * 2. Dark fields cannot be used on the input crossbar.
  *
  * This pass must be run after the CollectMochaCandidates pass as we use the fields marked with
  * is_mocha_candidate() as the starting point for this pass.
  */
class MarkDarkCandidates : public Inspector {
 private:
    PhvInfo&                    phv;
    const PhvUse&               uses;
    const CollectNonDarkUses&   nonDarkUses;

    /// Represents all fields with MAU uses that prevent allocation into dark containers.
    /// E.g. use on input crossbar, participation in meter operations, etc.
    bitvec          nonDarkMauUses;

    /// Number of dark candidates detected.
    size_t          darkCount;
    /// Size in bits of mocha candidates.
    size_t          darkSize;

    profile_t init_apply(const IR::Node* root) override;
    void end_apply() override;

 public:
    explicit MarkDarkCandidates(PhvInfo& p, const PhvUse& u, const CollectNonDarkUses& d)
        : phv(p), uses(u), nonDarkUses(d), darkCount(0), darkSize(0) { }
};

class CollectDarkCandidates : public PassManager {
 private:
    CollectNonDarkUses  nonDarkUses;
 public:
    CollectDarkCandidates(PhvInfo& p, PhvUse& u, const ReductionOrInfo &ri) : nonDarkUses(p, ri) {
        addPasses({
            &nonDarkUses,
            new MarkDarkCandidates(p, u, nonDarkUses)
        });
    }
};

#endif  /*  EXTENSIONS_BF_P4C_PHV_ANALYSIS_DARK_H_  */
