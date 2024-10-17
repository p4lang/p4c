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

#include "bf-p4c/phv/table_phv_constraints.h"
#include "bf-p4c/mau/table_layout.h"

Visitor::profile_t TernaryMatchKeyConstraints::init_apply(const IR::Node* root) {
    profile_t rv = MauModifier::init_apply(root);
    return rv;
}

bool TernaryMatchKeyConstraints::preorder(IR::MAU::Table* tbl) {
    if (tbl->uses_gateway()) return true;
    if (isATCAM(tbl)) return true;
    if (isTernary(tbl))
        calculateTernaryMatchKeyConstraints(tbl);
    return true;
}

void TernaryMatchKeyConstraints::end_apply() {
    return;
}

bool TernaryMatchKeyConstraints::isATCAM(IR::MAU::Table *tbl) const {
    cstring partition_index;
    IR::MAU::Table::Layout layout;
    TableLayout::check_for_atcam(layout, tbl, partition_index, phv);
    return layout.atcam;
}

bool TernaryMatchKeyConstraints::isTernary(IR::MAU::Table *tbl) const {
    if (!tbl->match_table) return false;
    IR::MAU::Table::Layout layout;
    TableLayout::check_for_ternary(layout, tbl);
    return layout.ternary;
}

void
TernaryMatchKeyConstraints::calculateTernaryMatchKeyConstraints(const IR::MAU::Table* tbl) const {
    LOG4("\tCalculating ternary match key constraints for " << tbl->name);
    size_t totalRoundedUpBytes = 0;
    size_t totalMetadataBytesUsed = 0;
    size_t totalBitsUsed = 0;

    // : Commenting *OFF* code used for constraining alignment
    // of ternary match fields. The imposed alignment constraints may
    // lead to erroneous PHV allocation fails due to preventing packing
    // of 1bit or narrow fields into PHV containers

    // *OFF* ordered_set<PHV::Field*> fields;

    // Calculate total rounded up (to nearest byte) size of all match key fields
    // Also calculate total rounded up (to nearest byte) size of all match key metadata fields
    for (const IR::MAU::TableKey* matchKey : tbl->match_key) {
        LOG5("\t\t" << matchKey->expr);
        le_bitrange bits;
        PHV::Field* f = phv.field(matchKey->expr, &bits);
        CHECK_NULL(f);
        size_t roundedUpSize = (f->size % BITS_IN_BYTE == 0) ? (f->size / BITS_IN_BYTE) :
            ((f->size / BITS_IN_BYTE) + 1);
        totalBitsUsed += bits.size();
        totalRoundedUpBytes += roundedUpSize;
        if (f->metadata && !f->deparsed_to_tm()) {
            totalMetadataBytesUsed += roundedUpSize;
            // *OFF*      fields.insert(f);
        }
    }
    LOG4("\tTotal bits used for match key by ternary table : " << totalBitsUsed);
    LOG4("\tTotal bytes used for match key by ternary table : " <<
         (((totalBitsUsed % BITS_IN_BYTE) == 0) ? (totalBitsUsed / BITS_IN_BYTE) :
          ((totalBitsUsed / BITS_IN_BYTE) + 1)));

    if (totalBitsUsed > MAX_TERNARY_MATCH_KEY_BITS) {
        error("Ternary table %1% uses %2%b as ternary match key. Maximum number of bits "
                "allowed is %3%b.\nRewrite your program to use fewer ternary match key bits."
                "\nTable %1% cannot fit within a single input crossbar in an MAU stage.",
                tbl->name, totalBitsUsed, MAX_TERNARY_MATCH_KEY_BITS);
        return;
    }

    LOG4("\tTotal rounded up bytes used for match key by ternary table: " << totalRoundedUpBytes);
    LOG4("\tTotal bytes used for match key by metadata     : " << totalMetadataBytesUsed);
    // xxx(Deep): Can we calculate a better threshold?
    // If the total size then exceeds the match key size threshold defined for ternary tables,
    // introduce a bit in byte alignment for all metadata fields
    return;
}

