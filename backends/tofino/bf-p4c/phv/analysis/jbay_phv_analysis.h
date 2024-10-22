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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_JBAY_PHV_ANALYSIS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_JBAY_PHV_ANALYSIS_H_

#include <optional>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

class JbayPhvAnalysis : public Inspector {
 private:
    const PhvInfo &phv;
    const PhvUse &uses;
    const DependencyGraph &dg;
    // const FieldDefUse           &defuse;
    ActionPhvConstraints &actions;

    enum field_uses_t {
        PARDE = 1,
        MAU = (1 << 1),
        STAGE1 = (1 << 2),
        STAGEN = (1 << 3),
        IPXBAR = (1 << 4),
        ALU = (1 << 5)
    };

    enum phv_types_t { NORMAL = 1, MOCHA = (1 << 1), DARK = (1 << 2), TPHV = (1 << 3) };

    enum field_types_t { HEADER = 1, METADATA = (1 << 1), POV = (1 << 2) };

    enum use_types_t { MATCH = 1, READ = (1 << 1), WRITE = (1 << 2) };

    ordered_map<const PHV::Field *, unsigned> fieldUsesMap;
    ordered_map<const PHV::Field *, unsigned> phvMap;

    ordered_set<const IR::MAU::Table *> listOfTables;
    std::vector<const IR::MAU::Table *> tableStack;
    ordered_map<const IR::MAU::Table *, int> tableDepStage;

    ordered_map<const IR::MAU::Table *, ordered_set<const PHV::Field *>> tableMatches;
    ordered_map<const IR::MAU::Table *, ordered_set<const PHV::Field *>> tablePHVReads;
    ordered_map<const IR::MAU::Table *, ordered_set<const PHV::Field *>> tablePHVWrites;

    ordered_map<const PHV::Field *, ordered_map<int, unsigned>> fieldUses;
    ordered_map<const PHV::Field *, ordered_map<int, unsigned>> candidateTypes;

    ordered_map<int, size_t> stageNormal;
    ordered_map<int, size_t> stageEither;
    ordered_map<int, size_t> stageMocha;
    ordered_map<int, size_t> stageDark;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Table *tbl) override;
    bool preorder(const IR::MAU::TableKey *read) override;
    bool preorder(const IR::MAU::Action *act) override;
    void postorder(const IR::MAU::Table *tbl) override;
    void end_apply() override;

    size_t totalBits() const;
    size_t totalAllocatedBits() const;
    size_t totalPhvBits() const;
    size_t totalTPhvBits();
    size_t totalHeaderBits() const;
    size_t totalMetadataBits() const;
    size_t totalPOVBits() const;

    size_t usedInPardeMetadata() const;
    size_t usedInParde();
    size_t usedInMau() const;
    size_t usedInPardeHeader() const;
    size_t usedInPardePOV() const;

    size_t ingressPhvBits() const;
    size_t egressPhvBits() const;
    size_t ingressTPhvBits() const;
    size_t egressTPhvBits() const;

    size_t aluUseBits();
    size_t aluUseBitsHeader() const;
    size_t aluUseBitsMetadata() const;
    size_t aluUseBitsPOV() const;

    size_t matchingBitsTotal();

    size_t determineDarkCandidates();
    size_t determineMochaCandidates();

    int longestDependenceChain() const;
    void printFieldLiveness(int maxStages);

    void assessCandidacy(const PHV::Field *f, int maxStages);
    void printCandidacy(int maxStages);
    void printStagewiseStats(int maxStages);
    void fixPHVAllocation(const PHV::Field *f, int maxStages);
    void fixMochaAllocation(const PHV::Field *f, int maxStages);

    bool isPHV(const PHV::Field *f) const;
    bool isHeader(const PHV::Field *f) const;
    bool isGress(const PHV::Field *f, gress_t gress) const;

 public:
    explicit JbayPhvAnalysis(const PhvInfo &p, const PhvUse &u, const DependencyGraph &g,
                             const FieldDefUse &, ActionPhvConstraints &a)
        : phv(p), uses(u), dg(g), /* defuse(du), */ actions(a) {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_JBAY_PHV_ANALYSIS_H_ */
