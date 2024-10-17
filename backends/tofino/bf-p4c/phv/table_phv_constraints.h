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

#ifndef BF_P4C_PHV_TABLE_PHV_CONSTRAINTS_H_
#define BF_P4C_PHV_TABLE_PHV_CONSTRAINTS_H_

#include "ir/ir.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/analysis/pack_conflicts.h"
#include "bf-p4c/phv/pragma/pa_container_size.h"
#include "bf-p4c/phv/utils/slice_alloc.h"

/** This class enforces the following constraints related to match key sizes for ternary match
  * tables:
  * - For ternary match tables, the total size of fields used in the match key must be less than or
  *   equal to 66 bytes. For tables that approach this limit (specifically whose match key size is
  *   greater than or equal to 64 bytes), PHV allocation enforces bit in byte alignment to 0 for the
  *   match key fields.
  * xxx(Deep): Add match key constraints for exact match tables.
  * xxx(Deep): Calculate threshold dynamically.
  */

class TernaryMatchKeyConstraints : public MauModifier {
 private:
    static constexpr unsigned BITS_IN_BYTE = 8;
    static constexpr unsigned TERNARY_MATCH_KEY_BITS_THRESHOLD = 64;
    static constexpr unsigned MAX_EXACT_MATCH_KEY_BYTES = 128;
    static constexpr size_t MAX_TERNARY_MATCH_KEY_BITS = (44 * 12) - 4;

    PhvInfo      &phv;

    profile_t init_apply(const IR::Node* root) override;
    bool preorder(IR::MAU::Table* tbl) override;
    void end_apply() override;

    bool isATCAM(IR::MAU::Table* tbl) const;
    bool isTernary(IR::MAU::Table* tbl) const;

    void calculateTernaryMatchKeyConstraints(const IR::MAU::Table* tbl) const;

 public:
    explicit TernaryMatchKeyConstraints(PhvInfo &p) : phv(p) { }
};

class CollectForceImmediateFields : public MauInspector {
 private:
    PhvInfo                         &phv;
    const ActionPhvConstraints      &actions;
    PackConflicts             &pack;

    bool preorder(const IR::MAU::Action* action) override;

 public:
    explicit CollectForceImmediateFields(
            PhvInfo& p,
            const ActionPhvConstraints& a,
            PackConflicts& c)
        : phv(p), actions(a), pack(c) { }
};

/** This class is meant to gather PHV constraints enforced by MAU and the way fields are used in
  * the MAU. Some of these constraints are temporary fixes that are workarounds for current
  * limitations of our allocation algorithms.
  */
class TablePhvConstraints : public PassManager {
 public:
    explicit TablePhvConstraints(
            PhvInfo& p,
            const ActionPhvConstraints& a,
            PackConflicts& c) {
        addPasses({
            new TernaryMatchKeyConstraints(p),
            new CollectForceImmediateFields(p, a, c)
        });
    }
};
/** This class gather match field information to optimize XBar, SRAM and TCAM resources. The idea is
  * to share match bytes as much as possible among fields. Small field or flags are the one that
  * should benefit the most from this optimization. On typical compilation mode, scoring is used to
  * compare various super cluster allocation and find the solution that is the most optimal from
  * the match resource perspective. Table size is also part of the score compute such that bigger
  * table that would benefit the most from a specific packing are prioritize over smaller one.
  *
  * This pass is also used by table first approach to get pack candidate when doing trivial PHV
  * allocation. The idea in this case is to find small field or flag that can be combined on the
  * same match byte to reduce pressure on XBar, SRAM and TCAM.
  */
class TableFieldPackOptimization : public MauInspector {
 private:
    const PhvInfo &phv;
    ordered_map<PHV::FieldSlice, ordered_map<PHV::FieldSlice, int>> candidate;
    ordered_map<const PHV::Field*, ordered_set<PHV::FieldSlice>> f_to_fs;

    profile_t init_apply(const IR::Node* root) override;
    void end_apply() override;
    bool preorder(const IR::MAU::Table* tbl) override;
    int getNumberOfEntriesInTable(const IR::MAU::Table* tbl);
    int getFieldSlicePackScore(const PHV::AllocSlice &slice, const PHV::AllocSlice &slice2) const;

 public:
    explicit TableFieldPackOptimization(PhvInfo& p) : phv(p) { }

    int getPackScore(const ordered_set<PHV::AllocSlice> &parent,
                     const ordered_set<PHV::AllocSlice> &slices) const;
    std::list<PHV::FieldSlice> getPackCandidate(const PHV::FieldSlice &fs) const;
};

#endif  /* BF_P4C_PHV_TABLE_PHV_CONSTRAINTS_H_ */
