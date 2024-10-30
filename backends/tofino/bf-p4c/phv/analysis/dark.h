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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_DARK_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_DARK_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/analysis/mocha.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "ir/ir.h"

/** This pass analyses all fields used in the program and marks fields that have nondark uses in the
 * MAU (used on the input crossbar). This pass is used by two different passes:
 * CollectDarkCandidates.
 */
class CollectNonDarkUses : public MauInspector {
 private:
    const PhvInfo &phv;
    const ReductionOrInfo &red_info;
    bitvec nonDarkMauUses;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Table *) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::MAU::Action *) override;
    bool contextNeedsIXBar();

 public:
    CollectNonDarkUses(const PhvInfo &p, const ReductionOrInfo &ri) : phv(p), red_info(ri) {}

    bool hasNonDarkUse(const PHV::Field *f) const;
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
    PhvInfo &phv;
    const PhvUse &uses;
    const CollectNonDarkUses &nonDarkUses;

    /// Represents all fields with MAU uses that prevent allocation into dark containers.
    /// E.g. use on input crossbar, participation in meter operations, etc.
    bitvec nonDarkMauUses;

    /// Number of dark candidates detected.
    size_t darkCount;
    /// Size in bits of mocha candidates.
    size_t darkSize;

    profile_t init_apply(const IR::Node *root) override;
    void end_apply() override;

 public:
    explicit MarkDarkCandidates(PhvInfo &p, const PhvUse &u, const CollectNonDarkUses &d)
        : phv(p), uses(u), nonDarkUses(d), darkCount(0), darkSize(0) {}
};

class CollectDarkCandidates : public PassManager {
 private:
    CollectNonDarkUses nonDarkUses;

 public:
    CollectDarkCandidates(PhvInfo &p, PhvUse &u, const ReductionOrInfo &ri) : nonDarkUses(p, ri) {
        addPasses({&nonDarkUses, new MarkDarkCandidates(p, u, nonDarkUses)});
    }
};

#endif /*  BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_DARK_H_  */
