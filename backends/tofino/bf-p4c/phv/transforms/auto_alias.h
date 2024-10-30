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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_TRANSFORMS_AUTO_ALIAS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_TRANSFORMS_AUTO_ALIAS_H_

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_alias.h"
#include "bf-p4c/phv/pragma/pa_no_overlay.h"
#include "lib/bitvec.h"

class DetermineCandidateHeaders : public Inspector {
 private:
    const PhvInfo &phv;
    ordered_map<cstring, ordered_set<const IR::MAU::Action *>> headers;
    ordered_set<cstring> headersValidatedInParser;
    ordered_map<cstring, ordered_set<const IR::MAU::Action *>> headersValidatedInMAU;
    ordered_set<cstring> allHeaders;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Extract *extract) override;
    bool preorder(const IR::MAU::Instruction *inst) override;
    void end_apply() override;

 public:
    explicit DetermineCandidateHeaders(const PhvInfo &p) : phv(p) {}

    const ordered_set<cstring> getCandidateHeaders() const {
        ordered_set<cstring> headerSet;
        for (auto &kv : headers) headerSet.insert(kv.first);
        return headerSet;
    }

    bool isCandidateHeader(cstring header) const { return headers.count(header); }

    const ordered_set<const IR::MAU::Action *> &getActionsForCandidateHeader(cstring header) const {
        static ordered_set<const IR::MAU::Action *> actions;
        if (!isCandidateHeader(header)) return actions;
        return headers.at(header);
    }
};

struct MapDestToInstruction : public Inspector {
    ordered_map<const PHV::Field *, ordered_set<const IR::MAU::Instruction *>> dest_to_inst;

    const PhvInfo &phv;

    bool preorder(const IR::MAU::Instruction *inst) override {
        if (inst->operands.empty()) return true;
        auto dest = phv.field(inst->operands[0]);
        if (!dest) return true;
        dest_to_inst[dest].insert(inst);
        return true;
    }

    explicit MapDestToInstruction(const PhvInfo &p) : phv(p) {}
};

class DetermineCandidateFields : public Inspector {
 private:
    const PhvInfo &phv;
    const DetermineCandidateHeaders &headers;
    PragmaAlias &pragma;
    PragmaNoOverlay &no_overlay;
    const MapDestToInstruction &d2i;

    ordered_set<const PHV::Field *> initialCandidateSet;
    ordered_map<const PHV::Field *,
                ordered_map<const PHV::Field *, ordered_set<const IR::MAU::Action *>>>
        candidateSources;

    inline void dropFromCandidateSet(const PHV::Field *field);
    bool multipleSourcesFound(const PHV::Field *dest, const PHV::Field *src) const;
    bool incompatibleConstraints(const PHV::Field *dest, const PHV::Field *src) const;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Instruction *inst) override;
    void end_apply() override;

 public:
    explicit DetermineCandidateFields(const PhvInfo &p, const DetermineCandidateHeaders &h,
                                      PragmaAlias &pa, PragmaNoOverlay &no_ovrl,
                                      const MapDestToInstruction &d)
        : phv(p), headers(h), pragma(pa), no_overlay(no_ovrl), d2i(d) {}
};

class AutoAlias : public PassManager {
 private:
    MapDestToInstruction d2i;

    DetermineCandidateHeaders headers;
    DetermineCandidateFields fields;

 public:
    explicit AutoAlias(const PhvInfo &phv, PragmaAlias &pa, PragmaNoOverlay &no_ovrl)
        : d2i(phv), headers(phv), fields(phv, headers, pa, no_ovrl, d2i) {
        addPasses({&d2i, &headers, &fields});
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_TRANSFORMS_AUTO_ALIAS_H_ */
