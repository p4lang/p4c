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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_PACK_CONFLICTS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_PACK_CONFLICTS_H_

#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/mau/action_mutex.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_no_pack.h"
#include "ir/ir.h"

/** This class is meant to gather information about what fields cannot be packed together because of
 * the constraint that two or more tables in the same stage must not invoke an action that writes
 * the same PHV container.
 */
class PackConflicts : public PassManager {
 private:
    PhvInfo &phv;
    const DependencyGraph &dg;
    IgnoreTableDeps ignore;
    const TablesMutuallyExclusive &mutex;
    const MauBacktracker &bt;
    const ActionMutuallyExclusive &amutex;
    const PragmaNoPack &pa_no_pack;
    const TableSummary *table_summary;

    /// Count for total number of no pack constraints induced by table placement
    size_t totalNumSet = 0;

    /// Stores a set of all actions invoked from the key table
    ordered_map<const IR::MAU::Table *, ordered_set<const IR::MAU::Action *>> tableActions;
    /// Stores a set of all fields written in a particular action
    /// xxx(Deep): Change the field here to PHV::FieldSlice for more precise analysis
    ordered_map<const IR::MAU::Action *, ordered_set<PHV::FieldSlice>> actionWrites;

    ordered_map<PHV::FieldSlice, int> fieldslice_to_id;
    assoc::hash_map<const PHV::Field *, ordered_set<le_bitrange>> field_fine_slices;
    int fieldslice_id_counter = 0;
    SymBitMatrix fieldslice_no_pack_i;

    profile_t init_apply(const IR::Node *root) override;

    class GatherWrites : public Inspector {
        PackConflicts &self;
        /// Populate the tableActions and actionWrites maps
        bool preorder(const IR::MAU::Action *act) override;

        /// Populate noPack constraints related to learning digests.
        bool preorder(const IR::BFN::Digest *digest) override;

     public:
        explicit GatherWrites(PackConflicts &s) : self(s) {}
    };
    /// Once the initial information is gathered, generate actual no pack constraints
    void end_apply() override;

    /// Populates fieldNoPack with no pack constraints for fields written by tables @p t1 and @p t2
    /// If tables are mutually exclusive, then do not generate any no pack constraints for fields
    /// written in actions invoked from those tables.
    /// If two actions are mutually exclusive, do not generate any no pack constraints for fields
    /// written in those actions.
    void generateNoPackConstraints(const IR::MAU::Table *t1, const IR::MAU::Table *t2);

    void generateNoPackConstraintsForBridgedFields(const IR::MAU::Table *, const IR::MAU::Table *);

    /// Update the PHV::Field object for every field with the number of fields with which it cannot
    /// be packed
    void updateNumPackConstraints();

 public:
    PackConflicts(PhvInfo &p, const DependencyGraph &d, const TablesMutuallyExclusive &m,
                  const MauBacktracker &b, const ActionMutuallyExclusive &a,
                  const PragmaNoPack &no_pack, const TableSummary *table_summary = nullptr)
        : phv(p),
          dg(d),
          mutex(m),
          bt(b),
          amutex(a),
          pa_no_pack(no_pack),
          table_summary(table_summary) {
        addPasses({new GatherWrites(*this), &ignore});
    }

    void removePackConflict(const PHV::FieldSlice &f1, const PHV::FieldSlice &f2);

    void addPackConflict(const PHV::FieldSlice &f1, const PHV::FieldSlice &f2);

    bool hasPackConflict(const PHV::FieldSlice &f1, const PHV::FieldSlice &f2) const;

    bool writtenInSameStageDifferentTable(const IR::MAU::Table *t1, const IR::MAU::Table *t2) const;

    void printNoPackConflicts() const;

    unsigned size() const;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_PACK_CONFLICTS_H_ */
