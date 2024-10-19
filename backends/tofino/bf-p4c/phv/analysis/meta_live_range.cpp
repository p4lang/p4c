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

#include "phv/analysis/meta_live_range.h"

cstring MetadataLiveRange::printAccess(int access) {
    switch (access) {
        case 1:
            return "R"_cs;
        case 2:
            return "W"_cs;
        case 3:
            return "RW"_cs;
        default:
            return "U"_cs;
    }
}

bool MetadataLiveRange::notOverlapping(std::pair<int, int> &range1, std::pair<int, int> &range2,
                                       int depDist) {
    return notOverlapping(range1.first, range1.second, range2.first, range2.second, depDist);
}

bool MetadataLiveRange::notOverlapping(int minStage1, int maxStage1, int minStage2, int maxStage2,
                                       int depDist) {
    bool range1LTrange2 =
        ((minStage1 + depDist) < minStage2) && ((maxStage1 + depDist) < minStage2);
    bool range2LTrange1 =
        ((minStage2 + depDist) < minStage1) && ((maxStage2 + depDist) < minStage1);
    LOG9("\tnotOverlapping [" << minStage1 << ", " << maxStage1 << "] <> [" << minStage2 << " , "
                              << maxStage2 << "]: range1 < range2 " << range1LTrange2
                              << ", range2 < range1 " << range2LTrange1);
    return (range1LTrange2 || range2LTrange1);
}

Visitor::profile_t MetadataLiveRange::init_apply(const IR::Node *root) {
    livemap.clear();
    livemapUsage.clear();
    minStages.clear();
    overlay.clear();
    BUG_CHECK(dg.finalized, "Dependence graph is not populated.");
    return Inspector::init_apply(root);
}

bool MetadataLiveRange::preorder(const IR::MAU::Table *t) {
    int minStage = dg.min_stage(t);
    minStages[minStage].insert(t);
    return true;
}

