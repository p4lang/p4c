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

#ifndef BF_P4C_COMMON_ELIM_UNUSED_H_
#define BF_P4C_COMMON_ELIM_UNUSED_H_

#include "field_defuse.h"

using namespace P4;

class ReplaceMember : public Transform {
    const IR::BFN::AliasMember *preorder(IR::BFN::AliasMember *m) { return m; }
    const IR::BFN::AliasMember *preorder(IR::Member *m) { return new IR::BFN::AliasMember(m, m); }
};

class ElimUnused : public PassManager {
    const PhvInfo &phv;
    FieldDefUse &defuse;
    std::set<cstring> &zeroInitFields;

    class Instructions;
    class CollectEmptyTables;
    class Tables;
    class Headers;

 public:
    ElimUnused(const PhvInfo &phv, FieldDefUse &defuse, std::set<cstring> &zeroInitFields);
};

class AbstractElimUnusedInstructions : public Transform {
 protected:
    const FieldDefUse &defuse;

    /// Names of fields whose extracts have been eliminated.
    std::set<cstring> eliminated;

    const IR::MAU::StatefulAlu *preorder(IR::MAU::StatefulAlu *salu) override {
        // Don't go through these.
        prune();
        return salu;
    }

    const IR::GlobalRef *preorder(IR::GlobalRef *gr) override {
        // Don't go through these.
        prune();
        return gr;
    }

    /// Determines whether the given extract, occurring in the given unit, should be eliminated.
    virtual bool elim_extract(const IR::BFN::Unit *unit, const IR::BFN::Extract *extract) = 0;

    const IR::BFN::Extract *preorder(IR::BFN::Extract *extract) override;
    const IR::BFN::FieldLVal *preorder(IR::BFN::FieldLVal *lval) override;

    explicit AbstractElimUnusedInstructions(const FieldDefUse &defuse) : defuse(defuse) {}
};

#endif /* BF_P4C_COMMON_ELIM_UNUSED_H_ */
