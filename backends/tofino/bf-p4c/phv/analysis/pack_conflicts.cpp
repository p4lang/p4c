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

#include "pack_conflicts.h"
#include "lib/log.h"

Visitor::profile_t PackConflicts::init_apply(const IR::Node *root) {
    profile_t rv = PassManager::init_apply(root);
    totalNumSet = 0;
    tableActions.clear();
    actionWrites.clear();
    fieldslice_to_id.clear();
    fieldslice_id_counter = 0;
    fieldslice_no_pack_i.clear();
    field_fine_slices.clear();
    // Initialize the fieldNoPack matrix to allow all fields to be packed together
    for (auto& f1 : phv) {
        for (auto& f2 : phv) {
            // If the fields are no pack according to deparser constraints, then set that here.
            if (phv.isDeparserNoPack(&f1, &f2)) {
                addPackConflict(PHV::FieldSlice(&f1), PHV::FieldSlice(&f2));
                continue;
            }
            // For all other fields, default is false.
            removePackConflict(PHV::FieldSlice(&f1), PHV::FieldSlice(&f2));
        }
        // Same field must always be packable with itself.
        removePackConflict(PHV::FieldSlice(&f1), PHV::FieldSlice(&f1));
    }

    // add pa_no_pack
    for (const auto& f : pa_no_pack.no_packs()) {
        LOG3("add pa_no_pack: " << f.first->name << " and " << f.second->name);
        addPackConflict(PHV::FieldSlice(f.first), PHV::FieldSlice(f.second));
    }
    return rv;
}

bool PackConflicts::GatherWrites::preorder(const IR::MAU::Action *act) {
    auto* tbl = findContext<IR::MAU::Table>();
    // Create a mapping of tables to all the actions it may invoke
    self.tableActions[tbl].insert(act);

    ActionAnalysis aa(self.phv, false, false, tbl, self.dg.red_info);
    ActionAnalysis::FieldActionsMap field_actions_map;
    aa.set_field_actions_map(&field_actions_map);
    act->apply(aa);

    // Collect all the PHV fields written to by this action into the actionWrites map
    for (auto& field_action : Values(field_actions_map)) {
        le_bitrange range;
        auto* write = self.phv.field(field_action.write.expr, &range);
        if (write == nullptr)
            BUG("Action does not have a write?");
        self.actionWrites[act].insert(PHV::FieldSlice(write, range)); }
    return true;
}

bool PackConflicts::GatherWrites::preorder(const IR::BFN::Digest* digest) {
    // The no-pack constraint on metadata fields used in learning digests is not applicable to JBay.
    if (Device::currentDevice() != Device::TOFINO) return true;
    // Currently we support only three digests: learning, mirror, and resubmit.
    if (digest->name != "learning" && digest->name != "mirror" && digest->name != "resubmit")
        return true;
    LOG3("    Determining constraints for " << digest->name << " digest.");
    ordered_set<const PHV::Field*> allDigestFields;
    // For fields that are not part of the same digest field lists, set the no-pack constraint.
    for (auto fieldList : digest->fieldLists) {
        for (auto flval1 : fieldList->sources) {
            const auto* f1 = self.phv.field(flval1->field);
            // Apply the constraint only to metadata fields.
            if (f1 == nullptr) continue;
            if (!f1->metadata && !f1->bridged) continue;
            allDigestFields.insert(f1); }
        for (const auto* f1 : allDigestFields) {
            for (const auto* f2 : allDigestFields) {
                if (f1 == f2) continue;
                // Fields within the same digest field list. So, packing them together is okay.
                self.phv.removeDigestNoPack(f1, f2); } } }

    // Set no pack for fields not marked digest pack okay.
    for (const auto* digestField : allDigestFields) {
        for (const auto& f : self.phv) {
            if (digestField == &f) continue;
            if (f.padding || f.isCompilerGeneratedPaddingField()) continue;
            if (!allDigestFields.count(&f)) {
                LOG3("\t  Setting no-pack for digest field " << digestField->name << " and "
                    "non-digest field " << f.name);
                self.addPackConflict(PHV::FieldSlice(digestField), PHV::FieldSlice(&f));
                self.phv.addDigestNoPack(digestField, &f); } } }

    return true;
}