bool CollectForceImmediateFields::preorder(const IR::MAU::Action* action) {
    const auto* tbl = findContext<IR::MAU::Table>();
    if (!tbl) return true;
    if (tbl->get_immediate_ctrl() != IR::MAU::Table::FORCE_IMMEDIATE) return true;
    LOG3("\t  Action " << action->name << " belongs to force_immediate table " << tbl->name);
    auto writtenFields = actions.actionWrites(action);
    ordered_set<const PHV::Field*> fields;
    for (const auto* f : writtenFields) {
        if (actions.written_by_ad_constant(f, action)) {
            fields.insert(f);
            if (!f->metadata) continue;
            PHV::Field* field = phv.field(f->id);
            CHECK_NULL(field);
            field->set_written_in_force_immediate(true);
            LOG3("\t\tSetting written in force immediate property for " << f->name);
        }
    }
    if (fields.size() == 0) return true;
    LOG3("\t\tListing fields written by action data/constant in this action:");
    for (auto* f : fields)
        LOG3("\t\t  " << f);
    for (const auto* f1 : fields) {
        for (const auto* f2 : writtenFields) {
            if (f1 == f2) continue;
            PHV::FieldSlice fs1(f1);
            PHV::FieldSlice fs2(f2);
            if (pack.hasPackConflict(fs1, fs2)) continue;
            LOG3("\t\tAdding pack conflict between " << f1->name << " and " << f2->name);
            pack.addPackConflict(fs1, fs2);
        }
    }
    return true;
}

Visitor::profile_t TableFieldPackOptimization::init_apply(const IR::Node* root) {
    profile_t rv = MauInspector::init_apply(root);
    candidate.clear();
    f_to_fs.clear();
    return rv;
}

// Dump the gathered information if log enabled
void TableFieldPackOptimization::end_apply() {
    LOG3("Show field to field slice mapping");
    for (const auto &kv : f_to_fs) {
        LOG3("\t Field: " << kv.first);
        for (const auto &fs : kv.second) {
            LOG3("\t\t FieldSlice: " << fs);
            for (const auto &sc : candidate.at(fs))
                LOG3("\t\t\t With FieldSlice: " << sc.first << " for a score of " << sc.second);
            std::list<PHV::FieldSlice> fs_list = getPackCandidate(fs);
            if (!fs_list.empty()) {
                LOG3("\t\t Pack Candidate:");
                for (const auto &fs_cand : fs_list)
                    LOG3("\t\t\t FieldSlice: " << fs_cand);
            }
        }
    }
    return;
}

// Get the number of entries for various type of table to estimate the weigth of the latter when
// packing field. This does not need to be fully accurate.
int TableFieldPackOptimization::getNumberOfEntriesInTable(const IR::MAU::Table* tbl) {
    int entries = 512;  // default number of entries

    if (tbl->layout.no_match_data())
        entries = 1;
    if (tbl->layout.pre_classifier)
        entries = tbl->layout.pre_classifer_number_entries;
    else if (auto k = tbl->match_table->getConstantProperty("size"_cs))
        entries = k->asInt();
    else if (auto k = tbl->match_table->getConstantProperty("min_size"_cs))
        entries = k->asInt();

    if (tbl->layout.exact) {
        if (tbl->layout.ixbar_width_bits < ceil_log2(entries))
            entries = 1 << tbl->layout.ixbar_width_bits;
    }
    return entries;
}