void MetadataLiveRange::setFieldLiveMap(const PHV::Field *f) {
    Log::TempIndent indent;
    LOG2("Setting live range for field " << f << indent);
    // minUse = earliest stage for uses of the field.
    // maxUse = latest stage for uses of the field.
    // minDef = earliest stage for defs of the field.
    // maxDef = latest stage for defs of the field.
    // Set the min values initially to the deparser, and the max values to the parser initially.
    const int DEPARSER = dg.max_min_stage + 1;
    LOG9("DEPARSER = " << DEPARSER);
    int minUse = DEPARSER;
    int minDef = DEPARSER;
    int maxUse = -1;
    int maxDef = -1;
    int maxUseAccess = 0;
    int minUseAccess = 0;
    int minDefAccess = 0;
    int maxDefAccess = 0;

    // For each use/def of the field, parser imply stage -1, deparser imply stage
    // `Devices::numStages` (12 for Tofino), and a table implies the corresponding dg.min_stage.
    auto set_access = [this, f, DEPARSER](const FieldDefUse::LocPairSet &accessSet, int &min,
                                          int &max, int &minAccess, int &maxAccess,
                                          int accessMode) {
        Log::TempIndent indent;
        LOG4("Looking for " << (accessMode == READ ? "uses" : "definitions") << indent);
        const auto kind = accessMode == READ ? "Used" : "Defined";
        if (accessSet.empty()) LOG5(kind << " accessSet empty");
        for (const auto &[unit, expr] : accessSet) {
            if (unit->is<IR::BFN::ParserState>() || unit->is<IR::BFN::Parser>() ||
                unit->to<IR::BFN::GhostParser>()) {
                // If the def is an implicit read inserted only for metadata fields to account for
                // uninitialized reads, then ignore that initialization.
                // A use can never be ImplicitParserInit as ImplicitParserInit does not use
                // anything.
                if (expr->is<ImplicitParserInit>()) {
                    LOG4("Ignoring implicit parser init.");
                    continue;
                }
                // Ignore parser use if field is marked as not parsed.
                if (notParsedFields.count(f)) continue;
                // There is no need to set the maxUse here, because maxUse is either -1 (if there
                // is no other use) or a non-negative value (which does not need to be updated).
                LOG4(kind << " in parser.");
                min = -1;
                minAccess = accessMode;
            } else if (unit->is<IR::BFN::Deparser>()) {
                // Ignore deparser use if field is marked as not deparsed.
                if (notDeparsedFields.count(f)) continue;
                // There is no need to set the min here, because min is either DEPARSER (if there
                // is no other use) or a between [-1, dg.max_min_stage] (which does not need to be
                // updated).
                LOG4(kind << " in deparser.");
                max = DEPARSER;
                maxAccess = accessMode;
            } else if (unit->is<IR::MAU::Table>()) {
                const auto *t = unit->to<IR::MAU::Table>();
                int stage = dg.min_stage(t);
                LOG4(kind << " in stage " << stage << " in table " << t->name);
                // Update min and max based on the min_stage of the table.
                // If the min and max are encountered for the first time, overwrite the earlier
                // accessMode information. If another use with the same min/max is encountered,
                // then append the accessMode information with the accessMode information for this
                // unit.
                if (stage < min) {
                    min = stage;
                    minAccess = accessMode;
                } else if (stage == min) {
                    minAccess |= accessMode;
                }
                if (stage > max) {
                    max = stage;
                    maxAccess = accessMode;
                } else if (stage == max) {
                    maxAccess |= accessMode;
                }
            } else {
                BUG("Unknown unit encountered %1%", unit->toString());
            }
        }
    };
    set_access(defuse.getAllUses(f->id), minUse, maxUse, minUseAccess, maxUseAccess, READ);
    set_access(defuse.getAllDefs(f->id), minDef, maxDef, minDefAccess, maxDefAccess, WRITE);

    LOG2("minUse: " << minUse << " (" << printAccess(minUseAccess) << "), minDef: " << minDef
                    << " (" << printAccess(minDefAccess) << ")");
    LOG2("maxUse: " << maxUse << " (" << printAccess(maxUseAccess) << "), maxDef: " << maxDef
                    << " (" << printAccess(maxDefAccess) << ")");
    livemap[f->id] = std::make_pair(std::min(minUse, minDef), std::max(maxUse, maxDef));

    // Update the access information for the minStage and maxStage uses by synthesizing the separate
    // access information maintained for uses and defs of the field.
    int minStageAccess = (minUse == minDef) ? (minUseAccess | minDefAccess)
                                            : ((minUse > minDef) ? minDefAccess : minUseAccess);
    int maxStageAccess = (maxUse == maxDef) ? (maxUseAccess | maxDefAccess)
                                            : ((maxUse < maxDef) ? maxDefAccess : maxUseAccess);
    // If the field's live range is only one stage, then make sure the usages are set up
    // correct.
    if (livemap[f->id].first == livemap[f->id].second) {
        int access = minStageAccess | maxStageAccess;
        livemapUsage[f->id] = std::make_pair(access, access);
    } else {
        livemapUsage[f->id] = std::make_pair(minStageAccess, maxStageAccess);
    }
    LOG2("Live range for " << f->name << " is [" << livemap[f->id].first << ", "
                           << livemap[f->id].second << "]. Access is ["
                           << printAccess(livemapUsage[f->id].first) << ", "
                           << printAccess(livemapUsage[f->id].second) << "].");
}

void MetadataLiveRange::setPaddingFieldLiveMap(const PHV::Field *f) {
    const int DEPARSER = dg.max_min_stage + 1;
    // For padding fields (marked by overlayable), the live range is the deparser (for
    // ingress fields) and the parser (for egress fields).
    if (f->gress == INGRESS) {
        livemap[f->id] = std::make_pair(DEPARSER, DEPARSER);
    } else if (f->gress == EGRESS) {
        livemap[f->id] = std::make_pair(-1, -1);
    } else if (f->gress == GHOST) {
        livemap[f->id] = std::make_pair(DEPARSER, DEPARSER);
    }
}