void PackConflicts::generateNoPackConstraintsForBridgedFields(
        const IR::MAU::Table* t1, const IR::MAU::Table* t2) {
    ordered_set<PHV::FieldSlice> fieldslices1;
    ordered_set<PHV::FieldSlice> fieldslices2;

    if (!tableActions.count(t1)) {
        LOG6("\t\tNo actions in table " << t1);
        return; }
    if (!tableActions.count(t2)) {
        LOG6("\t\tNo actions in table " << t2);
        return; }
    for (auto act1 : tableActions.at(t1)) {
        for (auto act2 : tableActions.at(t2)) {
            if (amutex(act1, act2)) {
                LOG6("Actions " << act1->name << " and " << act2->name << " are mutually "
                        "exclusive.");
            } else {
                LOG6("\t  Non mutually exclusive actions " << act1->name << " and " << act2->name);
                fieldslices1.insert(actionWrites[act1].begin(), actionWrites[act1].end());
                fieldslices2.insert(actionWrites[act2].begin(), actionWrites[act2].end()); } } }

    LOG5("\tFor table: " << t1->name << ", number of fieldslices written across actions: " <<
            fieldslices1.size());
    LOG5("\tFor table: " << t2->name << ", number of fieldslices written across actions: " <<
            fieldslices2.size());

    for (auto fs1 : fieldslices1) {
        for (auto fs2 : fieldslices2) {
            LOG3("checking fs1 " << fs1 << " and fs2 " << fs2);
            if (fs1 == fs2) continue;
            if (!fs1.field()->bridged && !fs1.field()->is_digest()) continue;
            if (!fs2.field()->bridged && !fs1.field()->is_digest()) continue;
            addPackConflict(fs1, fs2);
            LOG6("\t" << " Setting no pack for bridge/digest " << fs1 << " and " << fs2);
        }
    }
}

void PackConflicts::end_apply() {
    for (auto row1 : tableActions) {
        for (auto row2 : tableActions) {
            auto* t1 = row1.first;
            auto* t2 = row2.first;
            if (t1 == t2) continue;
            ordered_set<int> stage = bt.inSameStage(t1, t2);
            bool on_same_stage = false;
            if (table_summary && (table_summary->getActualState() == State::SUCCESS)) {
                // In most cases, stage information is the same in table summary and backtracker.
                // However, backtracker's stage information is updated during backtracking and
                // table summary's stage information is updated after table placement. If
                // CheckForUnallocatedTemps is triggered and IncrementalPHVAllocation tries to
                // allocate temp vars, table summary and backtracker will have different stage
                // information, since table placement is successful and backtracking is not
                // performed. Therefore, table summary is needed to provide updated stage info.
                auto t1_stages = table_summary->stages(t1);
                auto t2_stages = table_summary->stages(t2);
                for (auto stage : t1_stages) {
                    if (t2_stages.count(stage)) {
                        on_same_stage = true;
                        break;
                    }
                }
            }
            if (!stage.empty() || on_same_stage) {
                LOG4("\tGenerate no pack conditions for table " << t1->name << " and table " <<
                        t2->name);
                generateNoPackConstraints(t1, t2);
            } else if (t1->get_provided_stage() == t2->get_provided_stage()
                       && t1->get_provided_stage() >= 0) {
                // Potentially should not be an issue, as this can get caught by backtracking
                // but if the program does not backtrack, then expected behavior might not
                // be the same
                LOG4("\tGenerate no pack conditions for table " << t1->name << " and table "
                      << t2->name << " due to stage pragmas");
                generateNoPackConstraints(t1, t2);
            }

            // from table dependency graph, we can estimate if two tables will be placed
            // in the same stage. If they happen to write to bridge fields, do not pack
            // the bridge fields together due to container conflicts.
            // * Except if one of the tables has been injected for field initialization
            if (!t1->name.startsWith("__mau_inits_table") &&
                !t2->name.startsWith("__mau_inits_table") &&
                (dg.min_stage(t1) == dg.min_stage(t2))) {
                LOG4("\tGenerate no pack conditions for table " << t1->name << " and table "
                      << t2->name << " for bridge fields");
                generateNoPackConstraintsForBridgedFields(t1, t2);
            }
        }
    }

    for (auto tbl_pair : ignore.pairwise_deps_to_ignore()) {
        LOG1("\tGenerate no pack conditions for table " << tbl_pair.first->name << " and table " <<
              tbl_pair.second->name << " due to ignore_table_dependency constraints");
        generateNoPackConstraints(tbl_pair.first, tbl_pair.second);
    }

    LOG3("Total packing conditions: " << totalNumSet);
    updateNumPackConstraints();
    if (LOGGING(5))
        printNoPackConflicts();
}

