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

#ifndef BF_P4C_COMMON_ELIM_UNUSED_H_
#define BF_P4C_COMMON_ELIM_UNUSED_H_

#include "field_defuse.h"

using namespace P4;

class ReplaceMember : public Transform {
    const IR::BFN::AliasMember *preorder(IR::BFN::AliasMember *m) {
        return m; }
    const IR::BFN::AliasMember *preorder(IR::Member *m) {
        return new IR::BFN::AliasMember(m, m); }
};

class ElimUnused : public PassManager {
    const PhvInfo       &phv;
    FieldDefUse         &defuse;
    std::set<cstring> &zeroInitFields;

    class Instructions;
    class CollectEmptyTables;
    class Tables;
    class Headers;

 public:
    ElimUnused(const PhvInfo &phv, FieldDefUse &defuse,
                std::set<cstring> &zeroInitFields);
};

class AbstractElimUnusedInstructions : public Transform {
 protected:
    const FieldDefUse& defuse;

    /// Names of fields whose extracts have been eliminated.
    std::set<cstring> eliminated;

    const IR::MAU::StatefulAlu* preorder(IR::MAU::StatefulAlu* salu) override {
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
    virtual bool elim_extract(const IR::BFN::Unit* unit, const IR::BFN::Extract* extract) = 0;

    const IR::BFN::Extract* preorder(IR::BFN::Extract* extract) override;
    const IR::BFN::FieldLVal* preorder(IR::BFN::FieldLVal* lval) override;

    explicit AbstractElimUnusedInstructions(const FieldDefUse& defuse)
      : defuse(defuse) { }
};

#endif /* BF_P4C_COMMON_ELIM_UNUSED_H_ */
