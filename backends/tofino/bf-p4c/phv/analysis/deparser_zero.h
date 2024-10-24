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

#ifndef BF_P4C_PHV_ANALYSIS_DEPARSER_ZERO_H_
#define BF_P4C_PHV_ANALYSIS_DEPARSER_ZERO_H_

#include <optional>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_deparser_zero.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

/** This class is meant to identify fields suitable for the deparser zero optimization.
 * Deparser zero optimization optimizes packet headers that are not parsed but are added in a
 * particular MAU pipeline to be stored in the same 8-bit PHV container. This optimization applies
 * to fields that:
 * 1. are not parsed but are deparsed.
 * 2. are never read in the MAU.
 * 3. are never written in the MAU, or only written to a guaranteed 0.
 */
class IdentifyDeparserZeroCandidates : public Inspector {
 private:
    PhvInfo &phv;
    const FieldDefUse &defuse;
    const ReductionOrInfo &red_info;
    const PragmaDeparserZero &pragmaFields;
    ordered_set<const PHV::Field *> &candidateFields;

    /// Fields that are read in the MAU.
    bitvec mauReadFields;
    /// Fields that are written in the MAU.
    bitvec mauWrittenFields;
    /// Fields that are written to zero in the MAU in some action.
    bitvec mauWrittenToZeroFields;
    /// Fields that are written to non-zero values in the MAU in some action.
    bitvec mauWrittenToNonZeroFields;
    /// Fields that are written using non-set operations in some action.
    bitvec mauWrittenByNonSets;

    /// Removes all the fields that share the same byte with non deparser zero candidates.
    void eliminateNonByteAlignedFields();

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Action *act) override;
    bool preorder(const IR::BFN::DigestFieldList *) override;
    void end_apply() override;

 public:
    explicit IdentifyDeparserZeroCandidates(PhvInfo &p, const FieldDefUse &d,
                                            const ReductionOrInfo &ri, const PragmaDeparserZero &f,
                                            ordered_set<const PHV::Field *> &c)
        : phv(p), defuse(d), red_info(ri), pragmaFields(f), candidateFields(c) {}
};

/** Remove all extracts and initializations for the deparsed zero fields.
 */
class ImplementDeparserZero : public Transform {
 private:
    PhvInfo &phv;
    const ordered_set<const PHV::Field *> &candidateFields;
    const ClotInfo &clots;

    IR::Node *preorder(IR::BFN::Extract *extract) override;
    IR::Node *preorder(IR::MAU::Instruction *inst) override;

 public:
    explicit ImplementDeparserZero(PhvInfo &p, const ordered_set<const PHV::Field *> &c,
                                   const ClotInfo &ci)
        : phv(p), candidateFields(c), clots(ci) {}
};

class DeparserZeroOptimization : public PassManager {
 private:
    ordered_set<const PHV::Field *> candidateFields;

 public:
    explicit DeparserZeroOptimization(PhvInfo &p, const FieldDefUse &d, const ReductionOrInfo &ri,
                                      const PragmaDeparserZero &pf, const ClotInfo &c);
};

#endif /* BF_P4C_PHV_ANALYSIS_DEPARSER_ZERO_H_ */