// Get the match key of a table and fill "candidate" and "f_to_fs" maps that track field and field
// slice packing. Score of a specific pair is increased based on the table size. At the end both
// candidate[fs1][fs2] and candidate[fs2][fs1] should have the same score but tracking both simplify
// things at the getFieldSlicePackScore() level.
bool TableFieldPackOptimization::preorder(const IR::MAU::Table* tbl) {
    if (!tbl->match_table)
        return true;

    LOG3("\tGet match key for " << tbl->name);
    int num_entries = getNumberOfEntriesInTable(tbl);
    LOG3("\tNumber of entries " << num_entries);
    std::list<PHV::FieldSlice> matchfs;
    for (const IR::MAU::TableKey* matchKey : tbl->match_key) {
        if (matchKey->for_selection())
            continue;

        LOG3("\t\t" << matchKey->expr);
        le_bitrange bits;
        const PHV::Field* f = phv.field(matchKey->expr, &bits);
        CHECK_NULL(f);
        if (f->is_solitary()) {
            LOG3("\t\t Skipping since solitary attribute set");
            continue;
        }
        matchfs.emplace_back(f, bits);
    }
    for (auto cit = matchfs.cbegin(); cit != matchfs.cend(); cit++) {
        const PHV::FieldSlice &fs = *cit;
        for (auto cit2 = matchfs.cbegin(); cit2 != matchfs.cend(); cit2++) {
            const PHV::FieldSlice &fs2 = *cit2;
            if (fs == fs2)
                continue;

            if (phv.isFieldNoPack(fs.field(), fs2.field())) {
                LOG3("\t\t No pack set between " << fs.field() << " and " << fs2.field());
                continue;
            }
            int original_score = candidate[fs][fs2];
            candidate[fs][fs2] = original_score + num_entries;
            LOG3("\t\t Increasing score from " << original_score << " to " <<
                 original_score + num_entries << " between " << fs << " and " << fs2);
        }
        // Increasing slice score with itself to also increase chance of packing slice of the same
        // field in the same byte boundary.
        int original_score = candidate[fs][fs];
        candidate[fs][fs] = original_score + num_entries;
        LOG3("\t\t Increasing score from " << original_score << " to " <<
             original_score + num_entries << " on " << fs);
        f_to_fs[fs.field()].insert(fs);
    }
    return true;
}

// Return score for these two alloc slices. To have a non zero score, both slice must share a byte
// and be used simultaneously by a match table. We have to handle the possibility that PHV sliced
// the field differently than how it is matched.
int TableFieldPackOptimization::getFieldSlicePackScore(const PHV::AllocSlice &slice,
                                                       const PHV::AllocSlice &slice2) const {
    const PHV::Field* f = slice.field();
    if (!f_to_fs.count(f))
        return 0;
    const PHV::Field* f2 = slice2.field();
    if (!f_to_fs.count(f2))
        return 0;

    // Dark spilling should also not be part of the compute.
    if (slice.hasInitPrimitive() || slice2.hasInitPrimitive())
        return 0;

    le_byterange byte_range(slice.container_slice().loByte(),
                            slice.container_slice().hiByte());

    le_byterange byte_range2(slice2.container_slice().loByte(),
                             slice2.container_slice().hiByte());
    if (!byte_range.overlaps(byte_range2))
        return 0;
    le_bitrange range = slice.field_slice();
    le_bitrange range2 = slice2.field_slice();
    int pack_score = 0;
    for (const PHV::FieldSlice &fs : f_to_fs.at(f)) {
        const le_bitrange &fs_range = fs.range();
        if (fs_range.overlaps(range)) {
            for (const PHV::FieldSlice &fs2 : f_to_fs.at(f2)) {
                const le_bitrange &fs_range2 = fs2.range();
                if (fs_range2.overlaps(range2)) {
                    if (candidate.count(fs) && candidate.at(fs).count(fs2)) {
                        pack_score += candidate.at(fs).at(fs2);
                    }
                }
            }
        }
    }
    return pack_score;
}
/** Return the packing score of this new transaction over the committed one for a specific
 *  container. The score is based on individual new slices among them plus the score versus the
 *  parent transaction.
 */