void MetadataLiveRange::end_apply() {
    // If there are no tables, then dg.max_min_stage is -1, and this analysis is not required.
    if (dg.max_min_stage < 0) return;

    // Set of fields whose live ranges must be calculated.
    ordered_set<const PHV::Field *> fieldsConsidered;
    for (const PHV::Field &f : phv) {
        // Ignore metadata fields marked as no_init because initialization would cause their
        // container to become valid.
        if (f.is_invalidate_from_arch()) continue;
        if (noInitFields.count(&f) && !noOverlay.get_no_overlay_fields().count(&f)) {
            fieldsConsidered.insert(&f);
            continue;
        }
        // POV bits are always live, so ignore.
        if (f.pov) continue;
        // Ignore pa_no_overlay fields.
        if (noOverlay.get_no_overlay_fields().count(&f)) continue;
        // Ignore unreferenced fields because they are not allocated anyway.
        if (!uses.is_referenced(&f)) continue;
        fieldsConsidered.insert(&f);
    }
    for (const auto *f : fieldsConsidered) {
        if (f->overlayable)
            setPaddingFieldLiveMap(f);
        else
            setFieldLiveMap(f);
    }
    LOG1("Metadata live range overlay potential, subject to initialization");
    for (const auto *f1 : fieldsConsidered) {
        for (const auto *f2 : fieldsConsidered) {
            if (f1 == f2) continue;
            std::pair<int, int> range1 = livemap[f1->id];
            std::pair<int, int> range2 = livemap[f2->id];
            // No overlay possible if fields are of different gresses.
            if (f1->gress != f2->gress) {
                overlay(f1->id, f2->id) = false;
                continue;
            }
            // pair-wise no_overlay constraint check.
            if (!noOverlay.can_overlay(f1, f2)) {
                continue;
            }
            // If the live ranges of fields differ by more than DEP_DIST stages, then overlay due to
            // live range shrinking is possible.
            if (notOverlapping(range1, range2)) {
                overlay(f1->id, f2->id) = true;
                LOG3("    overlay(" << f1->name << ", " << f2->name << ")");
            }
            // For pa_no_init fields, dependence distance is of little less consideration.
            if (!overlay(f1->id, f2->id)) {
                if (noInitFields.count(f1) && noInitFields.count(f2)) {
                    if (notOverlapping(range1, range2, 0)) {
                        overlay(f1->id, f2->id) = true;
                        LOG1("    overlay noInit(" << f1->name << ", " << f2->name << ")");
                    }
                } else if (noInitFields.count(f1) && !noInitFields.count(f2)) {
                    if (range2.first < range1.first && range2.second < range1.first) {
                        overlay(f1->id, f2->id) = true;
                        LOG1("    overlay noInitF1(" << f1->name << ", " << f2->name << ")");
                    }
                } else if (noInitFields.count(f2) && !noInitFields.count(f1)) {
                    if (range1.first < range2.first && range1.second < range2.first) {
                        overlay(f1->id, f2->id) = true;
                        LOG1("    overlay noInitF2(" << f1->name << ", " << f2->name << ")");
                    }
                }
            }
        }
    }
    if (LOGGING(1)) printLiveRanges();
}

void MetadataLiveRange::printLiveRanges() const {
    LOG1("LIVERANGE REPORT");
    std::stringstream dashes;
    const int DEPARSER = dg.max_min_stage + 1;
    auto numStages = DEPARSER;
    const unsigned dashWidth = 125 + (numStages + 2) * 3;
    for (unsigned i = 0; i < dashWidth; i++) dashes << "-";
    LOG1(dashes.str());
    std::stringstream colTitle;
    colTitle << (boost::format("%=100s") % "Field Name") << "|  gress  | P |";
    for (int i = 0; i < numStages; i++) colTitle << boost::format("%=3s") % i << "|";
    colTitle << " D |";
    LOG1(colTitle.str());
    LOG1(dashes.str());
    for (auto entry : livemap) {
        const auto *f = phv.field(entry.first);
        int minUse = entry.second.first;
        int maxUse = entry.second.second;
        std::stringstream ss;
        ss << boost::format("%=100s") % f->name << "|" << boost::format("%=9s") % f->gress << "|";
        for (int i = -1; i <= numStages; i++) {
            if (minUse <= i && i <= maxUse)
                ss << " x |";
            else
                ss << "   |";
        }
        LOG1(ss.str());
    }
    LOG1(dashes.str());
}