void PackConflicts::generateNoPackConstraints(const IR::MAU::Table* t1, const IR::MAU::Table* t2) {
    if (mutex(t1, t2)) {
        LOG6("\tTables " << t1->name << " and " << t2->name << " are mutually exclusive");
        return; }

    ordered_set<PHV::FieldSlice> fieldslices1;
    ordered_set<PHV::FieldSlice> fieldslices2;
    size_t numSet = 0;

    if (!tableActions.count(t1)) {
        LOG6("\t\tNo actions in table " << t1);
        return; }
    if (!tableActions.count(t2)) {
        LOG6("\t\tNo actions in table " << t2);
        return; }
    for (auto act1 : tableActions.at(t1)) {
        for (auto act2 : tableActions.at(t2)) {
            if (amutex(act1, act2)) {
                LOG6("Actions " << act1->name << " and " << act2->name << " are mutually "
                        "exclusive.");
            } else {
                LOG6("\t  Non mutually exclusive actions " << act1->name << " and " << act2->name);
                fieldslices1.insert(actionWrites[act1].begin(), actionWrites[act1].end());
                fieldslices2.insert(actionWrites[act2].begin(), actionWrites[act2].end()); } } }

    LOG5("\tFor table: " << t1->name << ", number of fields written across actions: " <<
            fieldslices1.size());
    LOG5("\tFor table: " << t2->name << ", number of fields written across actions: " <<
            fieldslices2.size());

    for (auto fs1 : fieldslices1) {
        for (auto fs2 : fieldslices2) {
            if (fs1.field() == fs2.field() && fs1.range().overlaps(fs2.range())) {
                // xxx(Deep): This should be taken care of by mutually_exclusive_actions
                LOG1("Dependency analysis may be wrong if " << fs1 << " is written by both "
                     "tables " << t1->name << " and " << t2->name);
            } else {
                ++numSet;
                addPackConflict(fs1, fs2);
                LOG6("\t" << " Setting no pack for " << fs1 << " and " << fs2); } } }

    LOG4("\tNumber of no pack conditions added for " << t1->name << " and " << t2->name << " : " <<
         (numSet / 2));
    totalNumSet += numSet;
}

unsigned PackConflicts::size() const {
    return phv.sizeFieldNoPack();
}

void PackConflicts::removePackConflict(const PHV::FieldSlice& fs1, const PHV::FieldSlice& fs2) {
    phv.removeFieldNoPack(fs1.field(), fs2.field());
    if (!fieldslice_to_id.count(fs1)) return;
    if (!fieldslice_to_id.count(fs2)) return;
    fieldslice_no_pack_i(fieldslice_to_id[fs1], fieldslice_to_id[fs2]) = false;
}