int TableFieldPackOptimization::getPackScore(const ordered_set<PHV::AllocSlice> &parent,
                                             const ordered_set<PHV::AllocSlice> &slices) const {
    if (parent.size() + slices.size() <= 1 || slices.size() == 0)
        return 0;

    const PHV::Container container = slices.begin()->container();
    int pack_score = 0;

    for (auto cit = slices.cbegin(); cit != slices.cend(); cit++) {
        const PHV::AllocSlice &slice = *cit;
        BUG_CHECK(slice.container() == container,
                  "Trying to get a packing score for a set with multiple containers");
        for (auto cit2 = std::next(cit); cit2 != slices.cend(); cit2++) {
            const PHV::AllocSlice &slice2 = *cit2;
            pack_score += getFieldSlicePackScore(slice, slice2);
        }
        for (const PHV::AllocSlice &slice2 : parent) {
            pack_score += getFieldSlicePackScore(slice, slice2);
        }
    }
    return pack_score;
}

/** Return a list of PHV::FieldSlice that are good candidate to be packed with the provided
 *  PHV::FieldSlice fs. The returned list is sorted in reverse order such that the first elements
 *  are the one that would benefit the most from being packed with the provided PHV::FieldSlice fs.
 *  Only Metadata field are considered in this process. The returned list can contains FieldSlice
 *  of the same Field as the input FieldSlice but with a range that does not intersect with it.
 *  The returned FieldSlice can also specify a range that is different from the PHV slicing, e.g.:
 *  f1[2:7] while in PHV this field was sliced like f1[0:1], f1[2:3], f1[4:7]. In this case the
 *  caller must map the returned FieldSlice with the current slice for a particular field.
 *
 *  Multiple FieldSlice that refer to the same Field with intersecting range can also be returned,
 *  e.g.: f1[0:7], f1[0:3]. In this case the order on which they appear should be considered.
 *
 *  An empty list can be returned if no potential candidate are found.
 **/
std::list<PHV::FieldSlice>
TableFieldPackOptimization::getPackCandidate(const PHV::FieldSlice &fs) const {
    const PHV::Field* f = fs.field();
    const le_bitrange &fs_range = fs.range();
    std::list<PHV::FieldSlice> rv;

    if (!f->metadata || ((fs_range.size() % 8) == 0) || !f_to_fs.count(f))
        return rv;

    ordered_map<PHV::FieldSlice, int> pack_candidate;

    for (const PHV::FieldSlice &tracked_fs : f_to_fs.at(f)) {
        const le_bitrange &tracked_fs_range = tracked_fs.range();
        if (tracked_fs_range.overlaps(fs_range)) {
            if (candidate.count(tracked_fs)) {
                for (const auto &kv : candidate.at(tracked_fs)) {
                    const PHV::Field* candidate_f = kv.first.field();
                    if (!candidate_f->metadata)
                        continue;

                    if (candidate_f == f && kv.first.range().overlaps(fs_range)) {
                        le_bitinterval low_range;
                        le_bitinterval hi_range;
                        std::tie(low_range, hi_range) = kv.first.range() - fs_range;

                        if (!low_range.empty()) {
                            le_bitrange fs_low_range(low_range.lo, low_range.hi - 1);
                            PHV::FieldSlice low_slice(kv.first, fs_low_range);
                            pack_candidate[low_slice] += kv.second;
                        }

                        if (!hi_range.empty()) {
                            le_bitrange fs_hi_range(hi_range.lo, hi_range.hi - 1);
                            PHV::FieldSlice hi_slice(kv.first, fs_hi_range);
                            pack_candidate[hi_slice] += kv.second;
                        }
                    } else {
                        pack_candidate[kv.first] += kv.second;
                    }
                }
            }
        }
    }

    for (const auto &kv : pack_candidate)
        rv.push_back(kv.first);

    rv.sort([&](const PHV::FieldSlice &fs1, const PHV::FieldSlice &fs2) -> bool
        {
            int score1 = pack_candidate[fs1] / fs1.range().size();
            int score2 = pack_candidate[fs2] / fs2.range().size();
            return (score1 > score2);
        });

    return rv;
}