void PackConflicts::addPackConflict(const PHV::FieldSlice& fs1, const PHV::FieldSlice& fs2) {
    LOG6("add Pack Conflict " << fs1 << " and " << fs2);
    phv.addFieldNoPack(fs1.field(), fs2.field());

    field_fine_slices[fs1.field()].insert(fs1.range());
    field_fine_slices[fs2.field()].insert(fs2.range());

    if (!fieldslice_to_id.count(fs1)) fieldslice_to_id[fs1] = fieldslice_id_counter++;
    if (!fieldslice_to_id.count(fs2)) fieldslice_to_id[fs2] = fieldslice_id_counter++;
    fieldslice_no_pack_i(fieldslice_to_id[fs1], fieldslice_to_id[fs2]) = true;
}

bool PackConflicts::hasPackConflict(const PHV::FieldSlice& fs1, const PHV::FieldSlice& fs2) const {
    LOG6(this << " PackConflict:" << " - Checking for " << fs1 << " and " << fs2 << " )");
    if (!field_fine_slices.count(fs1.field())) return false;
    if (!field_fine_slices.count(fs2.field())) return false;

    for (const auto &range1 : field_fine_slices.at(fs1.field())) {
        if (!range1.overlaps(fs1.range())) continue;
        auto fs1_overlapped_fs = PHV::FieldSlice(fs1.field(), range1);
        int fs1_overlapped_id = fieldslice_to_id.at(fs1_overlapped_fs);
        for (const auto &range2 : field_fine_slices.at(fs2.field())) {
            if (!range2.overlaps(fs2.range())) continue;
            auto fs2_overlapped_fs = PHV::FieldSlice(fs2.field(), range2);
            int fs2_overlapped_id = fieldslice_to_id.at(fs2_overlapped_fs);
            if (fieldslice_no_pack_i(fs1_overlapped_id, fs2_overlapped_id)) {
                LOG6("has pack conflict");
                return true;
            }
        }
    }
    LOG6("does not have pack conflict");
    return false;
}

void PackConflicts::printNoPackConflicts() const {
    LOG5("List of no pack constraints for fields ");
    for (auto& f1 : phv) {
        for (auto& f2 : phv) {
            if (f1.id >= f2.id) continue;
            if (phv.isFieldNoPack(&f1, &f2)) {
                LOG5("\tno pack fields: " << f1.name << " (" << f1.id << "), " << f2.name <<
                     " (" << f2.id << ")");
            }
        }
    }
    LOG5("List of no pack constraints for fieldslices ");
    for (auto& entry1 : fieldslice_to_id) {
        auto fs1 = entry1.first;
        auto id1 = entry1.second;
        for (auto& entry2 : fieldslice_to_id) {
            auto fs2 = entry2.first;
            auto id2 = entry2.second;
            if (id1 >= id2) continue;
            if (hasPackConflict(fs1, fs2)) {
                LOG5("\tno pack fieldslices: " << fs1 << " (" << id1 << "), " <<
                                                  fs2 << " (" << id2 << ")");
            }
        }
    }
    LOG5("End List of no pack constraints ");
}

void PackConflicts::updateNumPackConstraints() {
    for (auto& f1 : phv) {
        size_t numPack = 0;
        for (auto& f2 : phv) {
            if (f1.id == f2.id) continue;
            if (phv.isFieldNoPack(&f1, &f2))
                numPack++; }
        f1.set_num_pack_conflicts(numPack); }
}

bool PackConflicts::writtenInSameStageDifferentTable(
        const IR::MAU::Table* t1,
        const IR::MAU::Table* t2) const {
    // Written in same stage and same table.
    if (t1 == t2) return false;
    // If no table placement from previous round, then ignore this.
    if (!bt.hasTablePlacement()) return false;
    ordered_set<int> stage = bt.inSameStage(t1, t2);
    std::stringstream ss;
    for (auto st : stage) ss << st << " ";
    if (!stage.empty())
        LOG3("\tTables " << t1->name << " and " << t2->name << " share common stages " <<
            ss.str());
    return !stage.empty();
}
