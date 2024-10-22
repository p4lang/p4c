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

#include "bf-p4c/phv/allocate_phv.h"

#include <numeric>
#include <sstream>

#include <boost/bind/bind.hpp>
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/logging/event_logger.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parser_query.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/fieldslice_live_range.h"
#include "bf-p4c/phv/legacy_action_packing_validator.h"
#include "bf-p4c/phv/live_range_split.h"
#include "bf-p4c/phv/optimize_phv.h"
#include "bf-p4c/phv/parser_extract_balance_score.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/slicing/phv_slicing_iterator.h"
#include "bf-p4c/phv/slicing/phv_slicing_split.h"
#include "bf-p4c/phv/utils/container_equivalence.h"
#include "bf-p4c/phv/utils/report.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_map.h"

const std::vector<PHV::Size> StateExtractUsage::extractor_sizes = {PHV::Size::b8, PHV::Size::b16,
                                                                   PHV::Size::b32};

// AllocScore metrics.
namespace {

using MetricName = AllocScore::MetricName;

// general
const MetricName n_tphv_on_phv_bits = "tphv_on_phv_bits"_cs;
const MetricName n_mocha_on_phv_bits = "mocha_on_phv_bits"_cs;
const MetricName n_dark_on_phv_bits = "dark_on_phv_bits"_cs;
const MetricName n_dark_on_mocha_bits = "dark_on_mocha_bits"_cs;
/// Number of bitmasked-set operations introduced by this transaction.
// Opt for the AllocScore, which minimizes the number of bitmasked-set instructions.
// This helps action data packing and gives table placement a better shot
// at fitting within the number of stages available on the device.

const MetricName n_bitmasked_set = "n_bitmasked_set"_cs;
/// Number of container bits wasted because POV slice lists/slices
/// do not fill the container wholly.
const MetricName n_wasted_pov_bits = "wasted_pov_bits"_cs;
const MetricName parser_extractor_balance = "parser_extractor_balance"_cs;
const MetricName n_inc_tphv_collections = "n_inc_tphv_collections"_cs;
const MetricName n_prefer_bits = "n_prefer_bits"_cs;

// by kind
const MetricName n_set_gress = "n_set_gress"_cs;
const MetricName n_set_parser_group_gress = "n_set_parser_group_gress"_cs;
const MetricName n_set_deparser_group_gress = "n_set_deparser_group_gress"_cs;
const MetricName n_set_parser_extract_group_source = "n_set_parser_extract_group_source"_cs;
const MetricName n_overlay_bits = "n_overlay_bits"_cs;
const MetricName n_field_packing_score = "n_field_packing_score"_cs;
// how many wasted bits in partial container get used.
const MetricName n_packing_bits = "n_packing_bits"_cs;
// smaller, better.
const MetricName n_packing_priority = "n_packing_priority"_cs;
const MetricName n_inc_containers = "n_inc_containers"_cs;
// if solitary but taking a container larger than it.
const MetricName n_wasted_bits = "n_wasted_bits"_cs;
// use of 8/16 bit containers
const MetricName n_inc_small_containers = "n_inc_small_containers"_cs;
// The number of CLOT-eligible bits that have been allocated to PHV
// (JBay only).
const MetricName n_clot_bits = "n_clot_bits"_cs;
// The number of containers in a deparser group allocated to
// non-deparsed fields of a different gress than the deparser group.
const MetricName n_mismatched_deparser_gress = "n_mismatched_deparser_gress"_cs;

void log_packing_opportunities(const FieldPackingOpportunity *p,
                               const std::list<PHV::SuperCluster *> sc) {
    if (LOGGING(4)) {
        if (p == nullptr) {
            LOG_DEBUG4("FieldPackingOpportunity is null");
            return;
        }

        LOG_DEBUG4("Packing opportunities:");
        for (const auto *cluster : sc) {
            std::set<const PHV::Field *> showed;
            cluster->forall_fieldslices([&](const PHV::FieldSlice &fs) {
                if (showed.count(fs.field())) return;
                showed.insert(fs.field());
                LOG_DEBUG4(TAB1 "Field " << fs.field() << " has " << p->nOpportunities(fs.field())
                                         << "opportunities");
            });
        }
    }
}

std::string str_supercluster_alignments(PHV::SuperCluster &sc, const AllocAlignment &align) {
    std::stringstream ss;
    ss << "slicelist alignments: " << "\n";
    for (const auto *sl : sc.slice_lists()) {
        ss << "[";
        std::string sep = "";
        for (const auto &fs : *sl) {
            ss << sep << "{" << fs << ", " << align.slice_alignment.at(fs) << "}";
            sep = ", ";
        }
        ss << "]" << "\n";
    }
    return ss.str();
}

// alloc_alignment_merge return merged alloc alignment if no conflict.
std::optional<AllocAlignment> alloc_alignment_merge(const AllocAlignment &a,
                                                    const AllocAlignment &b) {
    AllocAlignment rst;
    for (auto &alignment : std::vector<const AllocAlignment *>{&a, &b}) {
        // wont' be conflict
        for (auto &fs_int : alignment->slice_alignment) {
            rst.slice_alignment[fs_int.first] = fs_int.second;
        }
        // may conflict
        for (auto &cluster_int : alignment->cluster_alignment) {
            const auto *cluster = cluster_int.first;
            auto align = cluster_int.second;
            if (rst.cluster_alignment.count(cluster) && rst.cluster_alignment[cluster] != align) {
                return std::nullopt;
            }
            rst.cluster_alignment[cluster] = align;
        }
    }
    return rst;
}

enum class CondIsBetter { equal, yes, no };

struct AllocScoreCmpCond {
    cstring name;
    int delta;
    bool is_positive_better;
    AllocScoreCmpCond(cstring name, int delta, bool is_positive_better)
        : name(name), delta(delta), is_positive_better(is_positive_better) {}

    CondIsBetter is_better() const {
        if (delta == 0) {
            return CondIsBetter::equal;
        }
        if (is_positive_better) {
            return delta > 0 ? CondIsBetter::yes : CondIsBetter::no;
        } else {
            return delta < 0 ? CondIsBetter::yes : CondIsBetter::no;
        }
    }
};

std::pair<bool, cstring> prioritized_cmp(std::vector<AllocScoreCmpCond> conds) {
    for (const auto &c : conds) {
        switch (c.is_better()) {
            case CondIsBetter::equal: {
                continue;
                break;
            }
            case CondIsBetter::yes: {
                return {true, c.name};
            }
            case CondIsBetter::no: {
                return {false, c.name};
            }
        }
    }
    return {false, "equal"_cs};
}

ordered_map<MetricName, int> weighted_sum(const AllocScore &delta, const int dark_weight = 0,
                                          const int mocha_weight = 3, const int normal_weight = 3,
                                          const int tagalong_weight = 1) {
    ordered_map<MetricName, int> weighted_delta;
    for (auto kind : Device::phvSpec().containerKinds()) {
        int weight = 0;
        switch (kind) {
            case PHV::Kind::dark:
                weight = dark_weight;
                break;
            case PHV::Kind::mocha:
                weight = mocha_weight;
                break;
            case PHV::Kind::normal:
                weight = normal_weight;
                break;
            case PHV::Kind::tagalong:
                weight = tagalong_weight;
                break;
        }
        if (weight == 0) {
            continue;
        }
        if (delta.by_kind.count(kind)) {
            const auto &ks = delta.by_kind.at(kind);
            for (const auto &m : AllocScore::g_by_kind_metrics) {
                if (!ks.count(m)) {
                    continue;
                }
                weighted_delta[m] += weight * ks.at(m);
            }
        }
    }
    return weighted_delta;
}

/** default_alloc_score_is_better
 * As of 04/07/2020, followings is the same heuristic set as the current master .
 */
bool default_alloc_score_is_better(const AllocScore &left, const AllocScore &right) {
    AllocScore delta = left - right;
    const int DARK_TO_PHV_DISTANCE = 2;
    int container_type_score = 0;
    if (Device::currentDevice() == Device::TOFINO) {
        container_type_score = delta.general[n_tphv_on_phv_bits];
    } else {
        container_type_score = DARK_TO_PHV_DISTANCE * delta.general[n_dark_on_phv_bits] +
                               delta.general[n_mocha_on_phv_bits] +
                               delta.general[n_dark_on_mocha_bits];
    }

    auto weighted_delta = weighted_sum(delta);
    std::vector<AllocScoreCmpCond> conds;
    if (Device::currentDevice() != Device::TOFINO) {
        conds = {
            {n_clot_bits, weighted_delta[n_clot_bits], false},
            {n_prefer_bits, delta.general[n_prefer_bits], true},
            {n_overlay_bits, weighted_delta[n_overlay_bits], true},  // if !tofino
            {"container_type_score"_cs,
             // TODO:
             // The code below simply wants to reproduce:
             // allocate_phv.cpp#L235 at 405005d67b1bb0e31c36cbb274518780bf86f1aa.
             container_type_score * int(weighted_delta[n_inc_containers] == 0), false},
            {n_wasted_pov_bits, delta.general[n_wasted_pov_bits], false},
            {n_inc_tphv_collections, delta.general[n_inc_tphv_collections], false},
            {n_bitmasked_set, delta.general[n_bitmasked_set], false},
            {n_inc_containers, weighted_delta[n_inc_containers], false},
            {n_wasted_bits, weighted_delta[n_wasted_bits], false},
            {n_packing_bits, weighted_delta[n_packing_bits], true},
            {n_packing_priority, weighted_delta[n_packing_priority], false},
            {n_set_gress, weighted_delta[n_set_gress], false},
            {n_set_deparser_group_gress, weighted_delta[n_set_deparser_group_gress], false},
            {n_set_parser_group_gress, weighted_delta[n_set_parser_group_gress], false},
            {n_set_parser_extract_group_source, weighted_delta[n_set_parser_extract_group_source],
             false},
            {n_field_packing_score, weighted_delta[n_field_packing_score], true},
            // suspicious why true?
            {n_mismatched_deparser_gress, weighted_delta[n_mismatched_deparser_gress], true},
        };
    } else {
        conds = {
            {parser_extractor_balance, delta.general[parser_extractor_balance], true},
            {n_clot_bits, weighted_delta[n_clot_bits], false},
            {n_prefer_bits, delta.general[n_prefer_bits], true},
            {"container_type_score"_cs, container_type_score, false},
            {n_inc_tphv_collections, delta.general[n_inc_tphv_collections], false},
            {n_bitmasked_set, delta.general[n_bitmasked_set], false},
            {n_inc_containers, weighted_delta[n_inc_containers], false},
            {n_overlay_bits, weighted_delta[n_overlay_bits], true},  // if tofino
            {n_wasted_bits, weighted_delta[n_wasted_bits], false},
            {n_packing_bits, weighted_delta[n_packing_bits], true},
            {n_packing_priority, weighted_delta[n_packing_priority], false},
            {n_set_gress, weighted_delta[n_set_gress], false},
            {n_set_deparser_group_gress, weighted_delta[n_set_deparser_group_gress], false},
            {n_set_parser_group_gress, weighted_delta[n_set_parser_group_gress], false},
            {n_set_parser_extract_group_source, weighted_delta[n_set_parser_extract_group_source],
             false},
            {n_field_packing_score, weighted_delta[n_field_packing_score], true},
            // suspicious why true?
            {n_mismatched_deparser_gress, weighted_delta[n_mismatched_deparser_gress], true},
        };
    }
    auto rst = prioritized_cmp(conds);
    if (rst.second != "equal") {
        LOG_DEBUG6(TAB2 << (rst.first ? "better" : "worse") << ", because: " << rst.second);
    }

    return rst.first;
}

/** less_fragment_alloc_score_is_better
 * This is a generally better heuristic set: rules and priorities are reasonable compared
 * with the default one. We still keep the default one there because the compiler is still
 * buggy at this moment. If we entirely retired the default strategy, we will see a lot of
 * regressions because bugs are triggered.
 */
bool less_fragment_alloc_score_is_better(const AllocScore &left, const AllocScore &right) {
    AllocScore delta = left - right;
    auto weighted_delta = weighted_sum(delta, 0, 1, 3, 1);

    std::vector<AllocScoreCmpCond> conds;
    if (Device::currentDevice() == Device::TOFINO) {
        conds = {
            {n_tphv_on_phv_bits, delta.general[n_tphv_on_phv_bits], false},
            {n_prefer_bits, delta.general[n_prefer_bits], true},
            {n_wasted_bits, weighted_delta[n_wasted_bits], false},
            {n_inc_small_containers, weighted_delta[n_inc_small_containers], false},
            {n_inc_tphv_collections, delta.general[n_inc_tphv_collections], false},
            {n_inc_containers, weighted_delta[n_inc_containers], false},
            {n_overlay_bits, weighted_delta[n_overlay_bits], true},
            {n_packing_bits, weighted_delta[n_packing_bits], true},
            {n_packing_priority, weighted_delta[n_packing_priority], false},
            {n_bitmasked_set, delta.general[n_bitmasked_set], false},
            {parser_extractor_balance, delta.general[parser_extractor_balance], true},
            {n_set_gress, weighted_delta[n_set_gress], false},
            {n_set_deparser_group_gress, weighted_delta[n_set_deparser_group_gress], false},
            {n_set_parser_group_gress, weighted_delta[n_set_parser_group_gress], false},
            {n_set_parser_extract_group_source, weighted_delta[n_set_parser_extract_group_source],
             false},
            {n_field_packing_score, weighted_delta[n_field_packing_score], true},
            {n_mismatched_deparser_gress, weighted_delta[n_mismatched_deparser_gress], false},
        };
    } else {
        const int dark_penalty = 2;
        const int mocha_penalty = 3;
        int bad_container_bits = delta.general[n_tphv_on_phv_bits] +
                                 dark_penalty * delta.general[n_dark_on_phv_bits] +
                                 mocha_penalty * delta.general[n_mocha_on_phv_bits] +
                                 delta.general[n_dark_on_mocha_bits];
        conds = {
            {n_clot_bits, weighted_delta[n_clot_bits], false},
            {"bad_container_bits"_cs, bad_container_bits, false},
            {n_prefer_bits, delta.general[n_prefer_bits], true},
            {n_wasted_pov_bits, delta.general[n_wasted_pov_bits], false},
            {n_wasted_bits, weighted_delta[n_wasted_bits], false},
            // TODO: Starting with Tofino2, because of dark containers, we have to
            // promote the priority of overlay to almost the highest to encourage dark
            // overlay fields.
            {n_overlay_bits, weighted_delta[n_overlay_bits], true},
            {n_inc_small_containers, weighted_delta[n_inc_small_containers], false},
            {n_inc_containers, weighted_delta[n_inc_containers], false},
            {n_packing_bits, weighted_delta[n_packing_bits], true},
            {n_packing_priority, weighted_delta[n_packing_priority], false},
            {n_bitmasked_set, delta.general[n_bitmasked_set], false},
            {n_set_gress, weighted_delta[n_set_gress], false},
            {n_set_deparser_group_gress, weighted_delta[n_set_deparser_group_gress], false},
            {n_set_parser_group_gress, weighted_delta[n_set_parser_group_gress], false},
            {n_set_parser_extract_group_source, weighted_delta[n_set_parser_extract_group_source],
             false},
            {n_field_packing_score, weighted_delta[n_field_packing_score], true},
            {n_mismatched_deparser_gress, weighted_delta[n_mismatched_deparser_gress], false},
        };
    }

    auto rst = prioritized_cmp(conds);
    if (rst.second != "equal") {
        LOG_DEBUG6("left-hand-side score is better because: " << rst.second);
    }
    return rst.first;
}

/// @returns a new slice list where any adjacent slices of the same field with
/// contiguous little Endian ranges are merged into one slice, eg f[0:3], f[4:7]
/// become f[0:7].
const PHV::SuperCluster::SliceList *mergeContiguousSlices(
    const PHV::SuperCluster::SliceList *list) {
    if (!list->size()) return list;

    auto *rv = new PHV::SuperCluster::SliceList();
    auto slice = list->front();
    for (auto it = ++list->begin(); it != list->end(); ++it) {
        if (slice.field() == it->field() && slice.range().hi == it->range().lo - 1)
            slice = PHV::FieldSlice(slice.field(), FromTo(slice.range().lo, it->range().hi));
        else
            rv->push_back(slice);
    }
    if (rv->size() == 0 || rv->back() != slice) rv->push_back(slice);
    return rv;
}

bool satisfies_container_type_constraints(const PHV::ContainerGroup &group,
                                          const PHV::AlignedCluster &cluster) {
    // Check that these containers support the operations required by fields in
    // this cluster.
    for (auto t : group.types()) {
        if (cluster.okIn(t.kind())) {
            return true;
        }
    }
    return false;
}

/// @returns a concrete allocation based on current phv info object.
PHV::ConcreteAllocation make_concrete_allocation(const PhvInfo &phv, const PhvUse &uses) {
    return PHV::ConcreteAllocation(phv, uses);
}

/// remove superclusters with deparser zero fields.
/// deparsed-zero clusters will be appended to @p deparser_zero_superclusters, and returned
/// value will be (@p cluster_groups) minus deparsed-zero clusters.
std::list<PHV::SuperCluster *> remove_deparser_zero_superclusters(
    const std::list<PHV::SuperCluster *> &cluster_groups,
    std::list<PHV::SuperCluster *> &deparser_zero_superclusters) {
    std::list<PHV::SuperCluster *> rst;
    for (auto *sc : cluster_groups) {
        if (sc->is_deparser_zero_candidate()) {
            LOG_DEBUG4(TAB2 "Removing deparser zero supercluster from unallocated superclusters");
            deparser_zero_superclusters.push_back(sc);
            continue;
        }
        rst.push_back(sc);
    }
    return rst;
}

// SuperClusterDiagnoseInfo is ported from AllocatePHV::diagnoseSuperCluster.
// This struct and related analysis are not precise (high false positive rate).
// We should deprecate them once the table first alloc mode is ready for production.
struct SuperClusterActionDiagnoseInfo {
    // Identify if this is the minimal slice for this supercluster.
    // Conditions:
    // 1. The width of deparsed exact_containers slice lists in the supercluster is already 8b; or
    // 2. At least one deparsed exact_containers slice list is made up of no_split fields.
    bool scCannotBeSplitFurther = false;
    // Note down the offsets of the field slices within the slice list; the offset represents the
    // slices' alignments within their respective containers.
    ordered_map<PHV::FieldSlice, unsigned> fieldAlignments;
    // Only slice lists of interest are the ones with exact containers requirements
    ordered_set<const PHV::SuperCluster::SliceList *> sliceListsOfInterest;

    explicit SuperClusterActionDiagnoseInfo(const PHV::SuperCluster *sc) {
        auto &slice_lists = sc->slice_lists();
        for (const auto *list : slice_lists) {
            bool sliceListExact = false;
            int sliceListSize = 0;
            for (auto &slice : *list) {
                if (slice.field()->no_split()) scCannotBeSplitFurther = true;
                if (slice.field()->exact_containers()) sliceListExact = true;
                fieldAlignments[slice] = sliceListSize;
                sliceListSize += slice.size();
            }
            if (sliceListExact) {
                scCannotBeSplitFurther = true;
                sliceListsOfInterest.insert(list);
            }
        }
    }
};

void print_alloc_history(const PHV::Transaction &tx, const std::list<PHV::SuperCluster *> &clusters,
                         std::ostream &out) {
    std::set<PHV::FieldSlice> fs_allocated;
    for (auto *sc : clusters) {
        const auto slices = sc->slices();
        fs_allocated.insert(slices.begin(), slices.end());
    }

    for (const auto &container_status : tx.getTransactionStatus()) {
        // const auto& c = container_status.first;
        const auto &status = container_status.second;
        for (const auto &a : status.slices) {
            auto fs = PHV::FieldSlice(a.field(), a.field_slice());
            if (fs_allocated.count(fs)) {
                out << "allocate: " << a.container() << "[" << a.container_slice().lo << ":"
                    << a.container_slice().hi << "] <- " << fs << " @" << "["
                    << a.getEarliestLiveness() << "," << a.getLatestLiveness() << "]" << "\n";
            }
        }
    }
}

void print_or_throw_slicing_error(const PHV::AllocUtils &utils, const PHV::SuperCluster *sc,
                                  int n_slicing_tried) {
    auto &pa_container_sizes = utils.pragmas.pa_container_sizes();
    auto &meter_color_dests_8bit = utils.actions.meter_color_dests_8bit();
    ordered_set<const PHV::Field *> unsat_fields;
    sc->forall_fieldslices([&](const PHV::FieldSlice &fs) {
        const auto *f = fs.field();
        if (!unsat_fields.count(f) && pa_container_sizes.field_to_layout().count(f)) {
            unsat_fields.insert(f);
            if (meter_color_dests_8bit.count(f)) {
                if (f->size > 8) {
                    P4C_UNIMPLEMENTED(
                        "Currently the compiler only supports allocation "
                        "of meter color destination field %1% to an 8-bit container when "
                        "the meter color is also part of a larger operation (e.g. arithmetic, "
                        "bit operation). However, meter color destination %1% with size %2% "
                        "bits cannot be split based on its use. Therefore, it cannot be "
                        "allocated to an 8-bit container. Suggest using a meter color "
                        "destination that is less than or equal to 8b in size or simplify the "
                        "instruction relative to the meter color to a basic set.",
                        f->name, f->size);
                } else {
                    P4C_UNIMPLEMENTED(
                        "Currently the compiler only supports allocation of "
                        "meter color destination field %1% to an 8-bit container when "
                        "the meter color is also part of a larger operation (e.g. arithmetic, "
                        "bit operation). However, %1% cannot be allocated to an 8-bit "
                        "container.",
                        f->name);
                }
            }
        }
    });
    if (LOGGING(5)) {
        if (unsat_fields.size() > 0) {
            LOG_DEBUG5("Found " << unsat_fields.size() << " unsatisfiable fields:");
        }
        for (const auto *f : unsat_fields) {
            LOG_DEBUG5(TAB1 << f);
        }
    }
    if (unsat_fields.size() > 0) {
        std::stringstream ss;
        ss << "Cannot find a slicing to satisfy @pa_container_size pragma(s): ";
        std::string sep = "";
        for (const auto *f : unsat_fields) {
            ss << sep << f->name;
            sep = ", ";
        }
        ss << "\n" << sc;
        error("%1%", ss.str());
    } else if (n_slicing_tried == 0) {
        auto diagnose_info = SuperClusterActionDiagnoseInfo(sc);
        if (diagnose_info.scCannotBeSplitFurther) {
            std::stringstream ss;
            bool diagnosed = utils.actions.diagnoseSuperCluster(diagnose_info.sliceListsOfInterest,
                                                                diagnose_info.fieldAlignments, ss);
            if (diagnosed) {
                error("%1%", ss.str());
            } else {
                // SuperCluster can't be split any further, but it's not because
                // of field type or packing in actions (since diagnoseSuperCluster
                // returned false).
                //
                // If one of the slice lists contains a field that contains a number
                // of no_split bits that exceeds the size of the largest PHV container (32b),
                // then there's nothing the compiler can do to overcome that limitation
                // and the P4 code can't be compiled as-is.
                for (const auto &list : sc->slice_lists()) {
                    int no_split_size = 0;
                    int alignment_bits = 0;
                    std::optional<PHV::FieldSlice> prev_slice = std::nullopt;

                    for (const auto &slice : *list) {
                        if (!prev_slice || (prev_slice->field() != slice.field())) {
                            // First slice or new field in slice list.
                            no_split_size = 0;
                            if (slice.field()->exact_containers()) {
                                alignment_bits = 0;
                            } else {
                                if (slice.alignment()) alignment_bits = slice.alignment()->align;
                            }
                        }

                        if (slice.field()->no_split()) {
                            no_split_size += slice.size();

                            if ((no_split_size + alignment_bits) > int(PHV::Size::b32)) {
                                // Slice list too large for PHV container.
                                std::stringstream ss;
                                ss << "The slice list below contains " << no_split_size
                                   << " "
                                      "bits, the no_split attribute prevents it from being "
                                      "split any further, and it is too large to fit in the "
                                      "largest PHV containers."
                                   << "\n\n\t" << *list;
                                LOG_DEBUG3(ss);
                                error("%1%", ss.str());
                                return;
                            }
                        }
                        prev_slice = slice;
                    }
                }
            }
        }

        std::stringstream ss;
        ss << "Unable to slice the following group of fields due to unsatisfiable constraints: ";
        std::string sep = "";
        sc->forall_fieldslices([&](const PHV::FieldSlice &s) {
            ss << sep << s.field()->name;
            sep = ", ";
        });
        ss << std::endl;

        error("%1%", ss.str());
    }
}

const auto slicing_config =
    PHV::Slicing::IteratorConfig{false, true, false, false, false, (1 << 25), (1 << 19)};

// Create callback for creating FileLog objects
// Those can locally redirect LOG* macros to another file which
// will share path and suffix number with phv_allocation_*.log
// To restore original logging behaviour, call Logging::FileLog::close on the object.
Logging::FileLog *createFileLog(int pipeId, const cstring &prefix, int loglevel) {
    if (!LOGGING(loglevel)) return nullptr;

    auto filename = Logging::PassManager::getNewLogFileName(prefix);
    return new Logging::FileLog(pipeId, filename, Logging::Mode::AUTO);
}

class DummyParserPackingValidator : public PHV::ParserPackingValidatorInterface {
 public:
    const PHV::v2::AllocError *can_pack(const PHV::v2::FieldSliceAllocStartMap &,
                                        bool) const override {
        return nullptr;
    }
};

}  // namespace

PHV::Slicing::IteratorInterface *PHV::AllocUtils::make_slicing_ctx(
    const PHV::SuperCluster *sc) const {
    auto *packing_validator = new PHV::legacy::ActionPackingValidator(source_tracker, uses);
    auto *parser_packing_validator = new DummyParserPackingValidator();
    return new PHV::Slicing::ItrContext(
        phv, field_to_parser_states, parser_info, sc,
        pragmas.pa_container_sizes().field_to_layout(), *packing_validator,
        *parser_packing_validator,
        boost::bind(&AllocUtils::has_pack_conflict, this, boost::placeholders::_1,
                    boost::placeholders::_2),
        boost::bind(&AllocUtils::is_referenced, this, boost::placeholders::_1));
}

bool PHV::AllocUtils::can_physical_liverange_be_overlaid(const PHV::AllocSlice &a,
                                                         const PHV::AllocSlice &b) const {
    BUG_CHECK(a.isPhysicalStageBased() && b.isPhysicalStageBased(),
              "physical liverange overlay should be checked "
              "only when both slices are physical stage based");
    // TODO: need more thoughts on these constraints.
    const auto never_overlay = [](const PHV::Field *f) {
        return f->pov || f->deparsed_to_tm() || f->is_invalidate_from_arch();
    };
    if (never_overlay(a.field()) || never_overlay(b.field())) {
        return false;
    }
    BUG_CHECK(a.container() == b.container(),
              "checking overlay on different container: %1% and %2%", a.container(), b.container());
    const auto &cont = a.container();
    const bool is_a_deparsed = a.getLatestLiveness().first == Device::numStages();
    const bool is_b_deparsed = b.getLatestLiveness().first == Device::numStages();
    const bool both_deparsed = is_a_deparsed && is_b_deparsed;
    // TODO: The better and less constrainted checks are
    // `may be checksummed together` and `may be deparsed together`,
    // instead of will be both be deparsed. But since it is very likely that
    // the less constrainted checks will not give us any benefits (because
    // we are likely to have enough containers for those read-only fields),
    // we will just check whether they are both deparsed.
    if (both_deparsed) {
        // checksum engine cannot read a container multiple times.
        if (a.field()->is_checksummed() && b.field()->is_checksummed()) {
            return false;
        }
        // deparser cannot emit a non-8-bit container multiple times.
        if (cont.size() != 8) {
            return false;
        }
    }
    return pragmas.pa_no_overlay().can_overlay(a.field(), b.field()) && a.isLiveRangeDisjoint(b);
}

bool PHV::AllocUtils::is_clot_allocated(const ClotInfo &clots, const PHV::SuperCluster &sc) {
    // If the device doesn't support CLOTs, then don't bother checking.
    if (Device::numClots() == 0) return false;

    // In JBay, a clot-candidate field may sometimes be allocated to a PHV
    // container, eg. if it is adjacent to a field that must be packed into a
    // larger container, in which case the clot candidate would be used as
    // padding.

    // Check slice lists.
    bool needPhvAllocation = std::any_of(
        sc.slice_lists().begin(), sc.slice_lists().end(),
        [&](const PHV::SuperCluster::SliceList *slices) {
            return std::any_of(slices->begin(), slices->end(), [&](const PHV::FieldSlice &slice) {
                return !clots.fully_allocated(slice);
            });
        });

    // Check rotational clusters.
    needPhvAllocation |= std::any_of(
        sc.clusters().begin(), sc.clusters().end(), [&](const PHV::RotationalCluster *cluster) {
            return std::any_of(
                cluster->slices().begin(), cluster->slices().end(),
                [&](const PHV::FieldSlice &slice) { return !clots.fully_allocated(slice); });
        });

    return !needPhvAllocation;
}

std::list<PHV::SuperCluster *> PHV::AllocUtils::remove_singleton_metadata_slicelist(
    const std::list<PHV::SuperCluster *> &cluster_groups) {
    std::list<PHV::SuperCluster *> rst;
    for (auto *super_cluster : cluster_groups) {
        // Supercluster has more than one slice list.
        if (super_cluster->slice_lists().size() != 1) {
            rst.push_back(super_cluster);
            continue;
        }
        auto *slice_list = super_cluster->slice_lists().front();
        // The slice list has more than one fieldslice.
        if (slice_list->size() != 1) {
            rst.push_back(super_cluster);
            continue;
        }
        // The fieldslice is pov, or not metadata. Nor does supercluster is singleton.
        // TODO: These comments are vague, I assume that it was trying to find
        // super clusters that have
        // (1) only one slice list
        // (2) only one field in the slice list.
        // (3) only one field in the rotational cluster.
        // (4) no special constraints on the field.
        // and these super clusters does not need to have the field in a slice list because
        // the alignment searched for fields in slice lists is less than fields that are not.
        auto fs = slice_list->front();
        if (fs.size() != int(super_cluster->aggregate_size()) || !fs.field()->metadata ||
            fs.field()->exact_containers() || fs.field()->pov || fs.field()->is_checksummed() ||
            fs.field()->deparsed_bottom_bits() || fs.field()->is_marshaled()) {
            rst.push_back(super_cluster);
            continue;
        }

        ordered_set<PHV::SuperCluster::SliceList *> empty;
        PHV::SuperCluster *new_cluster = new PHV::SuperCluster(super_cluster->clusters(), empty);
        rst.push_back(new_cluster);
        LOG_DEBUG5("Replacing singleton " << super_cluster << " with " << new_cluster);
    }
    return rst;
}

std::list<PHV::SuperCluster *> PHV::AllocUtils::remove_unref_clusters(
    const PhvUse &uses, const std::list<PHV::SuperCluster *> &cluster_groups_input) {
    std::list<PHV::SuperCluster *> cluster_groups_filtered;
    for (auto *sc : cluster_groups_input) {
        if (sc->all_of_fieldslices([&](const PHV::FieldSlice &fs) {
                return !uses.is_allocation_required(fs.field());
            })) {
            continue;
        }
        cluster_groups_filtered.push_back(sc);
    }
    return cluster_groups_filtered;
}

std::list<PHV::SuperCluster *> PHV::AllocUtils::remove_clot_allocated_clusters(
    const ClotInfo &clot, std::list<PHV::SuperCluster *> clusters) {
    auto res = std::remove_if(clusters.begin(), clusters.end(), [&](const PHV::SuperCluster *sc) {
        return AllocUtils::is_clot_allocated(clot, *sc);
    });
    clusters.erase(res, clusters.end());
    return clusters;
}

const std::vector<MetricName> AllocScore::g_general_metrics = {
    n_tphv_on_phv_bits,     n_mocha_on_phv_bits, n_dark_on_phv_bits, n_dark_on_mocha_bits,
    n_prefer_bits,          n_bitmasked_set,     n_wasted_pov_bits,  parser_extractor_balance,
    n_inc_tphv_collections,
};

const std::vector<MetricName> AllocScore::g_by_kind_metrics = {
    n_set_gress,
    n_set_parser_group_gress,
    n_set_deparser_group_gress,
    n_set_parser_extract_group_source,
    n_overlay_bits,
    n_packing_bits,
    n_packing_priority,
    n_inc_containers,
    n_wasted_bits,
    n_inc_small_containers,
    n_clot_bits,
    n_field_packing_score,
    n_mismatched_deparser_gress,
};

AllocScore AllocScore::operator-(const AllocScore &right) const {
    AllocScore rst = *this;  // copy
    for (const auto &m : AllocScore::g_general_metrics) {
        rst.general[m] -= right.general.count(m) ? right.general.at(m) : 0;
    }

    for (auto kind : Device::phvSpec().containerKinds()) {
        if (!right.by_kind.count(kind)) {
            continue;
        }
        const auto &rs = right.by_kind.at(kind);
        for (const auto &m : AllocScore::g_by_kind_metrics) {
            if (rs.count(m)) {
                rst.by_kind[kind][m] -= rs.at(m);
            }
        }
    }
    return rst;
}

/** The metrics are calculated:
 * + is_tphv: type of @p group.
 * + n_set_gress:
 *     number of containers which set their gress
 *     to ingress/egress from std::nullopt.
 * + n_overlay_bits: container bits already used in parent alloc get overlaid.
 * + n_field_packing_score: container bits that share the same byte for two or
 *     more slices used as a condition in a match table.
 * + n_packing_bits: use bits that ContainerAllocStatus is PARTIAL in parent.
 * + n_inc_containers: the number of container used that was EMPTY.
 * + n_wasted_bits: if field is solitary, container.size() - slice.width().
 */
AllocScore::AllocScore(const PHV::Transaction &alloc, const PhvInfo &phv, const ClotInfo &clot,
                       const PhvUse &uses, const MapFieldToParserStates &field_to_parser_states,
                       const CalcParserCriticalPath &parser_critical_path,
                       const TableFieldPackOptimization &tablePackOpt,
                       FieldPackingOpportunity *packing_opportunities, const int bitmasks) {
    using ContainerAllocStatus = PHV::Allocation::ContainerAllocStatus;
    const auto *parent = alloc.getParent();

    general[n_bitmasked_set] = bitmasks;

    ordered_set<std::pair<const PHV::Field *, PHV::Container>> wasted_bits_counted;

    // Forall allocated slices group by container.
    for (const auto &kv : alloc.getTransactionStatus()) {
        const auto &container = kv.first;
        const auto kind = container.type().kind();
        const auto &gress = kv.second.gress;
        const auto &parserGroupGress = kv.second.parserGroupGress;
        const auto &deparserGroupGress = kv.second.deparserGroupGress;
        const auto &parserExtractGroupSource = kv.second.parserExtractGroupSource;
        bitvec parent_alloc_vec = calcContainerAllocVec(parent->slices(container));
        // The set of slices that are allocated in this transaction, by subtracting out
        // slices allocated in parent, robust in that @p alloc can commit things that
        // are already in the parent.
        auto slices = kv.second.slices;
        parent->foreach_slice(container,
                              [&](const PHV::AllocSlice &slice) { slices.erase(slice); });

        // skip, if there is no allocated slices.
        if (slices.size() == 0) continue;

        // Calculate number of wasted container bits, if POV bits are present in this container.
        size_t n_pov_bits = 0;
        for (const auto &slice : slices)
            if (slice.field()->pov) n_pov_bits += slice.width();
        if (n_pov_bits != 0)
            general[n_wasted_pov_bits] +=
                (container.size() > n_pov_bits ? (container.size() - n_pov_bits) : 0);

        // Calculate number of prefer container size bits.
        for (const auto &slice : slices) {
            if (slice.field()->prefer_container_size() != PHV::Size::null) {
                if (parent->getStatus(slice.field()).empty()) {
                    if (container.is(slice.field()->prefer_container_size()))
                        general[n_prefer_bits] += slice.width();
                }
            }
        }

        // calc n_wasted_bits and n_clot_bits
        for (const auto &slice : slices) {
            if (!slice.field()->is_solitary()) continue;
            PHV::FieldSlice field_slice(slice.field(), slice.field_slice());
            if (wasted_bits_counted.count({slice.field(), container})) {
                // correct over-counted bits
                by_kind[kind][n_wasted_bits] -= slice.field()->size;
            } else {
                // Assume all other bits are wasted because of this slice.
                // The number will be corrected when fieldslice of the same field
                // enter the if branch above.
                by_kind[kind][n_wasted_bits] += (container.size() - field_slice.size());
                wasted_bits_counted.insert({slice.field(), container});
            }
        }

        // calc n_clot_bits
        for (const auto &slice : slices) {
            // Loop over all CLOT-allocated slices that overlap with "slice" and add the number of
            // overlapping bits to the score.
            PHV::FieldSlice field_slice(slice.field(), slice.field_slice());
            const auto &slice_range = field_slice.range();
            for (auto *clotted_slice : Keys(*clot.slice_clots(&field_slice))) {
                const auto &clot_range = slice_range.intersectWith(clotted_slice->range());
                by_kind[kind][n_clot_bits] += clot_range.size();
            }
        }

        if (kind == PHV::Kind::normal) {
            for (const auto &slice : slices) {
                PHV::FieldSlice fieldSlice(slice.field(), slice.field_slice());
                if (Device::currentDevice() == Device::TOFINO) {
                    if (fieldSlice.is_tphv_candidate(uses))
                        general[n_tphv_on_phv_bits] += (slice.width());
                } else {
                    if (slice.field()->is_mocha_candidate())
                        general[n_mocha_on_phv_bits] += (slice.width());
                    else if (slice.field()->is_dark_candidate())
                        general[n_dark_on_phv_bits] += (slice.width());
                }
            }
        }

        if (kind == PHV::Kind::mocha)
            for (const auto &slice : slices)
                if (slice.field()->is_dark_candidate())
                    general[n_dark_on_mocha_bits] += (slice.width());

        // calc_n_inc_containers
        auto merged_status = alloc.alloc_status(container);
        auto parent_status = parent->alloc_status(container);
        if (parent_status == ContainerAllocStatus::EMPTY &&
            merged_status != ContainerAllocStatus::EMPTY) {
            by_kind[kind][n_inc_containers]++;

            // calc n_inc_small_containers
            if (container.size() < 32 && container.type().kind() == PHV::Kind::normal) {
                by_kind[kind][n_inc_small_containers]++;
            }
        }

        int pack_score = tablePackOpt.getPackScore(parent->slices(container), slices);
        by_kind[kind][n_field_packing_score] += pack_score;

        if (parent_status == ContainerAllocStatus::PARTIAL) {
            // calc n_packing_bits
            for (auto i = container.lsb(); i <= container.msb(); ++i) {
                if (parent_alloc_vec[i]) continue;
                for (const auto &slice : slices) {
                    if (slice.container_slice().contains(i)) {
                        by_kind[kind][n_packing_bits]++;
                        break;
                    }
                }
            }

            // calc n_packing_priority.
            if (packing_opportunities && by_kind[kind][n_packing_bits]) {
                int n_packing_priorities = 100000;  // do not use max because it might overflow.
                for (auto i = container.lsb(); i <= container.msb(); ++i) {
                    parent->foreach_slice(container, [&](const PHV::AllocSlice &p_slice) {
                        for (const auto &slice : slices) {
                            if (p_slice.container_slice().contains(i) &&
                                slice.container_slice().contains(i)) {
                                auto *f1 = p_slice.field();
                                auto *f2 = slice.field();
                                n_packing_priorities =
                                    std::min(n_packing_priorities,
                                             packing_opportunities->nOpportunitiesAfter(f1, f2));
                            }
                        }
                    });
                }
                by_kind[kind][n_packing_priority] = n_packing_priorities;
            }
        }

        // calc n_overlay_bits
        for (const int i : parent_alloc_vec) {
            for (const auto &slice : slices) {
                if (slice.container_slice().contains(i)) {
                    by_kind[kind][n_overlay_bits]++;
                }
            }
        }

        // If overlay is between multiple fields in the same transaction,
        // then that value needs to be calculated separately.
        int overlay_bits = 0;
        for (const auto &slice1 : slices) {
            for (const auto &slice2 : slices) {
                if (slice1 == slice2) continue;
                for (unsigned int i = 0; i < container.size(); i++) {
                    if (slice1.container_slice().contains(i) &&
                        slice2.container_slice().contains(i))
                        overlay_bits++;
                }
            }
        }
        // Slices are counted twice in the above loop, so divide by 2.
        by_kind[kind][n_overlay_bits] += (overlay_bits / 2);

        LOG_FEATURE("alloc_progress", 9,
                    TAB2 "Score for gress = " << gress << " parserGroupGress = " << parserGroupGress
                                              << " deparserGroupGress = " << deparserGroupGress);
        // gress
        if (!parent->gress(container) && gress) {
            LOG_FEATURE("alloc_progress", 9, TAB2 "Increasing n_set_gress for " << container);
            by_kind[kind][n_set_gress]++;
            if (gress != deparserGroupGress) {
                LOG_FEATURE("alloc_progress", 9,
                            TAB2 "Increasing n_mismatched_deparser_gress for "
                                << container << " (" << gress << " x " << deparserGroupGress
                                << ")");
                by_kind[kind][n_mismatched_deparser_gress]++;
            }
        }

        // Parser group gress
        if (!parent->parserGroupGress(container) && parserGroupGress) {
            LOG_FEATURE("alloc_progress", 9,
                        TAB2 "Increasing n_set_parser_group_gress for " << container);
            by_kind[kind][n_set_parser_group_gress]++;
        }

        // Deparser group gress
        if (!parent->deparserGroupGress(container) && deparserGroupGress) {
            LOG_FEATURE("alloc_progress", 9,
                        TAB2 "Increasing n_set_deparser_group_gress for " << container);
            by_kind[kind][n_set_deparser_group_gress]++;
        }

        // Parser extract group source
        if (Device::phvSpec().hasParserExtractGroups() &&
            parent->parserExtractGroupSource(container) == PHV::Allocation::ExtractSource::NONE &&
            parserExtractGroupSource != PHV::Allocation::ExtractSource::NONE) {
            by_kind[kind][n_set_parser_extract_group_source]++;
        }
    }

    // calc_n_inc_tphv_collections
    if (Device::currentDevice() == Device::TOFINO) {
        auto my_tphv_collections = alloc.getTagalongCollectionsUsed();
        auto parent_tphv_collections = parent->getTagalongCollectionsUsed();
        for (auto col : my_tphv_collections) {
            if (!parent_tphv_collections.count(col)) general[n_inc_tphv_collections]++;
        }
    }

    if (Device::currentDevice() == Device::TOFINO) {
        // This only matters for Tofino.
        // For JBay, all extractors are of same size (16-bit).
        calcParserExtractorBalanceScore(alloc, phv, field_to_parser_states, parser_critical_path);
    }
}

void AllocScore::calcParserExtractorBalanceScore(
    const PHV::Transaction &alloc, const PhvInfo &phv,
    const MapFieldToParserStates &field_to_parser_states,
    const CalcParserCriticalPath &parser_critical_path) {
    const auto *parent = alloc.getParent();

    ordered_map<const IR::BFN::ParserState *, std::set<PHV::Container>>
        critical_state_to_containers;

    auto &my_state_to_containers = alloc.getParserStateToContainers(phv, field_to_parser_states);
    auto &parent_state_to_containers =
        parent->getParserStateToContainers(phv, field_to_parser_states);

    // If program has user specified critical states, we will only compute score for those.
    //
    // Otherwise, we compute the score on the critical path (path with most extracted bits),
    // if --parser-bandwidth-opt is turned on or pa_parser_bandwidth_opt pragma is used.
    //
    if (!parser_critical_path.get_ingress_user_critical_states().empty() ||
        !parser_critical_path.get_egress_user_critical_states().empty()) {
        for (auto &kv : my_state_to_containers) {
            if (parser_critical_path.is_user_specified_critical_state(kv.first))
                critical_state_to_containers[kv.first].insert(kv.second.begin(), kv.second.end());
        }

        for (auto &kv : parent_state_to_containers) {
            if (parser_critical_path.is_user_specified_critical_state(kv.first))
                critical_state_to_containers[kv.first].insert(kv.second.begin(), kv.second.end());
        }
    } else if (BackendOptions().parser_bandwidth_opt) {
        for (auto &kv : my_state_to_containers) {
            if (parser_critical_path.is_on_critical_path(kv.first))
                critical_state_to_containers[kv.first].insert(kv.second.begin(), kv.second.end());
        }

        for (auto &kv : parent_state_to_containers) {
            if (parser_critical_path.is_on_critical_path(kv.first))
                critical_state_to_containers[kv.first].insert(kv.second.begin(), kv.second.end());
        }
    }

    for (auto &kv : critical_state_to_containers) {
        StateExtractUsage use(kv.second);
        general[parser_extractor_balance] += ParserExtractScore::get_score(use);
    }
}

bitvec AllocScore::calcContainerAllocVec(const ordered_set<PHV::AllocSlice> &slices) {
    bitvec allocatedBits;
    for (const auto &slice : slices) {
        allocatedBits |= bitvec(slice.container_slice().lo, slice.container_slice().size());
    }
    return allocatedBits;
}

std::ostream &operator<<(std::ostream &s, const AllocScore &score) {
    s << "{";
    for (const auto &m : AllocScore::g_general_metrics) {
        if (m == n_wasted_pov_bits && Device::currentDevice() == Device::TOFINO) {
            continue;
        }
        if (score.general.count(m) && score.general.at(m) > 0) {
            s << m << ": " << score.general.at(m) << ", ";
        }
    }

    for (auto kind : Device::phvSpec().containerKinds()) {
        if (!score.by_kind.count(kind)) continue;
        s << kind << "[";
        for (const auto &m : AllocScore::g_by_kind_metrics) {
            if (score.by_kind.at(kind).count(m) && score.by_kind.at(kind).at(m) > 0) {
                s << m << ": " << score.by_kind.at(kind).at(m) << ", ";
            }
        }
        s << "], ";
    }
    s << "}";
    return s;
}

AllocScore ScoreContext::make_score(const PHV::Transaction &alloc, const PhvInfo &phv,
                                    const ClotInfo &clot, const PhvUse &uses,
                                    const MapFieldToParserStates &f_ps,
                                    const CalcParserCriticalPath &cp,
                                    const TableFieldPackOptimization &tp_opt,
                                    const int bitmasks) const {
    return AllocScore(alloc, phv, clot, uses, f_ps, cp, tp_opt, packing_opportunities_i, bitmasks);
}

// Check if all slices are mutually exclusive with field
/* static */
bool CoreAllocation::can_overlay(const SymBitMatrix &mutex, const PHV::Field *f,
                                 const ordered_set<PHV::AllocSlice> &slices) {
    for (const auto &slice : slices)
        if (!mutex(f->id, slice.field()->id)) return false;
    return true;
}

// Check if there is at least one mutually exclusive slice in container
bool CoreAllocation::some_overlay(const SymBitMatrix &mutex, const PHV::Field *f,
                                  const ordered_set<PHV::AllocSlice> &slices) {
    for (const auto &slice : slices)
        if (mutex(f->id, slice.field()->id)) return true;
    return false;
}

bool CoreAllocation::can_physical_liverange_overlay(
    const PHV::AllocSlice &slice, const ordered_set<PHV::AllocSlice> &allocated) const {
    BUG_CHECK(slice.isPhysicalStageBased(),
              "slice must be physical-stage-based in can_physical_liverange_overlay");
    for (const auto &other : allocated) {
        BUG_CHECK(other.isPhysicalStageBased(),
                  "slice must be physical-stage-based in can_physical_liverange_overlay");
        if (!utils_i.can_physical_liverange_be_overlaid(slice, other)) {
            return false;
        }
    }
    return true;
}

bool CoreAllocation::hasARAinits(ordered_set<PHV::AllocSlice> slices) const {
    for (auto sl : slices) {
        if (sl.getInitPrimitive().isAlwaysRunActionPrim()) {
            LOG_DEBUG5(TAB2 ".. some AllocSlices have ARA inits");
            return true;
        }
    }
    return false;
}

bool CoreAllocation::satisfies_constraints(const PHV::ContainerGroup &group,
                                           const PHV::Field *f) const {
    // Check that TM deparsed fields aren't split
    if (f->no_split() && int(group.width()) < f->size) {
        LOG_DEBUG5(TAB1 "Constraint violation: Can't split field size "
                   << f->size << " across " << group.width() << " containers");
        return false;
    }
    return true;
}

bool CoreAllocation::satisfies_constraints(const PHV::ContainerGroup &group,
                                           const PHV::FieldSlice &slice) const {
    auto req = utils_i.pragmas.pa_container_sizes().expected_container_size(slice);
    if (req && !group.is(*req)) {
        LOG_DEBUG5(TAB1 "Constraint violation: " << slice << " must go to " << *req
                                                 << " because of @pa_container_size");
        return false;
    }
    return satisfies_constraints(group, slice.field());
}

bool CoreAllocation::satisfies_constraints(std::vector<PHV::AllocSlice> slices,
                                           const PHV::Allocation &alloc) const {
    if (slices.size() == 0) return true;
    // not exceeding container bound.
    for (const auto &slice : slices) {
        if (slice.container_slice().hi > int(slice.container().size()) - 1) {
            return false;
        }
    }

    // pa_container_type constraint check
    auto &pa_ct = utils_i.pragmas.pa_container_type();
    for (const auto &slice : slices) {
        auto required_kind = pa_ct.required_kind(slice.field());
        if (required_kind && *required_kind != slice.container().type().kind()) {
            return false;
        }
    }

    // Slices placed together must be placed in the same container.
    auto DifferentContainer = [](const PHV::AllocSlice &left, const PHV::AllocSlice &right) {
        return left.container() != right.container();
    };
    if (std::adjacent_find(slices.begin(), slices.end(), DifferentContainer) != slices.end()) {
        LOG_DEBUG5(TAB1
                   "Constraint violation: Slices placed together must be placed "
                   "in the same container: "
                   << slices);
        return false;
    }

    // Check exact containers for deparsed fields
    auto IsDeparsed = [](const PHV::AllocSlice &slice) {
        return (slice.field()->deparsed() || slice.field()->exact_containers());
    };
    auto IsDeparsedOrDigest = [](const PHV::AllocSlice &slice) {
        return (slice.field()->deparsed() || slice.field()->exact_containers() ||
                slice.field()->is_digest());
    };
    if (std::any_of(slices.begin(), slices.end(), IsDeparsed)) {
        // Reject mixes of deparsed/not deparsed fields.
        if (!std::all_of(slices.begin(), slices.end(), IsDeparsedOrDigest)) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: Mix of deparsed/not deparsed fields "
                       "cannot be placed together: "
                       << slices);
            return false;
        }

        // Calculate total size of slices.
        int aggregate_size = 0;
        int container_size = 0;
        for (auto &slice : slices) {
            aggregate_size += slice.width();
            container_size = int(slice.container().size());
        }

        // Reject slices that cannot fit in the container.
        if (container_size < aggregate_size) {
            LOG_DEBUG5(TAB1 "Constraint violation: Slices placed together are "
                       << aggregate_size << "b wide and cannot fit in an " << container_size
                       << "b container");
            return false;
        }

        // Reject slices that do not totally fill the container.
        if (container_size > aggregate_size) {
            LOG_DEBUG5(TAB1 "Constraint violation: Deparsed slices placed together are "
                       << aggregate_size << "b wide but do not completely fill " << container_size
                       << "b container");
            return false;
        }
    }

    // Check if any fields have the solitary constraint, which is mutually
    // unsatisfiable with slice lists, which induce packing.  Ignore adjacent slices of the same
    // field.
    std::vector<PHV::AllocSlice> used;
    ordered_set<const PHV::Field *> pack_fields;
    for (auto &slice : slices) {
        bool isDeparsedOrMau =
            utils_i.uses.is_deparsed(slice.field()) || utils_i.uses.is_used_mau(slice.field());
        bool overlayablePadding = slice.field()->overlayable;
        if (isDeparsedOrMau && !overlayablePadding) used.push_back(slice);
        if (!slice.field()->is_solitary()) pack_fields.insert(slice.field());
    }
    auto NotAdjacent = [](const PHV::AllocSlice &left, const PHV::AllocSlice &right) {
        return left.field() != right.field() ||
               left.field_slice().hi + 1 != right.field_slice().lo ||
               left.container_slice().hi + 1 != right.container_slice().lo;
    };
    auto NoPack = [](const PHV::AllocSlice &s) { return s.field()->is_solitary(); };
    bool not_adjacent = std::adjacent_find(used.begin(), used.end(), NotAdjacent) != used.end();
    bool solitary = std::find_if(used.begin(), used.end(), NoPack) != used.end();
    if (not_adjacent && solitary) {
        if (!std::all_of(pack_fields.begin(), pack_fields.end(),
                         [](const PHV::Field *f) { return f->padding; })) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: Slice list contains multiple fields, one of "
                       "them having 'no pack' constraint: "
                       << slices);
            return false;
        }
    }

    // Reject allocation of slices of the same deparsed field if the Less Significant bits
    // of the field are allocated to more significant container bits
    for (auto &cand_sl : slices) {
        auto *f = cand_sl.field();
        // Only apply check to deparsed fields
        if (!(f->deparsed() || f->deparsed_to_tm() || f->is_digest())) continue;

        for (auto &alloc_sl : alloc.slices(cand_sl.field())) {
            LOG_DEBUG5("\t" << alloc_sl);
            // Check allocated slices of the same field on the same container.
            if (cand_sl.container() != alloc_sl.container()) continue;

            bool cand_fslice_lsbs = (cand_sl.field_slice().hi < alloc_sl.field_slice().lo);
            bool cand_cont_lsbs = (cand_sl.container_slice().hi < alloc_sl.container_slice().lo);
            LOG_DEBUG5(" lsbs " << cand_fslice_lsbs << " - " << cand_cont_lsbs
                                << " cand = " << cand_sl << "  alloc = " << alloc_sl);
            if (cand_fslice_lsbs != cand_cont_lsbs) {
                LOG_DEBUG1("\t Reject allocation of the same deparsed field "
                           << f->name
                           << "  when the Less-Signifiant bits of the field are in the more "
                              "significant container bits");
                return false;
            }
        }
    }

    // Check sum of constraints.
    ordered_set<const PHV::Field *> containerBytesFields;
    ordered_map<const PHV::Field *, int> allocatedBitsInThisTransaction;
    for (auto &slice : slices) {
        if (slice.field()->hasMaxContainerBytesConstraint())
            containerBytesFields.insert(slice.field());
        allocatedBitsInThisTransaction[slice.field()] += slice.width();
    }

    // Check that slices that appear in wide arithmetic operations
    // are being allocated to the correct odd/even container number.
    for (auto &alloc_slice : slices) {
        if (alloc_slice.field()->used_in_wide_arith()) {
            auto cidx = alloc_slice.container().index();
            int lo_bit = alloc_slice.field_slice().lo;
            LOG_DEBUG7(TAB2 "cidx = " << cidx << " and lo_bit = " << lo_bit);
            if (alloc_slice.field()->bit_is_wide_arith_lo(lo_bit)) {
                if ((cidx % 2) != 0) {
                    LOG_DEBUG5(TAB1
                               "Constraint violation: Starting wide "
                               "arith lo at odd container");
                    return false;
                }
            } else {
                if ((cidx % 2) == 0) {
                    LOG_DEBUG5(TAB1
                               "Constraint violation: Starting wide "
                               "arith hi at even container");
                    return false;
                }

                // Find the lo AllocSlice for the field.
                // Check that it is one less than the current container.
                // Note: Allocation approach assumes that the lo slice is
                // allocated before the hi.
                bool found = false;
                for (auto &as : alloc.slices(alloc_slice.field())) {
                    // It is possible that there are multiple slices in the hi part of the
                    // slice list. E.g. field[32:47], field[48:63]. So for the latter field, we need
                    // to check the lo_bit corresponding to the nearest 32-bit interval. The
                    // following transformation of lo_bit is, therefore, necessary.
                    lo_bit -= (lo_bit % 32);
                    if (as.field() == alloc_slice.field()) {
                        int lo_bit2 = as.field_slice().lo;
                        if (lo_bit2 == (lo_bit - 32)) {
                            found = true;
                            LOG_DEBUG6(TAB1 "Found linked lo slice " << as);
                            auto lo_container_idx = as.container().index();
                            if (cidx != (lo_container_idx + 1)) {
                                LOG_DEBUG5(TAB1
                                           "Constraint violation: Starting wide arith hi at "
                                           "non-adjacent container");
                                return false;
                            }
                            break;
                        }
                    }
                }
                BUG_CHECK(found,
                          "Unable to find lo field slice associated with"
                          " wide arithmetic operation %1%",
                          alloc_slice);
            }
        }
    }

    bool isExtracted = false;
    for (const auto &slice : slices) {
        if (utils_i.uses.is_extracted(slice.field())) {
            isExtracted = true;
            break;
        }
    }
    // Check if two different fields are extracted in different mutex states
    // - Two different field can be extracted in same state
    // - Same field can be extracted in two non-mutex state(This includes clear-on-writes,
    //   and local parser variables)
    // - Constant extracts and strided headers are excluded
    int container_size = 0;
    if (isExtracted) {
        ordered_map<const IR::BFN::ParserState *, bitvec> state_to_vec;
        cstring prev_field;
        for (const auto &slice : slices) {
            auto field = slice.field();
            container_size = slice.container().size();
            if (field->padding || field->pov || field->is_solitary()) continue;
            if (!utils_i.field_to_parser_states.field_to_writes.count(slice.field())) continue;
            if (utils_i.strided_headers.get_strided_group(field)) continue;
            for (auto write : utils_i.field_to_parser_states.field_to_writes.at(field)) {
                auto state = utils_i.field_to_parser_states.write_to_state.at(write);
                if (write->getWriteMode() == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE) continue;
                auto extract = write->to<IR::BFN::Extract>();
                auto range = extract ? extract->source->to<IR::BFN::PacketRVal>() : nullptr;
                /// FIXME(vstill): the mutex check seems to be needed also in cases where the data
                /// are not comming from the packet
                if (range) {
                    if (!state_to_vec.count(state) && prev_field != field->name) {
                        auto parser = utils_i.field_to_parser_states.state_to_parser.at(state);
                        for (auto &sv : state_to_vec) {
                            if (!utils_i.parser_info.graph(parser).is_mutex(sv.first, state)) {
                                LOG_DEBUG5(TAB1
                                           "Constraint violation: Extracting fields in the "
                                           "slice list in non-mutex states "
                                           << sv.first->name << " and " << state->name);
                                return false;
                            }
                        }
                    }
                    auto extract_field = extract->dest->to<IR::BFN::FieldLVal>();
                    auto slice_range = slice.field_slice();
                    // If extraction happens over a field slice, then find if this
                    // particular slice is being extracted or not
                    if (auto extract_slice = extract_field->field->to<IR::Slice>()) {
                        int min = std::min(slice_range.hi,
                                           extract_slice->e1->to<IR::Constant>()->asInt());
                        int max = std::max(slice_range.lo,
                                           extract_slice->e2->to<IR::Constant>()->asInt());
                        // overlap range of field_slice and extract_slice is [max,min]
                        if (max <= min) {
                            state_to_vec[state].setrange(range->range.lo + max, (min - max));
                        }
                    } else {
                        state_to_vec[state].setrange(range->range.hi - slice.field_slice().hi,
                                                     slice.width());
                    }
                }
                prev_field = field->name;
            }
        }
        for (auto &sv : state_to_vec) {
            if (sv.second.max().index() - sv.second.min().index() + 1 > container_size) {
                LOG_DEBUG5(TAB1 "Constraint violation: Extraction size exceeds container size");
                return false;
            }
        }
    }

    // If there are no fields that have the max container bytes constraints, then return true.
    if (containerBytesFields.size() == 0) return true;

    PHV::Container container = slices.begin()->container();
    int containerBits = 0;
    LOG_DEBUG6(TAB1 "Container bit: " << containerBits);
    for (const auto *f : containerBytesFields) {
        auto allocated_slices = alloc.slices(f);
        ordered_set<PHV::Container> containers;
        int allocated_bits = 0;
        for (auto &slice : allocated_slices) {
            LOG_DEBUG6(TAB2 "Slice: " << slice);
            containers.insert(slice.container());
            allocated_bits += slice.width();
        }
        if (allocated_bits != f->size) allocated_bits += allocatedBitsInThisTransaction.at(f);
        containers.insert(container);
        int unallocated_bits = f->size - allocated_bits;
        for (auto &c : containers) {
            LOG_DEBUG6(TAB2 "Container: " << c);
            containerBits += static_cast<int>(c.size());
        }
        LOG_DEBUG6(TAB2 "Field: " << f->name
                                  << "\n\t"
                                     "allocated_bits: "
                                  << allocated_bits << ", container_bits: " << containerBits
                                  << ", unallocated_bits: " << unallocated_bits);
        BUG_CHECK(containerBits % 8 == 0, "Sum of container bits cannot be non byte multiple.");
        int containerBytes = (containerBits / 8) + ROUNDUP(unallocated_bits, 8);
        if (containerBytes > f->getMaxContainerBytes()) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: Violating maximum container bytes allowed for "
                       "field "
                       << f->name);
            return false;
        }
    }
    return true;
}

// NB: action-induced PHV constraints are checked separately as part of
// `can_pack` on slice lists.
bool CoreAllocation::satisfies_constraints(const PHV::Allocation &alloc,
                                           const PHV::AllocSlice &slice,
                                           ordered_set<PHV::AllocSlice> &initFields,
                                           std::vector<PHV::AllocSlice> &candidate_slices) const {
    const PHV::Field *f = slice.field();
    PHV::Container c = slice.container();
    const auto *container_status = alloc.getStatus(c);

    // Check container limitation.
    if (slice.field()->limited_container_ids()) {
        const auto &ok_ids = slice.field()->limited_container_ids();
        if (!ok_ids->getbit(Device::phvSpec().containerToId(slice.container()))) {
            LOG_DEBUG5(TAB1 "Constraint violation: Container " << c << " is not allowed for  "
                                                               << f);
            return false;
        };
    }

    // Check gress.
    auto containerGress = alloc.gress(c);
    if (containerGress && *containerGress != f->gress) {
        LOG_DEBUG5(TAB1 "Constraint violation: Container " << c << " is " << *containerGress
                                                           << " but slice needs " << f->gress);
        return false;
    }

    // Check parser group constraints.

    auto parserGroupGress = alloc.parserGroupGress(c);
    auto parserExtractGroupSource = alloc.parserExtractGroupSource(c);
    bool isExtracted = utils_i.uses.is_extracted(f);
    bool isExtractedFromPkt = utils_i.uses.is_extracted_from_pkt(f);

    // Check 1: all containers within parser group must have same gress assignment
    if (parserGroupGress) {
        if ((isExtracted || singleGressParserGroups) && (*parserGroupGress != f->gress)) {
            LOG_DEBUG5(TAB1 "Constraint violation: Container "
                       << c << " has parser group gress " << *parserGroupGress
                       << " but slice needs " << f->gress
                       << " (singleGressParserGroups = " << singleGressParserGroups << ")");
            return false;
        }
    }

    std::optional<IR::BFN::ParserWriteMode> write_mode;

    if (isExtracted) {
        auto it = utils_i.field_to_parser_states.field_to_writes.find(f);
        if (it != utils_i.field_to_parser_states.field_to_writes.end()) {
            if (!it->second.empty()) {
                write_mode = (*it->second.begin())->getWriteMode();
            }
        }

        // W0 is not allowed to be used with clear_on_write
        // W0 is a 32-bit container, and it will be the only container of its parser group,
        // so we do not need to check other containers of its parser group.
        if ((Device::currentDevice() == Device::JBAY) &&
            c == PHV::Container({PHV::Kind::normal, PHV::Size::b32}, 0) &&
            write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE) {
            LOG_DEBUG5(TAB1 "W0 is not allowed to be used with clear_on_write due to hw bug.");
            return false;
        }
    }

    // Check 2: all constainers within parser group must have same parser write mode
    if (isExtracted && parserGroupGress) {
        const PhvSpec &phvSpec = Device::phvSpec();
        unsigned slice_cid = phvSpec.containerToId(slice.container());

        for (auto sl : (*container_status).slices) {
            auto container_field = sl.field();
            if (container_field == f) {
                continue;
            }
            if (!utils_i.field_to_parser_states.field_to_writes.count(container_field)) continue;
            for (auto wf : utils_i.field_to_parser_states.field_to_writes.at(f)) {
                auto ef = wf->to<IR::BFN::Extract>();
                if (!ef) continue;
                auto ef_state = utils_i.field_to_parser_states.write_to_state.at(ef);
                for (auto wsl :
                     utils_i.field_to_parser_states.field_to_writes.at(container_field)) {
                    auto esl = wsl->to<IR::BFN::Extract>();
                    if (!esl) continue;
                    auto esl_state = utils_i.field_to_parser_states.write_to_state.at(esl);
                    // A container cannot have same extract in same state
                    if (esl_state == ef_state && ef->source->equiv(*esl->source) &&
                        ef->source->is<IR::BFN::PacketRVal>()) {
                        LOG_DEBUG5(TAB1 "Constraint violation: Slices of field "
                                   << f << " and " << container_field
                                   << " have same extract source in the same "
                                      "state "
                                   << esl_state->name);
                        return false;
                    }
                }
            }
        }

        BUG_CHECK(write_mode, "parser write mode not exist for extracted field %1%", f->name);

        for (unsigned cid : phvSpec.parserGroup(slice_cid)) {
            auto other = phvSpec.idToContainer(cid);
            if (c == other) continue;

            const auto *cs = alloc.getStatus(other);
            if (cs) {
                for (auto sl : (*cs).slices) {
                    if (!utils_i.field_to_parser_states.field_to_writes.count(sl.field())) continue;

                    std::optional<IR::BFN::ParserWriteMode> other_write_mode;

                    for (auto e : utils_i.field_to_parser_states.field_to_writes.at(sl.field())) {
                        other_write_mode = e->getWriteMode();
                        // In tofino2, all extractions happen using 16b extracts.
                        // So a 16-bit parser extractor extracts over a pair of even and
                        // odd phv 8-bit containers to perforn 8-bit extraction.
                        // If any of 8 -bit containers in the pair  are in CLEAR_ON_WRITE mode,
                        // then both containers will be cleared everytime an extraction happens.
                        // In order to avoid this corruption, if one container in the
                        // pair in in CLEAR_ON_WRITE mode, the other is not used in parser.
                        if (*other_write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE) {
                            LOG_DEBUG5(TAB1 "Constraint violation: Container "
                                       << c
                                       << " has parser"
                                          " write mode "
                                       << *write_mode << " but " << other
                                       << " has "
                                          "conflicting parser write mode "
                                       << *other_write_mode);
                            return false;
                        }
                    }

                    if (*write_mode != *other_write_mode) {
                        LOG_DEBUG5(TAB1 "Constraint violation: Container "
                                   << c
                                   << " has parser "
                                      "write mode "
                                   << *write_mode << " but " << other
                                   << " has "
                                      "conflicting parser write mode "
                                   << *other_write_mode);
                        return false;
                    }
                }
            }
        }
    }

    // Check 3: all constainers within parser extract group must have same source type
    using ExtractSource = PHV::Allocation::ExtractSource;
    if (Device::phvSpec().hasParserExtractGroups() && isExtracted &&
        parserExtractGroupSource != ExtractSource::NONE) {
        if ((parserExtractGroupSource == ExtractSource::PACKET && !isExtractedFromPkt) ||
            (parserExtractGroupSource == ExtractSource::NON_PACKET && isExtractedFromPkt)) {
            LOG_DEBUG5(TAB1 "Constraint violation: Container "
                       << c << " has parser extract group source " << parserExtractGroupSource
                       << " but slice needs " << (isExtractedFromPkt ? "packet" : "non-packet"));
            return false;
        }
    }

    // Check deparser group gress.
    auto deparserGroupGress = alloc.deparserGroupGress(c);
    bool isDeparsed = utils_i.uses.is_deparsed(f);
    if (isDeparsed && deparserGroupGress && *deparserGroupGress != f->gress) {
        LOG_DEBUG5(TAB1 "Constraint violation: Container " << c << " has deparser group gress "
                                                           << *deparserGroupGress
                                                           << " but slice needs " << f->gress);
        return false;
    }

    // true if @a is the same field allocated right before @b. e.g.
    // B1[0:1] <- f1[0:1], B1[2:7] <- f1[2:7]
    auto is_aligned_same_field_alloc = [](const PHV::AllocSlice &a, const PHV::AllocSlice &b) {
        if (a.field() != b.field() || a.container() != b.container()) {
            return false;
        }
        if (a.container_slice().hi > b.container_slice().hi)
            return a.field_slice().lo - b.field_slice().hi ==
                   a.container_slice().lo - b.container_slice().hi;
        else
            return b.field_slice().lo - a.field_slice().hi ==
                   b.container_slice().lo - a.container_slice().hi;
    };
    // Check no pack for this field.
    const auto &byte_slices = alloc.byteSlicesByLiveness(c, slice, utils_i.pragmas.pa_no_init());
    std::vector<PHV::AllocSlice> liveSlices;
    ordered_set<PHV::AllocSlice> initSlices;

    for (auto &sl : byte_slices) {
        // TODO: aligned fieldslice from the same field can be ignored
        if (is_aligned_same_field_alloc(slice, sl)) {
            continue;
        }
        liveSlices.push_back(sl);
    }

    for (auto &sl : initFields) {
        // TODO: aligned fieldslice from the same field can be ignored
        if (is_aligned_same_field_alloc(slice, sl)) {
            continue;
        }
        initSlices.insert(sl);
    }

    LOG_DEBUG7(TAB2 "liveSlices:");
    for (auto &sl : liveSlices) LOG_DEBUG7(TAB2 "  " << sl);
    LOG_DEBUG7(TAB2 "initSlices:");
    for (auto &sl : initSlices) LOG_DEBUG7(TAB2 "  " << sl);

    for (auto &sl : byte_slices) {
        bool disjoint = slice.isLiveRangeDisjoint(sl) ||
                        (slice.container_slice().overlaps(sl.container_slice()) &&
                         utils_i.phv.isMetadataMutex(slice.field(), sl.field()));

        if ((sl.field()->is_solitary() || slice.field()->is_solitary()) && disjoint) {
            LOG_DEBUG7(TAB1 "Ignoring solitary due to disjoint live range");
            continue;
        } else if (sl.field()->is_solitary() && !disjoint) {
            LOG_DEBUG5(TAB1 "Constraint violation: Field "
                       << sl.field()->name
                       << " has solitary "
                          "constraint and is already placed in this container");
            return false;
        } else if (slice.field()->is_solitary() && !disjoint) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: Slice has solitary constraint, but there are "
                       "other slices in the container.");
            return false;
        }
    }

    // pov bits are parser initialized as well.
    // discount slices that are going to be initialized through metadata initialization from being
    // considered uninitialized reads.

    // TODO: what about checksums? are they extracted?
    // *TODO* Replacing here is_extracted() with is_extracted_from_packet()
    //        causes table placement fitting regressions. Revisit in the future.
    auto is_slice_extracted = [this](const PHV::AllocSlice &s) {
        return !s.field()->pov && utils_i.uses.is_extracted_from_pkt(s.field());
    };
    auto is_slice_uninitialized = [this, &initFields](const PHV::AllocSlice &s) {
        return (s.field()->pov ||
                (utils_i.defuse.hasUninitializedRead(s.field()->id) && !initFields.count(s) &&
                 !utils_i.pragmas.pa_no_init().getFields().count(s.field())));
    };

    bool isThisSliceExtracted = is_slice_extracted(slice);
    bool isThisSliceUninitialized = is_slice_uninitialized(slice);

    LOG_DEBUG7(TAB1 "  slice: " << slice << std::endl
                                << TAB2 "isThisSliceUninitialized:" << isThisSliceUninitialized
                                << std::endl
                                << TAB2 "isThisSliceExtracted:" << isThisSliceExtracted);

    bool hasOtherUninitializedRead, hasOtherExtracted, hasExtractedTogether;

    for (auto slc : liveSlices) {
        hasOtherUninitializedRead = is_slice_uninitialized(slc);
        hasOtherExtracted = is_slice_extracted(slc);
        hasExtractedTogether =
            utils_i.phv.are_bridged_extracted_together(slice.field(), slc.field());

        LOG_DEBUG7(TAB1 "   liveSlice: "
                   << slc << std::endl
                   << TAB2 "hasOtherUninitialized:" << hasOtherUninitializedRead << std::endl
                   << TAB2 "hasOtherExtracted:" << hasOtherExtracted << std::endl
                   << TAB2 "hasExtractedTogether:" << hasExtractedTogether);

        if (hasOtherExtracted && !hasExtractedTogether &&
            (isThisSliceUninitialized || isThisSliceExtracted)) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: Container already contains extracted slices, "
                       "can't be packed because this slice is: "
                       << (isThisSliceExtracted ? "extracted" : "uninitialized"));
            return false;
        }

        // Account for metadata initialization and ensure that initialized fields are not considered
        // uninitialized any more.
        if (isThisSliceExtracted && !hasExtractedTogether &&
            (hasOtherExtracted || hasOtherUninitializedRead)) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: This slice is extracted, can't be packed because "
                       "allocated fields have: "
                       << (hasOtherExtracted ? "extracted" : "uninitialized"));
            return false;
        }
    }

    for (auto cand_sl : candidate_slices) {
        if (slice == cand_sl) break;
        if (slice.container() != cand_sl.container()) continue;
        if (!(Device::currentDevice() == Device::TOFINO) &&
            !slice.container_bytes().overlaps(cand_sl.container_bytes()))
            continue;

        hasOtherUninitializedRead =
            cand_sl.field()->pov || (utils_i.defuse.hasUninitializedRead(cand_sl.field()->id) &&
                                     !cand_sl.is_initialized() && !initSlices.count(cand_sl));
        hasOtherExtracted =
            !cand_sl.field()->pov && utils_i.uses.is_extracted_from_pkt(cand_sl.field());
        hasExtractedTogether =
            utils_i.phv.are_bridged_extracted_together(slice.field(), cand_sl.field()) ||
            slice.field()->header() == cand_sl.field()->header();

        // Update defs of extracted if both slices belong to same field
        if (slice.field()->id == cand_sl.field()->id) {
            isThisSliceExtracted =
                !slice.field()->pov && utils_i.uses.is_extracted_from_pkt(slice.field()) &&
                utils_i.defuse.hasDefInParser(slice.field(), slice.field_slice());
            hasOtherExtracted =
                !cand_sl.field()->pov && utils_i.uses.is_extracted_from_pkt(cand_sl.field()) &&
                utils_i.defuse.hasDefInParser(cand_sl.field(), cand_sl.field_slice());
            hasExtractedTogether = (isThisSliceExtracted && hasOtherExtracted);
        }

        LOG_DEBUG7(TAB1 "  candidate slice: "
                   << cand_sl << std::endl
                   << TAB2 "isThisSliceExtracted:" << isThisSliceExtracted << std::endl
                   << TAB2 "hasOtherUninitialized:" << hasOtherUninitializedRead << std::endl
                   << TAB2 "hasOtherExtracted:" << hasOtherExtracted << std::endl
                   << TAB2 "hasExtractedTogether:" << hasExtractedTogether << std::endl);

        if (hasOtherExtracted && !hasExtractedTogether &&
            (isThisSliceUninitialized || isThisSliceExtracted)) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: Other candidate slice is extracted; "
                       "can't be packed because this slice is: "
                       << (isThisSliceExtracted ? "extracted" : "uninitialized"));
            return false;
        }

        // Account for metadata initialization and ensure that initialized fields are not considered
        // uninitialized any more.
        if (isThisSliceExtracted && !hasExtractedTogether &&
            (hasOtherUninitializedRead || hasOtherExtracted)) {
            LOG_DEBUG5(TAB1
                       "Constraint violation: This slice is extracted, can't be packed because "
                       "other candidate fields are: "
                       << (hasOtherExtracted ? "extracted" : "uninitialized"));
            return false;
        }
    }

    return true;
}

/* static */
bool CoreAllocation::satisfies_constraints(const PHV::ContainerGroup &g,
                                           const PHV::SuperCluster &sc) {
    // Check max individual field width.
    if (int(g.width()) < sc.max_width()) {
        LOG_DEBUG5(TAB1 "Constraint violation: Container size " << g.width()
                                                                << " is too small for "
                                                                   "max field width "
                                                                << sc.max_width());
        return false;
    }

    // Check max slice list width.
    for (auto *slice_list : sc.slice_lists()) {
        int size = 0;
        for (auto &slice : *slice_list) size += slice.size();
        if (int(g.width()) < size) {
            LOG_DEBUG5(TAB1 "Constraint violation: Container size " << g.width()
                                                                    << " is too small "
                                                                       "for slice list width "
                                                                    << size);
            return false;
        }
    }

    // Check container type.
    for (const auto *rot : sc.clusters()) {
        for (const auto *ali : rot->clusters()) {
            auto rst = satisfies_container_type_constraints(g, *ali);
            if (!rst) {
                LOG_DEBUG5(TAB1 "ContainerGroup type can't satisfy cluster constraints");
                return false;
            }
        }
    }

    return true;
}

// Check for overlapping liveranges between slices of non-overlapping bitranges
bool CoreAllocation::hasCrossingLiveranges(std::vector<PHV::AllocSlice> candidate_slices,
                                           ordered_set<PHV::AllocSlice> alloc_slices) const {
    for (auto &cnd_slice : candidate_slices) {
        std::set<int> write_stages;
        if (cnd_slice.getEarliestLiveness().second.isWrite())
            write_stages.insert(cnd_slice.getEarliestLiveness().first);
        if (cnd_slice.getLatestLiveness().second.isWrite())
            write_stages.insert(cnd_slice.getLatestLiveness().first);
        if (write_stages.size() == 0) continue;

        for (auto &alc_slice : alloc_slices) {
            if (alc_slice.field() == cnd_slice.field()) continue;

            if (alc_slice.container() != cnd_slice.container()) continue;

            if (alc_slice.container_slice().overlaps(cnd_slice.container_slice())) continue;

            // Check non bitrange-overlapping slices (missed by phv_action_constraints)
            for (int stg : write_stages) {
                // candidate liverange overlaps with allocated liverage?
                bool lr_overlap = ((stg >= alc_slice.getEarliestLiveness().first) &&
                                   (stg < alc_slice.getLatestLiveness().first)) ||
                                  ((stg == alc_slice.getLatestLiveness().first) &&
                                   alc_slice.getLatestLiveness().second.isWrite());

                if (lr_overlap) {
                    LOG_DEBUG4("Found overlapping liverange between allocated "
                               << alc_slice << " and candidate " << cnd_slice);
                    LOG_DEBUG4(TAB2
                               "Stopped considering mocha container due to overlapping "
                               "liveranges of candidate slices with allocated slices in "
                               "non-overlapping container bitrange");
                    return true;
                }
            }
        }
    }

    return false;
}

// Check dark mutex of all candidates and build bitvec of container
// slice corresponding to dark mutex slices. If the bitvec is not contiguous
// then turn off ARA initializations; instead use regular table inits
bool CoreAllocation::checkDarkOverlay(const std::vector<PHV::AllocSlice> &candidate_slices,
                                      const PHV::Transaction &alloc) const {
    bool canUseARA = true;
    bitvec cntr_bits;

    for (const auto &sl : candidate_slices) {
        const auto &alloced_slices = alloc.slices(sl.container(), sl.container_slice());

        // Find slices that can be dark overlaid
        if (can_overlay(utils_i.phv.dark_mutex(), sl.field(), alloced_slices)) {
            // Disable use of ARA if container contains mutually exclusive slices
            if (some_overlay(utils_i.mutex(), sl.field(), alloced_slices)) {
                canUseARA = false;
                LOG_FEATURE("alloc_progress", 5, " ... cannot use ARA due to mutex overlay!");
                break;
            }

            // TODO: Could add checks for liveranges if initializations
            //              may happen at different stages

            // Update bitvec
            le_bitrange cntr_slice = sl.container_slice();
            cntr_bits.setrange(cntr_slice.lo, cntr_slice.size());
        }
    }

    canUseARA = canUseARA && cntr_bits.is_contiguous();
    return canUseARA;
}

bool CoreAllocation::rangesOverlap(const PHV::AllocSlice &slice,
                                   const IR::BFN::ParserPrimitive *prim) const {
    const IR::Expression *expr = nullptr;
    if (auto *extract = prim->to<IR::BFN::Extract>()) {
        expr = extract->dest->field;
    } else if (auto *csum_verify = prim->to<IR::BFN::ChecksumVerify>()) {
        expr = csum_verify->dest->field;
    }
    if (!expr) return false;

    int lo = 0;
    int hi = slice.field()->size - 1;
    if (auto *expr_slice = expr->to<IR::Slice>()) {
        hi = expr_slice->e1->to<IR::Constant>()->asInt();
        lo = expr_slice->e2->to<IR::Constant>()->asInt();
    }

    auto slice_range = slice.field_slice();
    le_bitrange prim_range(lo, hi);
    return slice_range.overlaps(prim_range);
}

// Check whether the candidate slices produce parser extractions that cause data corruption
bool CoreAllocation::checkParserExtractions(const std::vector<PHV::AllocSlice> &candidate_slices,
                                            const PHV::Transaction &alloc) const {
    if (!candidate_slices.size()) return true;

    // All candidate slices should be to the same container
    PHV::Container c = candidate_slices.front().container();
    const PHV::Allocation::ContainerStatus *status = alloc.getStatus(c);

    auto &field_to_writes = utils_i.field_to_parser_states.field_to_writes;
    auto &write_to_state = utils_i.field_to_parser_states.write_to_state;
    auto &state_to_parser = utils_i.field_to_parser_states.state_to_parser;

    for (auto &slice : candidate_slices) {
        const auto *f = slice.field();
        if (!f) continue;
        if (!field_to_writes.count(f)) continue;

        for (const auto *prim : field_to_writes.at(f)) {
            if (!rangesOverlap(slice, prim)) continue;

            bool prim_is_pkt_extract =
                prim->is<IR::BFN::Extract>() &&
                prim->to<IR::BFN::Extract>()->source->is<IR::BFN::InputBufferRVal>();

            for (auto &other_slice : status->slices) {
                const auto *other_f = other_slice.field();
                if (f == other_f) continue;
                if (!field_to_writes.count(other_f)) continue;
                if (utils_i.mutex()(f->id, other_f->id)) continue;

                for (const auto *other_prim : field_to_writes.at(other_f)) {
                    if (!rangesOverlap(other_slice, other_prim)) continue;

                    bool other_prim_is_pkt_extract =
                        other_prim->is<IR::BFN::Extract>() &&
                        other_prim->to<IR::BFN::Extract>()->source->is<IR::BFN::InputBufferRVal>();

                    if (prim_is_pkt_extract || other_prim_is_pkt_extract) {
                        auto *state = write_to_state.at(prim);
                        auto *other_state = write_to_state.at(other_prim);
                        auto *parser = state_to_parser.at(state);

                        if (state == other_state) continue;

                        if (utils_i.parser_info.graph(parser).is_reachable(state, other_state) ||
                            utils_i.parser_info.graph(parser).is_reachable(other_state, state)) {
                            LOG_DEBUG6(
                                TAB1 "Conflicting extracts for "
                                << slice << " and " << other_slice << std::endl
                                << TAB1 "  state " << state->name << ": " << prim << std::endl
                                << TAB1 "  state " << other_state->name << ": " << other_prim);
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool CoreAllocation::try_pack_slice_list(
    std::vector<PHV::AllocSlice> &candidate_slices, PHV::Transaction &perContainerAlloc,
    PHV::Allocation::LiveRangeShrinkingMap &initActions,
    std::optional<PHV::Allocation::LiveRangeShrinkingMap> &initNodes, const PHV::Container &c,
    const PHV::SuperCluster &super_cluster,
    std::optional<PHV::Allocation::ConditionalConstraints> &action_constraints,
    int &num_bitmasks) const {
    // Maintain a list of conditional constraints that are already a part of a slice list that
    // follows the required alignment. Therefore, we do not need to recursively call
    // tryAllocSliceList() for those slice lists because those slice lists will be allocated at
    // the required alignment later on during the allocation of the supercluster.
    std::set<int> conditionalConstraintsToErase;
    // Check whether the candidate slice allocations respect action-induced constraints.
    CanPackErrorCode canPackErrorCode;

    // Gather the initialization actions for all the fields that are allocated to/are candidates
    // for allocation in this container. All these are summarized in the initActions map.
    for (auto &field_slice : candidate_slices) {
        // Get the initialization actions for all the field slices that are candidates for
        // allocation and in the parent transaction.
        auto initPointsForTransaction = perContainerAlloc.getInitPoints(field_slice);
        if (initPointsForTransaction && initPointsForTransaction->size() > 0)
            initActions[field_slice.field()].insert(initPointsForTransaction->begin(),
                                                    initPointsForTransaction->end());
    }

    // -  Populate actual_container_state with allocated slices that
    //    do not have overlap with any of the candidate_slices
    // - Also update initActions with the actions of the slices in actual_container_state
    PHV::Allocation::MutuallyLiveSlices container_state =
        perContainerAlloc.slicesByLiveness(c, candidate_slices);
    // Actual slices in the container, after accounting for metadata overlay.
    PHV::Allocation::MutuallyLiveSlices actual_container_state;
    for (auto &field_slice : container_state) {
        bool sliceLiveRangeDisjointWithAllCandidates = true;
        auto Overlaps = [&](const PHV::AllocSlice &slice) {
            return slice.container_slice().overlaps(field_slice.container_slice());
        };
        // Check if any of the candidate slices being considered for allocation overlap with the
        // slice already in the container. Even if one of the slices overlaps, it is considered
        // a case of metadata overlay enabled by live range shrinking.
        bool hasOverlay = std::any_of(candidate_slices.begin(), candidate_slices.end(), Overlaps);
        for (auto &candidate_slice : candidate_slices) {
            if (!utils_i.phv.metadata_mutex()(field_slice.field()->id, candidate_slice.field()->id))
                sliceLiveRangeDisjointWithAllCandidates = false;
        }
        // If the current slice overlays with at least one candidate slice AND its live range
        // does not overlap with the candidate slices, we do not consider the existing slice to
        // be part of the live container state.
        const bool notPartOfLiveState = hasOverlay && sliceLiveRangeDisjointWithAllCandidates;
        LOG_DEBUG6(TAB2 "Keep container_state slice " << field_slice
                                                      << " in "
                                                         "actual_container_state:"
                                                      << (notPartOfLiveState ? "NO" : "YES"));
        if (notPartOfLiveState) continue;
        actual_container_state.insert(field_slice);
        // Get initialization actions for all other slices in this container and not overlaying
        // with the candidate fields.
        auto initPointsForTransaction = perContainerAlloc.getInitPoints(field_slice);
        if (initPointsForTransaction && initPointsForTransaction->size() > 0)
            initActions[field_slice.field()].insert(initPointsForTransaction->begin(),
                                                    initPointsForTransaction->end());
    }

    if (initActions.size() > 0)
        LOG_DEBUG5(TAB1 "Printing total initialization map:\n"
                   << utils_i.meta_init.printLiveRangeShrinkingMap(initActions, TAB2));

    if (LOGGING(6)) {
        LOG_DEBUG6(TAB1 "Candidates sent to ActionPhvConstraints:");
        for (auto &slice : candidate_slices) LOG_DEBUG6(TAB2 << slice);
    }

    // can_pack verification.
    std::tie(canPackErrorCode, action_constraints) = utils_i.actions.can_pack(
        perContainerAlloc, candidate_slices, actual_container_state, initActions);

    bool creates_new_container_conflicts = utils_i.actions.creates_container_conflicts(
        actual_container_state, initActions, utils_i.meta_init.getTableActionsMap());
    // If metadata initialization causes container conflicts to be created, then do not use this
    // allocation.
    if (action_constraints && initActions.size() > 0 && creates_new_container_conflicts) {
        LOG_DEBUG5(TAB1
                   "Action constraint violation: creates new container conflicts for "
                   "this packing. Cannot pack into container "
                   << c << canPackErrorCode);

        return false;
    }

    if (!action_constraints) {
        LOG_DEBUG5(TAB1 "Action constraint violation: Cannot pack into container "
                   << c << canPackErrorCode);
        return false;
    } else if (action_constraints->size() > 0) {
        if (LOGGING(5)) {
            LOG_DEBUG5(TAB2 "But only if the following placements are respected:");
            for (auto kv_source : *action_constraints) {
                LOG_DEBUG5(TAB3 "Source " << kv_source.first);
                for (auto kv : kv_source.second) {
                    std::stringstream ss;
                    ss << TAB4 << kv.first << " @ " << kv.second.bitPosition;
                    if (kv.second.container) ss << " and @container " << *(kv.second.container);
                    LOG_DEBUG5(ss.str());
                }
            }
        }

        // Find slice lists that contain slices in action_constraints.
        for (auto kv_source : *action_constraints) {
            std::optional<const PHV::SuperCluster::SliceList *> slice_list = std::nullopt;
            for (auto &slice_and_pos : kv_source.second) {
                const auto &slice_lists = super_cluster.slice_list(slice_and_pos.first);
                if (slice_lists.size() > 1) {
                    // If a slice is in multiple slice lists, abort.
                    // TODO: This is overly constrained.
                    LOG_DEBUG5(TAB1 "Failed: Conditional placement is in multiple slice lists");
                    return false;
                } else if (slice_lists.size() == 0) {
                    // TODO: this seems to be too conservative. We can craft a slice
                    // list to satisfy the condition constraint, as long as the slicelist
                    // is valid.
                    if (slice_list) {
                        LOG_DEBUG5(TAB1 "Failed: Slice "
                                   << slice_and_pos.first
                                   << " is not in "
                                      "a slice list, while other slices in the same conditional "
                                      "constraint is in a slice list.");
                        return false;
                    } else {
                        // not in a slicelist ignored.
                        continue;
                    }
                }

                auto *candidate = slice_lists.front();
                if (slice_list) {
                    auto &fs1 = slice_list.value()->front();
                    auto &fs2 = candidate->front();
                    if (fs1.field()->exact_containers() != fs2.field()->exact_containers()) {
                        LOG_DEBUG5(TAB1
                                   "Failed: Two slice cannot be placed in one container "
                                   "because different exact_containers: "
                                   << fs1 << " " << fs2);
                        return false;
                    }
                    // TODO: Even with above fix, this conditional constraint
                    // slicelist allocation is still wrong. It overwrites previous
                    // slice_list found for one constraint, which does not make
                    // sense here. We need a further fix for this behavior.
                }
                slice_list = candidate;
            }

            // At this point, all conditional placements for this source are in the same slice
            // list. If the alignments check out, we do not need to apply the conditional
            // constraints for this source.
            if (slice_list) {
                // Check that the positions induced by action constraints match
                // the positions in the slice list.  The offset is relative to
                // the beginning of the slice list until the first
                // action-constrained slice is encountered, at which point the
                // offset is set to the required offset.
                LOG_DEBUG5(TAB3 "Found field in another slice list.");
                int offset = 0;
                bool absolute = false;
                int size = 0;
                std::optional<PHV::Container> requiredContainer = std::nullopt;
                std::map<PHV::FieldSlice, int> bitPositions;
                for (auto &slice : **slice_list) {
                    size += slice.range().size();
                    if (kv_source.second.find(slice) == kv_source.second.end()) {
                        bitPositions[slice] = offset;
                        offset += slice.range().size();
                        continue;
                    }

                    int required_pos = kv_source.second.at(slice).bitPosition;
                    if (requiredContainer &&
                        *requiredContainer != kv_source.second.at(slice).container)
                        BUG("Error setting up conditional constraints: Multiple containers "
                            "%1% and %2% found",
                            *requiredContainer, *kv_source.second.at(slice).container);
                    requiredContainer = kv_source.second.at(slice).container;
                    if (!absolute && required_pos < offset) {
                        // This is the first slice with an action alignment constraint.  Check
                        // that the constraint is >= the bits seen so far. If this check fails,
                        // then set can_place to false so that we may try the next container.
                        LOG_DEBUG5(TAB1 "Action constraint violation: "
                                   << slice
                                   << " must be "
                                      "placed at bit "
                                   << required_pos << " but is " << offset << "b deep in a slice "
                                   << "list");
                        return false;
                    } else if (!absolute) {
                        absolute = true;
                        offset = required_pos + slice.range().size();
                    } else if (offset != required_pos) {
                        // If the alignment due to the conditional constraint is not the same as
                        // the alignment inherent in the slice list structure, then this
                        // placement is not possible. So set can_place to false so that we may
                        // try the next container.
                        LOG_DEBUG5(TAB1 "Action constraint violation: "
                                   << slice
                                   << " must be "
                                      "placed at bit "
                                   << required_pos
                                   << " which conflicts with "
                                      "another action-induced constraint for another slice in the"
                                      " slice list");
                        return false;
                    } else {
                        offset += slice.range().size();
                    }
                }

                if (requiredContainer) {
                    for (auto &slice : **slice_list) {
                        if (kv_source.second.find(slice) != kv_source.second.end()) continue;
                        BUG_CHECK(bitPositions.count(slice),
                                  "Cannot calculate offset for slice %1%", slice);
                        (*action_constraints)[kv_source.first][slice] =
                            PHV::Allocation::ConditionalConstraintData(bitPositions.at(slice),
                                                                       *requiredContainer);
                    }
                } else {
                    // If we've reached here, then all the slices that have conditional
                    // constraints are in slice_list at the right required alignment. Therefore,
                    // we can mark this source for erasure from the conditional constraints map.
                    conditionalConstraintsToErase.insert(kv_source.first);
                }
            }
        }
    } else {
        LOG_DEBUG5(TAB1 "No action constraints - can pack into container " << c);
        if (initNodes)
            num_bitmasks =
                utils_i.actions.count_bitmasked_set_instructions(candidate_slices, *initNodes);
        else
            num_bitmasks =
                utils_i.actions.count_bitmasked_set_instructions(candidate_slices, initActions);
    }

    if (conditionalConstraintsToErase.size() > 0) {
        for (auto i : conditionalConstraintsToErase) {
            LOG_DEBUG5(TAB2 "Erasing conditional constraint associated with source #" << i);
            (*action_constraints).erase(i);
        }
    }
    return true;
}

std::optional<std::vector<PHV::AllocSlice>> CoreAllocation::prepare_candidate_slices(
    PHV::SuperCluster::SliceList &slices, const PHV::Container &c,
    const PHV::Allocation::ConditionalConstraint &start_positions) const {
    std::vector<PHV::AllocSlice> candidate_slices;
    for (auto &field_slice : slices) {
        if (c.is(PHV::Kind::mocha) && !field_slice.field()->is_mocha_candidate()) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: " << c << " cannot contain the non-mocha field slice "
                                        << field_slice);
            return std::nullopt;
        }
        if (c.is(PHV::Kind::dark) && !field_slice.field()->is_dark_candidate()) {
            LOG_FEATURE(
                "alloc_progress", 5,
                TAB1 "Failed: " << c << " cannot contain the non-dark field slice " << field_slice);
            return std::nullopt;
        }
        le_bitrange container_slice =
            StartLen(start_positions.at(field_slice).bitPosition, field_slice.size());
        // Field slice has a const Field*, so get the non-const version using the PhvInfo object
        candidate_slices.push_back(PHV::AllocSlice(utils_i.phv.field(field_slice.field()->id), c,
                                                   field_slice.range(), container_slice));
    }
    return candidate_slices;
}

bool CoreAllocation::try_metadata_overlay(
    const PHV::Container &c, std::optional<ordered_set<PHV::AllocSlice>> &allocedSlices,
    const PHV::AllocSlice &slice, std::optional<PHV::Allocation::LiveRangeShrinkingMap> &initNodes,
    ordered_set<PHV::AllocSlice> &new_candidate_slices,
    ordered_set<PHV::AllocSlice> &metaInitSlices,
    PHV::Allocation::LiveRangeShrinkingMap &initActions, PHV::Transaction &perContainerAlloc,
    const PHV::Allocation::MutuallyLiveSlices &alloced_slices,
    PHV::Allocation::MutuallyLiveSlices &actual_cntr_state) const {
    bool prevDeparserZero = std::all_of(
        alloced_slices.begin(), alloced_slices.end(),
        [](const PHV::AllocSlice &a) { return a.field()->is_deparser_zero_candidate(); });
    if (prevDeparserZero && !slice.field()->is_deparser_zero_candidate()) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB1 "Failed: Can't do metadata overlay on deparsed zero container " << c);
        return false;
    }

    LOG_DEBUG5(TAB1 "Can overlay " << slice << " on " << alloced_slices
                                   << " with metadata initialization.");
    initNodes = utils_i.meta_init.findInitializationNodes(alloced_slices, slice, perContainerAlloc,
                                                          actual_cntr_state);
    bool noInitPresent = true;
    if (!initNodes) {
        LOG_FEATURE("alloc_progress", 5, TAB1 "Failed: Can't find meta initialization points.");
        return false;
    } else {
        if (!allocedSlices) {
            allocedSlices = alloced_slices;
        } else {
            allocedSlices->insert(alloced_slices.begin(), alloced_slices.end());
        }

        new_candidate_slices.push_back(slice);
        LOG_DEBUG5(TAB1 "Found the following initialization points:");
        LOG_DEBUG5(utils_i.meta_init.printLiveRangeShrinkingMap(*initNodes, TAB2));
        // For the initialization plan returned, note the fields that would need to
        // be initialized in the MAU.
        for (auto kv : *initNodes) {
            if (kv.second.size() > 0) noInitPresent = false;
            if (!slice.field()->is_padding() && kv.first == slice.field()) {
                LOG_DEBUG5(TAB2 "A. Inserting " << slice << " into metaInitSlices");
                metaInitSlices.insert(slice);

                initActions[kv.first].insert(kv.second.begin(), kv.second.end());
                LOG6("\t\t\tAdding initActions for field: " << kv.first);
                continue;
            }

            for (const auto &sl : alloced_slices) {
                if (!sl.field()->is_padding() && kv.first == sl.field()) {
                    LOG_DEBUG5(TAB2 "B. Inserting " << sl << " into metaInitSlices");
                    metaInitSlices.insert(sl);

                    initActions[kv.first].insert(kv.second.begin(), kv.second.end());
                    LOG6("\t\t\tAdding " << kv.second.size()
                                         << " initActions for field: " << kv.first);
                    continue;
                }
            }
        }
    }

    if (!noInitPresent && disableMetadataInit) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB1
                    "Failed: Live range shrinking requiring metadata "
                    "initialization is disabled in this round");
        return false;
    }
    return true;
}

bool CoreAllocation::try_dark_overlay(std::vector<PHV::AllocSlice> &dark_slices,
                                      PHV::Transaction &perContainerAlloc, const PHV::Container &c,
                                      std::vector<PHV::AllocSlice> &candidate_slices,
                                      ordered_set<PHV::AllocSlice> &new_candidate_slices,
                                      bool &new_overlay_container,
                                      ordered_set<PHV::AllocSlice> &metaInitSlices,
                                      const PHV::ContainerGroup &group,
                                      const bool &canDarkInitUseARA) const {
    for (auto &slice : dark_slices) {
        ordered_set<PHV::AllocSlice> alloced_slices;
        // Get all overlaid slices; For mocha/dark containers get all allocated slice as
        // overlaying is across entire container (i.e. can only write entire container)
        if (c.is(PHV::Kind::mocha) || c.is(PHV::Kind::dark))
            alloced_slices = perContainerAlloc.slices(c);
        else
            alloced_slices = perContainerAlloc.slices(slice.container(), slice.container_slice());
        LOG_FEATURE("alloc_progress", 5,
                    "    Attempting to overlay "
                        << slice.field() << " on " << alloced_slices
                        << " by pushing one of them into a dark container.");

        // Get non parser-mutually-exclusive slices allocated in container c
        PHV::Allocation::MutuallyLiveSlices container_state =
            perContainerAlloc.slicesByLiveness(c, candidate_slices);
        // Actual slices in the container, after accounting for metadata overlay.
        PHV::Allocation::MutuallyLiveSlices actual_cntr_state;
        PHV::Allocation::MutuallyLiveSlices actual_cntr_state2;
        for (auto &field_slice : container_state) {
            LOG_DEBUG5("   Container Slice: " << field_slice);
            for (auto &candidate_slice : candidate_slices) {
                LOG_DEBUG5(TAB2 " A. overlap check for slices " << field_slice << " and "
                                                                << candidate_slice);
                if (!utils_i.phv.metadata_mutex()(field_slice.field()->id,
                                                  candidate_slice.field()->id)) {
                    actual_cntr_state.insert(field_slice);
                    break;
                }
            }

            // Add to container state allocated fields that overlap with existing fields
            // overlaid with candidate fields
            for (auto &alloc_slice : alloced_slices) {
                LOG_DEBUG5(TAB2 " B. overlap check for slices " << field_slice << " and "
                                                                << alloc_slice);
                if (alloc_slice != field_slice &&
                    !utils_i.phv.field_mutex()(field_slice.field()->id, alloc_slice.field()->id) &&
                    !utils_i.phv.metadata_mutex()(field_slice.field()->id,
                                                  alloc_slice.field()->id)) {
                    actual_cntr_state2.insert(field_slice);
                    break;
                }
            }
        }

        for (auto c_sl : actual_cntr_state2) {
            if (!actual_cntr_state.count(c_sl)) {
                LOG_DEBUG5("  Slice " << c_sl << " not found in actual_cntr_state");
                actual_cntr_state.insert(c_sl);
            }
        }

        auto darkInitNodes = utils_i.dark_init.findInitializationNodes(
            group, alloced_slices, slice, perContainerAlloc, canDarkInitUseARA, prioritizeARAinits);
        if (!darkInitNodes) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: Cannot find initialization points for dark containers.");
            return false;
        } else {
            // Create initialization points for the dark container.
            if (!generateNewAllocSlices(slice, alloced_slices, *darkInitNodes, new_candidate_slices,
                                        perContainerAlloc, actual_cntr_state)) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB1 "Failed: New dark primitives incompatible with other primitives "
                                 "in container. Skipping container "
                                << c);
                return false;
            }

            LOG_DEBUG5(TAB1 "Found " << darkInitNodes->size()
                                     << " initialization points for dark containers.");
            unsigned primNum = 0;
            for (auto &prim : *darkInitNodes) {
                LOG_DEBUG5(TAB2 << prim);
                if (primNum++ == 0) continue;
                metaInitSlices.insert(prim.getDestinationSlice());
            }
            new_overlay_container = true;

            // TODO We should  populate InitNodes with darkInitNodes to later
            // properly populate initActions
            // TODO
        }
    }
    return true;
}

bool CoreAllocation::check_metadata_and_dark_overlay(
    const PHV::Container &c, std::vector<PHV::AllocSlice> &complex_overlay_slices,
    std::vector<PHV::AllocSlice> &candidate_slices,
    ordered_set<PHV::AllocSlice> &new_candidate_slices, PHV::Transaction &perContainerAlloc,
    ordered_map<const PHV::AllocSlice, OverlayInfo> &overlay_info,
    std::optional<PHV::Allocation::LiveRangeShrinkingMap> &initNodes,
    std::optional<ordered_set<PHV::AllocSlice>> &allocedSlices,
    ordered_set<PHV::AllocSlice> &metaInitSlices,
    PHV::Allocation::LiveRangeShrinkingMap &initActions, bool &new_overlay_container,
    const PHV::ContainerGroup &group, const bool &canDarkInitUseARA) const {
    /// 3. check metadta init or dark overlay.
    // If there are slices already allocated for these container bits, then check if
    // overlay is enabled by live shrinking is possible. If yes, then note down
    // information about the initialization required and allocated slices for later
    // constraint verification.
    std::vector<PHV::AllocSlice> dark_slices;
    for (const auto &slice : complex_overlay_slices) {
        const auto &alloced_slices =
            perContainerAlloc.slices(slice.container(), slice.container_slice());
        bool metadataOverlay = overlay_info.at(slice).metadata_overlay;
        bool darkOverlay = overlay_info.at(slice).dark_overlay;
        bool is_mocha_or_dark = c.is(PHV::Kind::dark) || c.is(PHV::Kind::mocha);
        // Get non parser-mutually-exclusive slices allocated in container c
        PHV::Allocation::MutuallyLiveSlices container_state =
            perContainerAlloc.slicesByLiveness(c, candidate_slices);
        // Actual slices in the container, after accounting for metadata overlay.
        PHV::Allocation::MutuallyLiveSlices actual_cntr_state;
        for (auto &field_slice : container_state) {
            bool sliceOverlaysAllCandidates = true;
            for (auto &candidate_slice : candidate_slices) {
                if (!utils_i.phv.metadata_mutex()(field_slice.field()->id,
                                                  candidate_slice.field()->id))
                    sliceOverlaysAllCandidates = false;
            }
            if (sliceOverlaysAllCandidates) continue;
            actual_cntr_state.insert(field_slice);
        }
        // Disable metadata initialization if the container for metadata overlay is a mocha
        // or dark container.
        if (!is_mocha_or_dark && metadataOverlay && (!prioritizeARAinits || !darkOverlay)) {
            if (!try_metadata_overlay(c, allocedSlices, slice, initNodes, new_candidate_slices,
                                      metaInitSlices, initActions, perContainerAlloc,
                                      alloced_slices, actual_cntr_state))
                return false;
        } else if (!c.is(PHV::Kind::dark) && darkOverlay) {
            // Process dark overlays after processing all other overlays.
            // Push into a list and process immediately after this loop completes.
            LOG5("    ...and can overlay " << slice << " on " << alloced_slices
                                           << " by pushing one of them into a dark container."
                                              " Will try after placing other candidates...");
            dark_slices.push_back(slice);
        } else {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: " << c << " already contains slices at this position");
            return false;
        }
    }
    return try_dark_overlay(dark_slices, perContainerAlloc, c, candidate_slices,
                            new_candidate_slices, new_overlay_container, metaInitSlices, group,
                            canDarkInitUseARA);
}

// Compare properties of existing primitive and new primitive and
// check if some of them need to be maintained in new slice.
bool CoreAllocation::update_new_prim(const PHV::AllocSlice &existingSl, PHV::AllocSlice &newSl,
                                     ordered_set<PHV::AllocSlice> &toBeRemovedFromAlloc) const {
    LOG_DEBUG5("\t\t existing Prim:" << existingSl.getInitPrimitive());
    LOG_DEBUG5("\t\t      new Prim:" << newSl.getInitPrimitive());

    // if existing slice does not have a primitive  then there is nothing to do
    if (existingSl.getInitPrimitive().isEmpty()) {
        LOG_DEBUG5(" Existing slice has empty primitive. No further update checks required.");
        return true;
    }

    auto &existingPrim = existingSl.getInitPrimitive();
    auto &newPrim = newSl.getInitPrimitive();

    // Do not do overlay if slice is already used for overlay with incompatible ARA semantics
    if ((existingPrim.getInitPoints().size() && newPrim.isAlwaysRunActionPrim()) ||
        (existingPrim.isAlwaysRunActionPrim() && newPrim.getInitPoints().size())) {
        LOG_FEATURE("alloc_progress", 5, " Slice can not be both ARA and use regular table!");
        return false;
    }

    // Do overlay but no need to change @newSl if @existingSl has a different source
    if (((existingPrim.getSourceSlice() != nullptr) && newPrim.destAssignedToZero()) ||
        (existingPrim.destAssignedToZero() && (newPrim.getSourceSlice() != nullptr))) {
        LOG_DEBUG5(" Incompatible comparison between move and zero init prims");
        return true;
    }

    // Check if existing prim has properties that need to be maintained in new prim
    if (existingPrim.isNOP()) {
        LOG_DEBUG5(" Existing slice is a NOP. No further update checks required.");
        return true;
    }

    // Keep early liverange if new prim is NOP
    if (newPrim.isNOP()) {
        if (existingSl.getEarliestLiveness().first < newSl.getEarliestLiveness().first) {
            LOG_DEBUG5(" New slice is NOP. Keeping existing slice early liverange.");
            toBeRemovedFromAlloc.insert(newSl);
            newSl.setEarliestLiveness(existingSl.getEarliestLiveness());
        }
    }

    // If existing and new primitive use different regular tables then
    // do not proceed with new overlay
    if (existingPrim.getInitPoints().size()) {
        for (const auto act : existingPrim.getInitPoints()) {
            if (!newPrim.getInitPoints().count(act)) {
                LOG_DEBUG5(
                    " Existing slice and new slice use different regular tables for "
                    "initizalization; Dumping new overlay.");
                return false;
            }
        }
    }

    if (existingPrim.destAssignedToZero()) {
        newPrim.setAssignZeroToDest();
        newPrim.addPriorUnits(existingPrim.getARApriorUnits());
        if (existingSl.getEarliestLiveness().first == newSl.getEarliestLiveness().first) {
            for (auto *prim : existingPrim.getARApriorPrims()) newPrim.addPriorPrims(prim);
        }

        if (existingPrim.isAlwaysRunActionPrim()) newPrim.setAlwaysRunActionPrim();
        LOG_DEBUG5(" Maintaining zero-init, ARA, prior units/prims from existing prim in slice: "
                   << newSl);
    }
    return true;
}

bool CoreAllocation::try_place_wide_arith_hi(const PHV::ContainerGroup &group,
                                             const PHV::Container &c,
                                             PHV::SuperCluster::SliceList *hi_slice,
                                             const PHV::SuperCluster &super_cluster,
                                             PHV::Transaction &this_alloc,
                                             const ScoreContext &score_ctx) const {
    std::vector<PHV::AllocSlice> hi_candidate_slices;
    bool can_alloc_hi = false;
    for (const auto &next_container : group) {
        if ((c.index() + 1) != next_container.index()) {
            continue;
        }
        LOG_DEBUG5("Checking adjacent container " << next_container);
        const std::vector<PHV::Container> one_container = {next_container};
        // so confusing, parameter size here means bit width,
        // but group.size() returns the number of containers.
        auto small_grp = PHV::ContainerGroup(group.width(), one_container);

        auto hi_alignments = build_slicelist_alignment(small_grp, super_cluster, hi_slice);
        if (hi_alignments.empty()) {
            LOG_DEBUG6("Couldn't build hi alignments");
            can_alloc_hi = false;
            break;
        }

        for (const auto &alloc_align : hi_alignments) {
            auto try_hi = tryAllocSliceList(this_alloc, small_grp, super_cluster, *hi_slice,
                                            alloc_align.slice_alignment, score_ctx);
            if (try_hi != std::nullopt) {
                LOG_DEBUG5(TAB1 "Wide arith hi slice could be allocated in " << next_container);
                LOG_DEBUG5(TAB1 << hi_slice);
                can_alloc_hi = true;
                // this allocation for high has to be commited together with low as they cannot be
                // separated, otherwise we risk allocation of high will fail later
                this_alloc.commit(*try_hi);
                break;
            }
        }
        break;
    }
    if (!can_alloc_hi) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB1 "Failed: Wide arithmetic hi slice could not be allocated.");
        return false;
    } else {
        LOG_FEATURE("alloc_progress", 5, TAB1 "Wide arithmetic hi slice could be allocated.");
        return true;
    }
}

bool CoreAllocation::find_previous_allocation(
    PHV::Container &previous_container,
    ordered_map<PHV::FieldSlice, PHV::AllocSlice> &previous_allocations,
    const PHV::Allocation::ConditionalConstraint &start_positions,
    PHV::SuperCluster::SliceList &slices, const PHV::ContainerGroup &group,
    const PHV::Allocation &alloc) const {
    // Set previous_container to the container provided as part of start_positions, if any.
    LOG_FEATURE("alloc_progress", 5, "\nTrying to allocate slices at container indices:");
    for (auto &slice : slices) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB1 << start_positions.at(slice).bitPosition << ": " << slice);
        if (start_positions.at(slice).container)
            LOG_FEATURE(
                "alloc_progress", 5,
                TAB2 "(Required container: " << *(start_positions.at(slice).container) << ")");
        if (start_positions.at(slice).container) {
            PHV::Container slice_prev_cont = *(start_positions.at(slice).container);
            if (previous_container == PHV::Container()) {
                previous_container = slice_prev_cont;
            } else if (previous_container != slice_prev_cont) {
                LOG_FEATURE("alloc_progress", 5,
                            "mixed previous containers for one slice list: "
                                << previous_container << " and " << slice_prev_cont);
                return false;
            }
        }
    }

    // Check if any of these slices have already been allocated.  If so, record
    // where.  Because we have already finely sliced each field, we can check
    // slices for equivalence rather than containment.

    LOG_DEBUG5("\nChecking if any of the slices hasn't been allocated already");
    for (auto &slice : slices) {
        // TODO: Looking up existing allocations is expensive in the
        // current implementation.  Consider refactoring.
        auto alloc_slices = alloc.slices(slice.field(), slice.range());
        BUG_CHECK(alloc_slices.size() <= 1, "Fine slicing failed");
        if (alloc_slices.size() == 0) continue;
        auto alloc_slice = *alloc_slices.begin();

        // Check if previous allocations were to a container in this group.
        if (!group.contains(alloc_slice.container())) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: Slice "
                            << slice
                            << " has already been allocated to a different container group");
            return false;
        }

        // Check if all previous allocations were to the same container.
        if (previous_container != PHV::Container() &&
            previous_container != alloc_slice.container()) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1
                        "Failed: Some slices in this list have already been allocated to "
                        "different containers");
            return false;
        }
        previous_container = alloc_slice.container();

        // Check that previous allocations match the proposed bit positions in this allocation.
        if (alloc_slice.container_slice().lo != start_positions.at(slice).bitPosition) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: " << alloc_slice
                                        << " has already been allocated and does not start at "
                                        << start_positions.at(slice).bitPosition);
            return false;
        }

        // Record previous allocations for use later.
        previous_allocations.emplace(slice, alloc_slice);
    }
    return true;
}

std::optional<PHV::Transaction> CoreAllocation::tryAllocSliceList(
    const PHV::Allocation &alloc, const PHV::ContainerGroup &group,
    const PHV::SuperCluster &super_cluster, const PHV::SuperCluster::SliceList &slice_list,
    const ordered_map<PHV::FieldSlice, int> &start_positions, const ScoreContext &score_ctx) const {
    PHV::Allocation::ConditionalConstraint start_pos;
    for (auto fs : slice_list) {
        start_pos[fs] = PHV::Allocation::ConditionalConstraintData(start_positions.at(fs));
    }
    return tryAllocSliceList(alloc, group, super_cluster, start_pos, score_ctx);
}

// FIELDSLICE LIST <--> CONTAINER GROUP allocation.
// This function generally is used under two cases:
// 1. Allocating the slice list of a super_cluster.
// 2. Allocating a single field.
// For the both cases, @p start_positions are valid starting positions of slices.
// Additionally, start_positions also now represents conditional constraints that are generated by
// ActionPhvConstraints.
// The sub-problem here, is to find the best container for this SliceList that
// 1. It is valid.
// 2. Try to maximize overlays. (in terms of the number of overlays).
// 3. If same n_overlay, try to maximize packing,
//    in terms of choosing the container with least free room).
std::optional<PHV::Transaction> CoreAllocation::tryAllocSliceList(
    const PHV::Allocation &alloc, const PHV::ContainerGroup &group,
    const PHV::SuperCluster &super_cluster,
    const PHV::Allocation::ConditionalConstraint &start_positions,
    const ScoreContext &score_ctx) const {
    // Collect up the field slices to be allocated.
    PHV::SuperCluster::SliceList slices;
    for (auto &kv : start_positions) slices.push_back(kv.first);

    PHV::Transaction alloc_attempt = alloc.makeTransaction();
    const int container_size = int(group.width());
    PHV::ContainerEquivalenceTracker cet(alloc);

    // Set previous_container to the container provided as part of start_positions, if any.
    PHV::Container previous_container;

    // Check if any of these slices have already been allocated.  If so, record
    // where.  Because we have already finely sliced each field, we can check
    // slices for equivalence rather than containment.
    ordered_map<PHV::FieldSlice, PHV::AllocSlice> previous_allocations;

    if (!find_previous_allocation(previous_container, previous_allocations, start_positions, slices,
                                  group, alloc)) {
        return std::nullopt;
    }
    // If we have already allocated some fields of our slices to previous_container, we need to
    // consider it non-equivalent to all other containers. The same holds if the container is
    // explicitly asked for by some of start_positions (which is also reflected to
    // previous_container by find_previous_allocation)
    cet.exclude(previous_container);
    LOG_FEATURE("alloc_progress", 5, TAB1 "Got previous allocations: " << previous_allocations);

    // Check FIELD<-->GROUP constraints for each field.
    LOG_DEBUG5("Checking constraints for each field");
    for (auto &slice : slices) {
        if (!satisfies_constraints(group, slice)) {
            LOG_FEATURE(
                "alloc_progress", 5,
                TAB1 "Failed: Slice " << slice << " doesn't satisfy slice<-->group constraints");
            return std::nullopt;
        }
    }

    // figure out whether we have a wide arithmetic lo operation and find the
    // associated hi field slice.
    LOG_DEBUG7("Check if slices can fit the container and find wide arithmetic operations");
    bool wide_arith_lo = false;
    PHV::SuperCluster::SliceList *hi_slice = nullptr;
    for (const auto &slice : slices) {
        if (slice.field()->bit_used_in_wide_arith(slice.range().lo)) {
            cet.set_is_wide_arith();
            if (slice.field()->bit_is_wide_arith_lo(slice.range().lo)) {
                wide_arith_lo = true;
                cet.set_is_wide_arith_low();
                hi_slice = super_cluster.findLinkedWideArithSliceList(&slices);
                BUG_CHECK(hi_slice,
                          "Unable to find hi field slice associated with"
                          " wide arithmetic operation in the cluster %1%",
                          super_cluster);
                LOG_DEBUG7(TAB1 "Slice " << slice
                                         << " seems to be wide arith lo with linked "
                                            "slice list: "
                                         << hi_slice);
            }
        }
    }  // end for (auto& slice : slices)

    // Compute the aggregate size required for the slices before comparing against the container
    // size.
    int aggregate_size = 0;
    // sum of slices size
    for (const auto &slice : slices) {
        aggregate_size += slice.size();
    }
    // Return if the slices can't fit together in a container.

    if (container_size < aggregate_size) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB1 "Failed: Slices are "
                        << aggregate_size << "b in total and cannot fit in a " << container_size
                        << "b container");
        return std::nullopt;
    }

    // Look for a container to allocate all slices in.
    AllocScore best_score = AllocScore::make_lowest();
    std::optional<PHV::Transaction> best_candidate = std::nullopt;
    for (const PHV::Container &c : group) {
        LOG_FEATURE("alloc_progress", 5, "Trying to allocate to " << c);
        if (auto equivalent_c = cet.find_equivalent_tried_container(c)) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Container " << c
                                          << " is indistinguishible "
                                             "from an already tried container "
                                          << *equivalent_c << ", skipping");
            continue;
        }

        // If any slices were already allocated, ensure that unallocated slices
        // are allocated to the same container.
        if (previous_container != PHV::Container() && previous_container != c) {
            LOG_FEATURE(
                "alloc_progress", 5,
                TAB1 "Failed: Some slices were already allocated to " << previous_container);
            continue;
        }

        // Generate candidate_slices if we choose this container.
        std::vector<PHV::AllocSlice> candidate_slices;
        if (auto res = prepare_candidate_slices(slices, c, start_positions)) {
            candidate_slices = *res;
        } else {
            continue;
        }

        // Check slice list<-->container constraints.
        if (!satisfies_constraints(candidate_slices, alloc_attempt)) continue;

        // Check that there's space.
        // Results of metadata initialization. This is a map of field to the initialization actions
        // determined by FindInitializationNode methods.
        ordered_set<PHV::AllocSlice> new_candidate_slices;
        std::optional<PHV::Allocation::LiveRangeShrinkingMap> initNodes = std::nullopt;
        std::optional<ordered_set<PHV::AllocSlice>> allocedSlices = std::nullopt;
        // The metadata slices that require initialization after live range shrinking.
        ordered_set<PHV::AllocSlice> metaInitSlices;
        PHV::Transaction perContainerAlloc = alloc_attempt.makeTransaction();
        // Flag case that overlay requires new container such as dark overlay
        bool new_overlay_container = false;
        bool canDarkInitUseARA = !utils_i.settings.no_code_change &&
                                 checkDarkOverlay(candidate_slices, perContainerAlloc);
        PHV::Allocation::LiveRangeShrinkingMap initActions;
        // Check that the placement can be done through metadata initialization.
        // Process dark overlays only after processing the parser/metadata overlays to ensure that
        // the validation sees all other slices that might be allocated to the container.
        std::vector<PHV::AllocSlice> dark_slices;
        std::vector<PHV::AllocSlice> complex_overlay_slices;

        ordered_map<const PHV::AllocSlice, OverlayInfo> overlay_info;
        for (const auto &slice : candidate_slices) {
            if (!utils_i.uses.is_referenced(slice.field()) && !slice.field()->isGhostField())
                continue;
            // Skip slices that have already been allocated.
            if (previous_allocations.find(PHV::FieldSlice(slice.field(), slice.field_slice())) !=
                previous_allocations.end())
                continue;
            const auto &alloced_slices =
                perContainerAlloc.slices(slice.container(), slice.container_slice());

            // For mutually exclusive overlay take into account use of
            // ARA initialized slices -- ARA initializations break mutex property
            const bool control_flow_overlay =
                can_overlay(utils_i.mutex(), slice.field(), alloced_slices) &&
                !hasARAinits(alloced_slices);
            const bool physical_liverange_overlay =
                utils_i.settings.physical_liverange_overlay &&
                can_physical_liverange_overlay(slice, alloced_slices);
            const bool metadataOverlay =
                !utils_i.settings.no_code_change &&
                can_overlay(utils_i.phv.metadata_mutex(), slice.field(), alloced_slices);
            const bool darkOverlay =
                !utils_i.settings.no_code_change &&
                can_overlay(utils_i.phv.dark_mutex(), slice.field(), alloced_slices);
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Testing overlay of " << slice.field() << " on " << alloced_slices);
            LOG_FEATURE("alloc_progress", 5, TAB2 "Control flow overlay: " << control_flow_overlay);
            LOG_FEATURE("alloc_progress", 5, TAB2 "Metadata overlay: " << metadataOverlay);
            LOG_FEATURE("alloc_progress", 5, TAB2 "Dark overlay: " << darkOverlay);
            overlay_info[slice] = {control_flow_overlay, physical_liverange_overlay,
                                   metadataOverlay, darkOverlay};
            /// 1. no overlapped slice.
            if (alloced_slices.empty()) {
                new_candidate_slices.insert(slice);
                continue;
            }
            /// 2. directly overlaid.
            if (control_flow_overlay || physical_liverange_overlay) {
                LOG_DEBUG5(TAB1 "Can overlay " << slice << " on " << alloced_slices);
                new_candidate_slices.insert(slice);
                continue;
            }
            complex_overlay_slices.push_back(slice);
        }
        if (!check_metadata_and_dark_overlay(c, complex_overlay_slices, candidate_slices,
                                             new_candidate_slices, perContainerAlloc, overlay_info,
                                             initNodes, allocedSlices, metaInitSlices, initActions,
                                             new_overlay_container, group, canDarkInitUseARA))
            continue;

        if ((new_candidate_slices.size() > 0) ||
            (new_overlay_container && (metaInitSlices.size() > 0))) {
            candidate_slices.clear();
            for (auto sl : new_candidate_slices) candidate_slices.push_back(sl);
        }

        if (LOGGING(5) && metaInitSlices.size() > 0) {
            LOG_DEBUG5(TAB1 "Printing all fields for whom metadata has been initialized");
            for (const auto &sl : metaInitSlices) LOG_DEBUG5(TAB2 << sl);
        }

        if (LOGGING(6)) {
            LOG_DEBUG6(TAB1 "Candidate slices:");
            for (auto &slice : candidate_slices) LOG_DEBUG6(TAB2 << slice);
            LOG_DEBUG6(TAB1 "Existing slices in container: " << c);
            for (auto &slice : perContainerAlloc.slices(c)) LOG_DEBUG6(TAB2 << slice);
        }

        // Check that all parser extractions are compatible
        if (!checkParserExtractions(candidate_slices, perContainerAlloc)) continue;

        // Check that we don't get parser extract group source conflict between slices (existing
        // and to-be-allocated)
        using ExtractSource = PHV::Allocation::ExtractSource;
        if (Device::phvSpec().hasParserExtractGroups()) {
            ExtractSource source = ExtractSource::NONE;  // shared over all lambda invocations
            auto check = [&source, this](const auto &slices) {
                for (auto &slice : slices) {
                    if (utils_i.uses.is_extracted(slice.field())) {
                        auto sliceSource = utils_i.uses.is_extracted_from_pkt(slice.field())
                                               ? ExtractSource::PACKET
                                               : ExtractSource::NON_PACKET;
                        if (source == ExtractSource::NONE) source = sliceSource;
                        if (source != sliceSource) {
                            LOG_DEBUG5(TAB1 "Not possible because slice "
                                       << slice << " has extract source " << sliceSource
                                       << " while previous slice(s) has " << source);
                            return false;
                        }
                    }
                }
                return true;
            };
            // set the source from the slices already in the container
            bool consistent = check(perContainerAlloc.slices(c));
            BUG_CHECK(consistent,
                      "There is inconsistent parser extract group source in already allocated "
                      "slices in %1%",
                      c);
            // and check it is consistent with the new slices
            // TODO: check the new slices are consistent among themselves before trying to
            // allocate them
            if (!check(candidate_slices)) continue;
        }  // if (Device::phvSpec().hasParserExtractGroups())

        // check pa_container_type constraints for candidates after dark overlay.
        bool pa_container_type_ok = true;
        for (const auto &sl : candidate_slices) {
            auto req_kind = utils_i.pragmas.pa_container_type().required_kind(sl.field());
            if (req_kind && sl.container().type().kind() != *req_kind) {
                LOG_DEBUG5(TAB1 "Not possible because @pa_container_type specify that "
                           << sl.field() << " must be allocated to " << *req_kind << ", so "
                           << sl.container() << " is not allowed");
                pa_container_type_ok = false;
                break;
            }
        }
        if (!pa_container_type_ok) {
            continue;
        }

        // Add special handling for dark overlays of Mocha containers
        //        - If the overlaying candidate slice liverange overlaps
        //          with the liverange of other existing slices in the container
        //          then do skip the dark overlay
        if (new_overlay_container && c.is(PHV::Kind::mocha)) {
            if (hasCrossingLiveranges(candidate_slices, perContainerAlloc.slices(c))) continue;
        }

        // Check that each field slice satisfies slice<-->container
        // constraints, skipping slices that have already been allocated.
        bool constraints_ok = std::all_of(
            candidate_slices.begin(), candidate_slices.end(), [&](const PHV::AllocSlice &slice) {
                PHV::FieldSlice fs(slice.field(), slice.field_slice());
                bool alloced =
                    previous_allocations.find(PHV::FieldSlice(fs)) != previous_allocations.end();
                return alloced ? true
                               : satisfies_constraints(perContainerAlloc, slice, metaInitSlices,
                                                       candidate_slices);
            });
        if (!constraints_ok) {
            initNodes = std::nullopt;
            allocedSlices = std::nullopt;
            continue;
        }
        std::optional<PHV::Allocation::ConditionalConstraints> action_constraints;
        int num_bitmasks = 0;
        if (!try_pack_slice_list(candidate_slices, perContainerAlloc, initActions, initNodes, c,
                                 super_cluster, action_constraints, num_bitmasks)) {
            continue;
        }

        // Create this alloc for calculating score.
        // auto this_alloc = alloc_attempt.makeTransaction();
        auto this_alloc = perContainerAlloc.makeTransaction();
        for (auto &slice : candidate_slices) {
            bool is_referenced = utils_i.uses.is_referenced(slice.field());
            bool is_allocated =
                previous_allocations.find(PHV::FieldSlice(slice.field(), slice.field_slice())) !=
                previous_allocations.end();
            if ((is_referenced || slice.field()->isGhostField()) && !is_allocated) {
                if (initActions.size() && initActions.count(slice.field())) {
                    // For overlay enabled by live range shrinking, we also need to store
                    // initialization information as a member of the allocated slice.
                    this_alloc.allocate(slice, &initActions, singleGressParserGroups);
                    LOG5("\t\tFound initialization point for metadata field "
                         << slice.field()->name);
                } else {
                    this_alloc.allocate(slice, nullptr, singleGressParserGroups);
                }
            } else if (!is_referenced && !slice.field()->isGhostField()) {
                LOG_FEATURE("alloc_progress", 5, "NOT ALLOCATING unreferenced field: " << slice);
            }
        }

        // Add metadata initialization information for previous allocated slices. As more and more
        // slices get allocated, the initialization actions for already allocated slices in the
        // container may change. So, update the initialization information in these already
        // allocated slices based on the latest initialization plan.
        if (initActions.size() && allocedSlices) {
            for (auto &slice : *allocedSlices) {
                if (initActions.count(slice.field())) {
                    LOG5("        Initialization noted corresponding to already allocated slice: "
                         << slice);
                    this_alloc.addMetadataInitialization(slice, initActions);
                }
            }
        }

        // Recursively try to allocate slices according to conditional
        // constraints induced by actions.  NB: By allocating the current slice
        // list first, we guarantee that recursion terminates, because each
        // invocation allocates fields or fails before recursion, and there are
        // finitely many fields.
        bool conditional_constraints_satisfied = true;
        for (auto kv : *action_constraints) {
            if (kv.second.empty()) continue;
            // If we actually call allocation recursively we cannot use cache due to possibility
            // of different recursive allocations resulting in different scores even if the
            // containers at this level are equivalent
            LOG_FEATURE("alloc_progress", 9, TAB2 "Invalidating equivalence for " << c);
            cet.invalidate(c);
            auto action_alloc =
                tryAllocSliceList(this_alloc, group, super_cluster, kv.second, score_ctx);
            if (!action_alloc) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB1
                            "Failed: Slice list has conditional constraints that cannot be "
                            "satisfied");
                conditional_constraints_satisfied = false;
                break;
            }
            LOG_DEBUG5(TAB1 "Conditional constraints are satisfied");
            this_alloc.commit(*action_alloc);
        }

        if (!conditional_constraints_satisfied) continue;

        // If this is a wide arithmetic lo operation, make sure
        // the most significant slices can be allocated in the directly
        // adjacent container -- either the container is free or can overlay.
        // Do this here, after lo slice has been committed to transaction.
        if (wide_arith_lo) {
            LOG_FEATURE("alloc_progress", 9,
                        TAB1 "If wide arithmetic low is allocated to "
                            << c << " the high " << hi_slice << " needs to be placed next:");
            if (!try_place_wide_arith_hi(group, c, hi_slice, super_cluster, this_alloc, score_ctx))
                continue;
        }
        perContainerAlloc.commit(this_alloc);

        auto score =
            score_ctx.make_score(perContainerAlloc, utils_i.phv, utils_i.clot, utils_i.uses,
                                 utils_i.field_to_parser_states, utils_i.parser_critical_path,
                                 utils_i.tablePackOpt, num_bitmasks);
        LOG_FEATURE("alloc_progress", 5,
                    TAB1 "SLICE LIST score for container " << c << ": " << score);

        // update the best
        if ((!best_candidate || score_ctx.is_better(score, best_score))) {
            LOG_FEATURE("alloc_progress", 5, TAB2 "Best score for container " << c);
            best_score = score;
            best_candidate = std::move(perContainerAlloc);
            if (score_ctx.stop_at_first()) {
                break;
            }
        }
    }  // end of for containers

    if (!best_candidate) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB2 "Failed: There is no suitable candidate for slices" << slices);
        return std::nullopt;
    }

    alloc_attempt.commit(*best_candidate);
    return alloc_attempt;
}

// Used for dark overlays - Replaces original slice allocation with
// new slice allocation and spilled slice allocation in dark and normal
// ---
bool CoreAllocation::generateNewAllocSlices(
    const PHV::AllocSlice &origSlice, const ordered_set<PHV::AllocSlice> &alloced_slices,
    PHV::DarkInitMap &slices, ordered_set<PHV::AllocSlice> &new_candidate_slices,
    PHV::Transaction &alloc_attempt,
    const PHV::Allocation::MutuallyLiveSlices &container_state) const {
    std::vector<PHV::AllocSlice> initializedAllocSlices;
    for (auto &entry : slices) {  // Iterate over the initialization of the new field and the dark
                                  // allocated field (spill + write-back)
        LOG_DEBUG5(TAB2 "DarkInitEntry: " << entry);
        auto dest = entry.getDestinationSlice();
        LOG_DEBUG5(TAB2 "Adding destination: " << dest);
        dest.setInitPrimitive(&(entry.getInitPrimitive()));

        // Also update the lifetime of prior/post prims related to the source slice
        for (auto *prim : dest.getInitPrimitive().getARApostPrims()) {
            bool sameEarly =
                (dest.getEarliestLiveness() == prim->getDestinationSlice().getEarliestLiveness());
            bool sameLate =
                (dest.getLatestLiveness() == prim->getDestinationSlice().getLatestLiveness());
            BUG_CHECK(sameEarly || sameLate, "A.None of the liverange bounds is the same ...?");

            if (sameEarly && !sameLate) {
                LOG_DEBUG4(TAB2 "A.Updating latest liveness for: " << prim->getDestinationSlice());
                LOG_DEBUG4(TAB3 "to " << dest.getLatestLiveness());
                prim->setDestinationLatestLiveness(dest.getLatestLiveness());
            }

            if (!sameEarly && sameLate) {
                LOG_DEBUG4(
                    TAB2 "A.Updating earliest liveness for: " << prim->getDestinationSlice());
                LOG_DEBUG4(TAB3 "to " << dest.getEarliestLiveness());
                prim->setDestinationEarliestLiveness(dest.getEarliestLiveness());
            }
        }

        for (auto *prim : dest.getInitPrimitive().getARApriorPrims()) {
            bool sameEarly =
                (dest.getEarliestLiveness() == prim->getDestinationSlice().getEarliestLiveness());
            bool sameLate =
                (dest.getLatestLiveness() == prim->getDestinationSlice().getLatestLiveness());
            BUG_CHECK(sameEarly || sameLate, "B.None of the liverange bounds is the same ...?");

            if (sameEarly && !sameLate) {
                LOG_DEBUG4(TAB2 "B.Updating latest liveness for: " << prim->getDestinationSlice());
                LOG_DEBUG4(TAB3 "to " << dest.getLatestLiveness());
                prim->setDestinationLatestLiveness(dest.getLatestLiveness());
            }

            if (!sameEarly && sameLate) {
                LOG_DEBUG4(
                    TAB2 "B.Updating earliest liveness for: " << prim->getDestinationSlice());
                LOG_DEBUG4(TAB3 "to " << dest.getEarliestLiveness());
                prim->setDestinationEarliestLiveness(dest.getEarliestLiveness());
            }
        }

        initializedAllocSlices.push_back(dest);
    }
    std::vector<PHV::AllocSlice> rv;
    LOG_DEBUG5(TAB2 "Original candidate slice: " << origSlice);
    bool foundAnyNewSliceForThisOrigSlice = false;
    PHV::Container dstCntr = origSlice.container();
    ordered_set<PHV::AllocSlice> new_container_state;
    new_container_state.insert(container_state.begin(), container_state.end());
    for (auto sl : new_candidate_slices)
        if (sl.container() == dstCntr) new_container_state.insert(sl);
    LOG_DEBUG5("  New Container State:");
    for (auto sl : new_container_state) LOG_DEBUG5(TAB1 << sl);

    // Create mapping from sources of writes to bitranges for each stage
    std::map<int, std::map<PHV::Container, bitvec>> perStageSources2Ranges;
    for (auto &newSlice : initializedAllocSlices) {
        LOG_DEBUG6(TAB2 " newSlice: " << newSlice);
        // Init stage of newSlice
        int initStg = newSlice.getEarliestLiveness().first;
        // srcCntr is not set for zeroInits and NOPs
        PHV::Container srcCntr = PHV::Container();

        if (newSlice.getInitPrimitive().getSourceSlice())
            srcCntr = newSlice.getInitPrimitive().getSourceSlice()->container();

        le_bitrange cBits = newSlice.container_slice();

        if (newSlice.container() == dstCntr && !newSlice.getInitPrimitive().isNOP()) {
            perStageSources2Ranges[initStg][srcCntr].setrange(cBits.lo, cBits.size());

            if (srcCntr == PHV::Container()) {
                LOG_DEBUG6(TAB2 "  Adding bits " << cBits << " for stage " << initStg
                                                 << " from zero init");
            } else {
                LOG_DEBUG6(TAB2 "  Adding bits " << cBits << " for stage " << initStg
                                                 << " from source container " << srcCntr);
            }
        }

        if (!origSlice.representsSameFieldSlice(newSlice))
            LOG_DEBUG5(TAB2 "Found new slice: " << newSlice);

        if (new_container_state.size() > 1) {
            for (auto mls : new_container_state) {
                LOG_DEBUG6(TAB3 "mlsSlice: " << mls);
                le_bitrange mlsBits = mls.container_slice();

                // If mls is overlaid with origSlice then skip mls
                // because this is the slice prior to overlay
                if (mlsBits.intersectWith(cBits).size() > 0) continue;

                // This is counting the number of source at the time newSlice is initialized,
                // so we only care about the start of newSlice liverange
                // overlapping with the liveranges of other allocated slices.
                bool disjoint_lr =
                    PHV::LiveRange(newSlice.getEarliestLiveness(), newSlice.getEarliestLiveness())
                        .is_disjoint(
                            PHV::LiveRange(mls.getEarliestLiveness(), mls.getLatestLiveness()));
                if (disjoint_lr) continue;

                PHV::Container mlsCntr = PHV::Container();
                bool hasPrim = mls.hasInitPrimitive();
                LOG_DEBUG5(TAB2 "Checking slice " << mls
                                                  << " for common init stages (has dark "
                                                     "primitive :"
                                                  << hasPrim << ")");
                if (hasPrim && mls.getInitPrimitive().getSourceSlice()) {
                    LOG_DEBUG6(TAB3 "with source " << mls.getInitPrimitive().getSourceSlice());
                } else if (hasPrim) {
                    LOG_DEBUG6(TAB3 "isNop: " << mls.getInitPrimitive().isNOP() << " zeroInit: "
                                              << mls.getInitPrimitive().destAssignedToZero());
                } else {
                    LOG_DEBUG6(TAB3 "empty: " << mls.getInitPrimitive().isEmpty());
                }

                // Account for bits from source container
                if (hasPrim && mls.getEarliestLiveness().second.isWrite() &&
                    newSlice.getEarliestLiveness().second.isWrite() &&
                    (mls.getEarliestLiveness().first == initStg)) {
                    if (mls.getInitPrimitive().getSourceSlice()) {
                        mlsCntr = mls.getInitPrimitive().getSourceSlice()->container();
                        LOG_DEBUG6(TAB2 "A. mls container: " << mlsCntr);
                        LOG_DEBUG6(TAB3 "Primitive: " << mls.getInitPrimitive());
                    }
                    // Else account for previously allocated alive bits in destination container
                } else {
                    mlsCntr = mls.container();
                    LOG_DEBUG6(TAB2 "B. mls container: " << mlsCntr);
                }

                if (!(hasPrim && mls.getInitPrimitive().isNOP())) {
                    // Update per stage sources unless dark prim is NOP
                    perStageSources2Ranges[initStg][mlsCntr].setrange(mlsBits.lo, mlsBits.size());
                    if (mlsCntr == PHV::Container()) {
                        LOG_DEBUG6(TAB2 "Adding bits " << mlsBits << " for stage " << initStg
                                                       << " from zero init");
                    } else {
                        LOG_DEBUG6(TAB2 "Adding bits " << mlsBits << " for stage " << initStg
                                                       << " from container " << mlsCntr);
                    }
                }

                if (perStageSources2Ranges[initStg].size() > 2) {
                    LOG_FEATURE("alloc_progress", 5,
                                TAB2 "Failed: Too many sources: "
                                    << perStageSources2Ranges[initStg].size());
                    return false;
                }
            }
        }

        // Verify the number of sources in initialization actions
        PHV::ActionSet initActions = newSlice.getInitPoints();
        initActions.insert(newSlice.getInitPrimitive().getInitPoints().begin(),
                           newSlice.getInitPrimitive().getInitPoints().end());

        for (auto *act : initActions) {
            auto action_sources =
                utils_i.actions.getActionSources(act, dstCntr, new_candidate_slices, alloc_attempt);
            std::set<PHV::Container> sources;
            for (auto s2r : perStageSources2Ranges[initStg]) {
                if (s2r.first == dstCntr) {
                    bitvec b(s2r.second);
                    b -= action_sources.dest_bits;
                    if (b.empty()) continue;
                }
                sources.emplace(s2r.first);
            }
            for (auto c : action_sources.phv) sources.emplace(c);
            int srcCnt =
                sources.size() + (action_sources.has_ad || action_sources.has_const ? 1 : 0);

            if (srcCnt > 2) {
                LOG_FEATURE(
                    "alloc_progress", 5,
                    TAB2 "Failed: Too many sources in action " << act->name << ": " << srcCnt);
                return false;
            }
        }

        if (newSlice.extends_live_range(origSlice)) {
            LOG_DEBUG5(TAB2 "New dark primitive " << newSlice << " extend original slice liverange"
                                                  << origSlice << ";  thus skipping container "
                                                  << newSlice.container());
            return false;
        }

        foundAnyNewSliceForThisOrigSlice = true;
        rv.push_back(newSlice);
    }
    // Check for contiguity in init slices from the same source
    // container (AlwaysRun actions can not use bitmask-set)
    for (auto stgEntry : perStageSources2Ranges) {
        if (stgEntry.second.size() == 1) continue;

        for (auto srcEntry : stgEntry.second) {
            if (!srcEntry.second.is_contiguous()) {
                std::stringstream cntrSS;
                if (srcEntry.first == PHV::Container())
                    cntrSS << "zeroInit";
                else
                    cntrSS << srcEntry.first;

                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Failed: Non contiguous bits from source "
                                << cntrSS.str() << " (" << srcEntry.second << ") in stage "
                                << stgEntry.first);
                return false;
            }
        }
    }

    if (!foundAnyNewSliceForThisOrigSlice) rv.push_back(origSlice);
    for (auto &slice : rv) new_candidate_slices.insert(slice);
    LOG_DEBUG5(TAB2 "New candidate slices:");
    for (auto &slice : new_candidate_slices) LOG_DEBUG5(TAB3 << slice);
    ordered_set<PHV::AllocSlice> toBeRemovedFromAlloc;
    ordered_set<PHV::AllocSlice> toBeAddedToAlloc;
    for (auto &alreadyAllocatedSlice : alloced_slices) {
        LOG_DEBUG5(TAB2 "Already allocated slice: " << alreadyAllocatedSlice);
        bool foundAnyNewSliceForThisAllocatedSlice = false;
        for (auto &newSlice : initializedAllocSlices) {
            if (!alreadyAllocatedSlice.representsSameFieldSlice(newSlice)) continue;
            LOG_DEBUG5(TAB3 "Found new slice: " << newSlice);
            if (newSlice.extends_live_range(alreadyAllocatedSlice)) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB3 "New dark primitive "
                                << newSlice << " extends existing slice liverange"
                                << alreadyAllocatedSlice << ";  thus skipping container "
                                << newSlice.container());
                return false;
            }
            foundAnyNewSliceForThisAllocatedSlice = true;

            // Copy prior units from removed to new slice
            if (!update_new_prim(alreadyAllocatedSlice, newSlice, toBeRemovedFromAlloc)) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB3 "New dark primitive " << newSlice
                                                       << " not compatible with existing slice "
                                                       << alreadyAllocatedSlice);
                return false;
            }

            toBeRemovedFromAlloc.insert(alreadyAllocatedSlice);
            toBeAddedToAlloc.insert(newSlice);
        }
        if (!foundAnyNewSliceForThisAllocatedSlice)
            LOG_DEBUG5(TAB4 "Did not find any new slice. So stick with the existing one.");
    }

    // Check if new_candidate_slices contains slice that are to be removed
    for (auto rmv_sl : toBeRemovedFromAlloc) {
        if (new_candidate_slices.count(rmv_sl)) {
            new_candidate_slices.erase(rmv_sl);
            LOG_DEBUG5(TAB3 "Removed " << rmv_sl << " from new_candidate_slices");
        }
    }
    alloc_attempt.removeAllocatedSlice(toBeRemovedFromAlloc);
    for (auto &slice : toBeAddedToAlloc) {
        alloc_attempt.allocate(slice, nullptr, singleGressParserGroups);
        LOG_DEBUG5(TAB2 "Allocating slice " << slice);
    }
    PHV::Container c = origSlice.container();
    LOG_DEBUG5(TAB1 "State of allocation object now:");
    for (auto &slice : alloc_attempt.slices(c)) LOG_DEBUG5(TAB2 "Slice: " << slice);
    return true;
}

std::vector<AllocAlignment> CoreAllocation::build_alignments(
    int max_n, const PHV::ContainerGroup &container_group,
    const PHV::SuperCluster &super_cluster) const {
    // collect all possible alignments for each slice_list
    std::vector<std::vector<AllocAlignment>> all_alignments;
    for (const PHV::SuperCluster::SliceList *slice_list : super_cluster.slice_lists()) {
        auto curr_sl_alignment =
            build_slicelist_alignment(container_group, super_cluster, slice_list);
        if (curr_sl_alignment.size() == 0) {
            LOG_DEBUG5("Cannot build alignment for " << slice_list);
            break;
        }
        all_alignments.push_back(curr_sl_alignment);
    }
    // not all slice list has valid alignment, simply skip.
    if (all_alignments.size() < super_cluster.slice_lists().size()) {
        return {};
    }

    // find max_n alignments which is a 'intersection' of one allocAlignemnt for
    // each slice list that is not conflict with others.
    std::vector<AllocAlignment> rst;
    std::function<void(int depth, AllocAlignment &curr)> dfs = [&](int depth,
                                                                   AllocAlignment &curr) -> void {
        if (depth == int(all_alignments.size())) {
            rst.push_back(curr);
            return;
        }
        for (const auto &align_choice : all_alignments[depth]) {
            auto next = alloc_alignment_merge(curr, align_choice);
            if (next) {
                dfs(depth + 1, *next);
            }
            if (int(rst.size()) == max_n) {
                return;
            }
        }
    };
    AllocAlignment emptyAlignment;
    dfs(0, emptyAlignment);
    return rst;
}

std::vector<AllocAlignment> CoreAllocation::build_slicelist_alignment(
    const PHV::ContainerGroup &container_group, const PHV::SuperCluster &super_cluster,
    const PHV::SuperCluster::SliceList *slice_list) const {
    std::vector<AllocAlignment> rst;
    auto valid_list_starts = super_cluster.aligned_cluster(slice_list->front())
                                 .validContainerStart(container_group.width());
    for (const int le_offset_start : valid_list_starts) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB1 "Trying slicelist alignment at offset " << le_offset_start);
        int le_offset = le_offset_start;
        AllocAlignment curr;
        bool success = true;
        for (auto &slice : *slice_list) {
            const PHV::AlignedCluster &cluster = super_cluster.aligned_cluster(slice);
            auto valid_start_options = cluster.validContainerStart(container_group.width());

            // if the slice 's cluster cannot be placed at the current offset.
            if (!valid_start_options.getbit(le_offset)) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Failed: Slice list requires slice to start at "
                                << le_offset << " which its cluster cannot support: " << slice
                                << " with list starts with " << le_offset_start);
                success = false;
                break;
            }

            // Return if the slice is part of another slice list but was previously
            // placed at a different start location.
            // TODO: We may need to be smarter about coordinating all
            // valid starting ranges for all slice lists.
            if (curr.cluster_alignment.count(&cluster) &&
                curr.cluster_alignment.at(&cluster) != le_offset) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Failed: Two slice lists have conflicting alignment requirements "
                                 "for field slice "
                                << slice);
                success = false;
                break;
            }

            // Otherwise, update the alignment for this slice's cluster.
            curr.cluster_alignment[&cluster] = le_offset;
            curr.slice_alignment[slice] = le_offset;
            le_offset += slice.size();
        }
        if (success) {
            LOG_DEBUG6("found one valid slicelist alignment: " << curr.slice_alignment);
            rst.push_back(curr);
        }
    }
    return rst;
}

std::optional<PHV::Transaction> CoreAllocation::alloc_super_cluster_with_alignment(
    const PHV::Allocation &alloc, const PHV::ContainerGroup &container_group,
    PHV::SuperCluster &super_cluster, const AllocAlignment &alignment,
    const std::list<const PHV::SuperCluster::SliceList *> &sorted_slice_lists,
    const ScoreContext &score_ctx) const {
    // Make a new transaction.
    PHV::Transaction alloc_attempt = alloc.makeTransaction();

    ordered_set<PHV::FieldSlice> allocated;
    for (const PHV::SuperCluster::SliceList *slice_list : sorted_slice_lists) {
        // Try allocating the slice list.
        auto partial_alloc_result =
            tryAllocSliceList(alloc_attempt, container_group, super_cluster, *slice_list,
                              alignment.slice_alignment, score_ctx);
        if (!partial_alloc_result) {
            LOG_FEATURE("alloc_progress", 5, "Failed to allocate slice list " << *slice_list);
            return std::nullopt;
        }
        alloc_attempt.commit(*partial_alloc_result);

        // Track allocated slices in order to skip them when allocating their clusters.
        for (auto &slice : *slice_list) {
            allocated.insert(slice);
            LOG_FEATURE("alloc_progress", 5, "Add to allocated: " << slice);
        }
    }

    // After allocating each slice list, use the alignment for each slice in
    // each list to place its cluster.
    for (auto *rotational_cluster : super_cluster.clusters()) {
        for (auto *aligned_cluster : rotational_cluster->clusters()) {
            // Sort all field slices in an aligned cluster based on the
            // number of times they are written to or read from in different actions
            std::vector<PHV::FieldSlice> slice_list;
            for (const PHV::FieldSlice &slice : aligned_cluster->slices()) {
                slice_list.push_back(slice);
            }
            utils_i.actions.sort(slice_list);

            // Forall fields in an aligned cluster, they must share a same start position.
            // Compute possible starts.
            bitvec starts;
            if (alignment.cluster_alignment.count(aligned_cluster)) {
                starts = bitvec(alignment.cluster_alignment.at(aligned_cluster), 1);
            } else {
                auto optStarts = aligned_cluster->validContainerStart(container_group.width());
                if (optStarts.empty()) {
                    // Other constraints satisfied, but alignment constraints
                    // cannot be satisfied.
                    LOG_FEATURE(
                        "alloc_progress", 6,
                        TAB1 "Failed: Alignment constraint violation: No valid start positions "
                             "for aligned cluster "
                            << slice_list);
                    return std::nullopt;
                }
                // Constraints satisfied so long as aligned_cluster is placed
                // starting at a bit position in `starts`.
                starts = optStarts;
            }

            // Compute all possible alignments
            std::optional<PHV::Transaction> best_alloc = std::nullopt;
            AllocScore best_score = AllocScore::make_lowest();
            for (auto start : starts) {
                bool failed = false;
                auto this_alloc = alloc_attempt.makeTransaction();
                // Try allocating all fields at this alignment.
                for (const PHV::FieldSlice &slice : slice_list) {
                    // Skip fields that have already been allocated above.
                    if (allocated.find(slice) != allocated.end()) continue;
                    ordered_map<PHV::FieldSlice, int> start_map = {{slice, start}};
                    auto partial_alloc_result = tryAllocSliceList(
                        this_alloc, container_group, super_cluster,
                        PHV::SuperCluster::SliceList{slice}, start_map, score_ctx);
                    if (partial_alloc_result) {
                        this_alloc.commit(*partial_alloc_result);
                        LOG_FEATURE("alloc_progress", 5, TAB1 "Allocated rotational " << slice);
                    } else {
                        failed = true;
                        LOG_FEATURE("alloc_progress", 5,
                                    TAB1 "Failed to allocate rotational " << slice);
                        break;
                    }
                }  // for slices

                if (failed) continue;
                auto score =
                    score_ctx.make_score(this_alloc, utils_i.phv, utils_i.clot, utils_i.uses,
                                         utils_i.field_to_parser_states,
                                         utils_i.parser_critical_path, utils_i.tablePackOpt);
                if (!best_alloc || score_ctx.is_better(score, best_score)) {
                    best_alloc = std::move(this_alloc);
                    best_score = score;
                    if (score_ctx.stop_at_first()) {
                        break;
                    }
                }
            }

            if (!best_alloc) {
                LOG_FEATURE("alloc_progress", 5, TAB1 "Failed to allocate rotational cluster");
                return std::nullopt;
            }
            alloc_attempt.commit(*best_alloc);
            LOG_FEATURE("alloc_progress", 5, TAB1 "Allocated rotational cluster!");
        }
    }
    return alloc_attempt;
}

std::optional<const PHV::SuperCluster::SliceList *>
CoreAllocation::find_first_unallocated_slicelist(
    const PHV::Allocation &alloc, const std::list<PHV::ContainerGroup *> &container_groups,
    const PHV::SuperCluster &sc) const {
    ScoreContext score_ctx("dummy"_cs, true,
                           [](const AllocScore &, const AllocScore &) { return false; });
    ordered_set<const PHV::SuperCluster::SliceList *> never_allocated;
    for (const PHV::SuperCluster::SliceList *slice_list : sc.slice_lists()) {
        never_allocated.insert(slice_list);
    }
    // Sort slice lists according to the number of times they
    // have been written to and read from in various actions.
    // This helps simplify constraints by placing destinations before sources
    std::list<const PHV::SuperCluster::SliceList *> slice_lists(sc.slice_lists().begin(),
                                                                sc.slice_lists().end());
    utils_i.actions.sort(slice_lists);

    for (const auto &container_group : container_groups) {
        // Check container group/cluster group constraints.
        if (!satisfies_constraints(*container_group, sc)) continue;
        std::vector<AllocAlignment> alignments = build_alignments(1, *container_group, sc);
        if (alignments.empty()) {
            continue;
        }
        const auto &alignment = alignments[0];

        PHV::Transaction alloc_attempt = alloc.makeTransaction();
        for (const PHV::SuperCluster::SliceList *slice_list : slice_lists) {
            // Try allocating the slice list.
            auto partial_alloc_result =
                tryAllocSliceList(alloc_attempt, *container_group, sc, *slice_list,
                                  alignment.slice_alignment, score_ctx);
            if (!partial_alloc_result) {
                break;
            } else {
                never_allocated.erase(slice_list);
                alloc_attempt.commit(*partial_alloc_result);
            }
        }
    }
    if (never_allocated.size() > 0) {
        // must return the first unallocatable sl in the action-sorted order.
        for (const auto &sl : slice_lists) {
            if (never_allocated.count(sl)) {
                return sl;
            }
        }
    }
    return std::nullopt;
}

// SUPERCLUSTER <--> CONTAINER GROUP allocation.
std::optional<PHV::Transaction> CoreAllocation::try_alloc(
    const PHV::Allocation &alloc, const PHV::ContainerGroup &container_group,
    PHV::SuperCluster &super_cluster, int max_alignment_tries,
    const ScoreContext &score_ctx) const {
    if (PHV::AllocUtils::is_clot_allocated(utils_i.clot, super_cluster)) {
        LOG_DEBUG5("Skipping CLOT-allocated super cluster: " << super_cluster);
        return alloc.makeTransaction();
    }

    // Check container group/cluster group constraints.
    if (!satisfies_constraints(container_group, super_cluster)) return std::nullopt;

    LOG_DEBUG5("\nBuild alignments");
    std::vector<AllocAlignment> alignments =
        build_alignments(max_alignment_tries, container_group, super_cluster);
    LOG_DEBUG5("Found " << alignments.size() << " valid alignments");
    if (alignments.empty()) {
        LOG_FEATURE("alloc_progress", 5,
                    "Failed: SuperCluster is not allocable due to alignment:\n"
                        << super_cluster);
        return std::nullopt;
    }

    // Sort slice lists according to the number of times they
    // have been written to and read from in various actions.
    // This helps simplify constraints by placing destinations before sources
    std::list<const PHV::SuperCluster::SliceList *> sorted_slice_lists(
        super_cluster.slice_lists().begin(), super_cluster.slice_lists().end());
    utils_i.actions.sort(sorted_slice_lists);
    LOG_FEATURE("alloc_progress", 5, "Sorted SliceList:");
    for (auto *s_list : sorted_slice_lists) {
        for (auto fsl : *s_list) {
            LOG_FEATURE("alloc_progress", 5, TAB1 << fsl);
        }
    }

    // try different alignments.
    std::optional<PHV::Transaction> best_alloc = std::nullopt;
    AllocScore best_score = AllocScore::make_lowest();
    for (const auto &alignment : alignments) {
        LOG_FEATURE("alloc_progress", 6,
                    "Try allocate with " << str_supercluster_alignments(super_cluster, alignment));
        auto this_alloc = alloc_super_cluster_with_alignment(
            alloc, container_group, super_cluster, alignment, sorted_slice_lists, score_ctx);
        if (this_alloc) {
            auto score = score_ctx.make_score(*this_alloc, utils_i.phv, utils_i.clot, utils_i.uses,
                                              utils_i.field_to_parser_states,
                                              utils_i.parser_critical_path, utils_i.tablePackOpt);
            if (!best_alloc || score_ctx.is_better(score, best_score)) {
                best_alloc = std::move(this_alloc);
                best_score = score;
                LOG_FEATURE("alloc_progress", 5, "Allocated Sorted SliceList with this alignment");
                // TODO: currently we stop at the first valid alignmnt
                // and avoid search too much which might slow down compilation.
                break;
            }
        } else {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: Cannot allocate sorted SliceList with this alignment");
        }
    }
    return best_alloc;
}

/* static */
std::list<PHV::ContainerGroup *> PHV::AllocUtils::make_device_container_groups() {
    const PhvSpec &phvSpec = Device::phvSpec();
    std::list<PHV::ContainerGroup *> rv;

    // Build MAU groups
    for (const PHV::Size s : phvSpec.containerSizes()) {
        for (auto group : phvSpec.mauGroups(s)) {
            // Get type of group
            if (group.empty()) continue;
            // Create group.
            rv.emplace_back(new PHV::ContainerGroup(s, group));
        }
    }

    // Build TPHV collections
    for (auto collection : phvSpec.tagalongCollections()) {
        // Each PHV_MAU_Group holds containers of the same size.  Hence, TPHV
        // collections are split into three groups, by size.
        ordered_map<PHV::Type, bitvec> groups_by_type;

        // Add containers to groups by size
        for (auto cid : collection) {
            auto type = phvSpec.idToContainer(cid).type();
            groups_by_type[type].setbit(cid);
        }

        for (auto kv : groups_by_type)
            rv.emplace_back(new PHV::ContainerGroup(kv.first.size(), kv.second));
    }

    return rv;
}

void PHV::AllocUtils::clear_slices(PhvInfo &phv) {
    phv.clear_container_to_fields();
    for (auto &f : phv) f.clear_alloc();
}

void PHV::AllocUtils::bind_slices(const PHV::ConcreteAllocation &alloc, PhvInfo &phv) {
    for (auto container_and_slices : alloc) {
        // TODO: Do we need to ensure that the live ranges of the constituent slices, put together,
        // completely cover the entire pipeline (plus parser and deparser).

        for (PHV::AllocSlice slice : container_and_slices.second.slices) {
            PHV::Field *f = const_cast<PHV::Field *>(slice.field());
            auto init_points = alloc.getInitPoints(slice);
            static const PHV::ActionSet emptySet;
            slice.setInitPoints(init_points ? *init_points : emptySet);
            if (init_points) {
                slice.setMetaInit();
                LOG_DEBUG5(TAB1 "Adding " << init_points->size()
                                          << " initialization points for "
                                             "field slice: "
                                          << slice.field()->name << " " << slice);
                // for (const auto* a : *init_points)
                //     LOG_DEBUG4(TAB2 "Action: " << a->name);
            }
            f->add_alloc(slice);
            phv.add_container_to_field_entry(slice.container(), f);
        }
    }
}

bool PHV::AllocUtils::update_refs(PHV::AllocSlice &slc, const PhvInfo &p,
                                  const FieldDefUse::LocPairSet &refs, PHV::FieldUse fuse) {
    bool updated = false;
    LOG_DEBUG5(" Updating table " << (fuse.isWrite() ? "defs" : "uses") << " for " << slc);

    for (auto ref : refs) {
        if (ref.first->is<IR::BFN::Parser>() || ref.first->is<IR::BFN::ParserState>() ||
            ref.first->is<IR::BFN::GhostParser>()) {
            LOG_DEBUG5("  Parser ref : not recording");
            continue;
        } else if (ref.first->is<IR::BFN::Deparser>()) {
            LOG_DEBUG5("  Deparser ref : not recorded");
        } else if (ref.first->is<IR::MAU::Table>()) {
            auto *tbl = ref.first->to<IR::MAU::Table>();
            le_bitrange bits;
            p.field(ref.second, &bits);
            if (bits.overlaps(slc.field_slice())) {
                updated |= slc.addRef(tbl->name, fuse);
            }
        } else {
            BUG("Found a reference %s in unit %s that is not the parser, deparser, or table",
                ref.second->toString(), ref.first->toString());
        }
    }

    return updated;
}

void PHV::AllocUtils::update_slice_refs(PhvInfo &phv, const FieldDefUse &defuse) {
    for (auto &f : phv) {
        // Find which field slices have multiple AllocSlices (i.e. are
        // dark overlaid and thus have updated refs)
        // *NOTE*: We don't want to update those because update_refs
        //         uses field-granularity defs/uses rather than field-slice granularity.
        std::map<le_bitrange, int> num_slices_per_range;
        for (auto slc : f.get_alloc()) {
            if (!num_slices_per_range.count(slc.field_slice())) {
                num_slices_per_range[slc.field_slice()] = 1;
            } else {
                num_slices_per_range[slc.field_slice()]++;
            }
        }

        for (auto &slc : f.get_alloc()) {
            if (!slc.getRefs().size()) {
                LOG_DEBUG5("  0-ref slice: " << slc);
            }

            // Do not update AllocSlices that have common bitranges
            // (dark overlaid slices)
            if (num_slices_per_range[slc.field_slice()] > 1) {
                LOG_DEBUG5("\t ... not updating refs for slice " << slc);
                continue;
            }

            bool updated = PHV::AllocUtils::update_refs(
                slc, phv, defuse.getAllDefs(slc.field()->id), PHV::FieldUse(PHV::FieldUse::WRITE));

            updated |= PHV::AllocUtils::update_refs(slc, phv, defuse.getAllUses(slc.field()->id),
                                                    PHV::FieldUse(PHV::FieldUse::READ));
            LOG_DEBUG5("  Slice " << slc << " got its refs updated : " << updated);
        }
    }
}

std::list<PHV::SuperCluster *> PHV::AllocUtils::create_strided_clusters(
    const CollectStridedHeaders &strided_headers,
    const std::list<PHV::SuperCluster *> &cluster_groups) {
    std::list<PHV::SuperCluster *> rst;
    for (auto *sc : cluster_groups) {
        const ordered_set<const PHV::Field *> *strided_group = nullptr;
        auto slices = sc->slices();
        for (const auto &sl : slices) {
            strided_group = strided_headers.get_strided_group(sl.field());
            if (!strided_group) continue;
            if (slices.size() != 1) {
                error(
                    "Field %1% requires strided allocation"
                    " but has conflicting constraints on it.",
                    sl.field()->name);
            }
            break;
        }
        if (!strided_group) {
            rst.push_back(sc);
            continue;
        }
        bool merged = false;
        for (auto *r : rst) {
            auto r_slices = r->slices();
            for (const auto &sl : r_slices) {
                if (!strided_group->count(sl.field())) continue;
                if (r_slices.front().range() == slices.front().range()) {
                    rst.remove(r);
                    auto m = r->merge(sc);
                    m->needsStridedAlloc(true);
                    rst.push_back(m);
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
        if (!merged) rst.push_back(sc);
    }

    return rst;
}

void merge_slices(safe_vector<PHV::AllocSlice> &slices,
                  safe_vector<PHV::AllocSlice> &merged_alloc) {
    std::optional<PHV::AllocSlice> last = std::nullopt;
    for (auto &slice : slices) {
        if (last == std::nullopt) {
            last = slice;
            continue;
        }
        if (last->container() == slice.container() &&
            last->field_slice().lo == slice.field_slice().hi + 1 &&
            last->container_slice().lo == slice.container_slice().hi + 1 &&
            last->getEarliestLiveness() == slice.getEarliestLiveness() &&
            last->getLatestLiveness() == slice.getLatestLiveness() &&
            last->getInitPrimitive() == slice.getInitPrimitive()) {
            int new_width = last->width() + slice.width();
            PHV::ActionSet new_init_points;
            if (last->hasMetaInit())
                new_init_points.insert(last->getInitPoints().begin(), last->getInitPoints().end());
            if (slice.hasMetaInit())
                new_init_points.insert(slice.getInitPoints().begin(), slice.getInitPoints().end());
            if (new_init_points.size() > 0)
                LOG_DEBUG5("Merged slice contains " << new_init_points.size()
                                                    << " initialization points.");
            PHV::AllocSlice new_slice(slice.field(), slice.container(), slice.field_slice().lo,
                                      slice.container_slice().lo, new_width, new_init_points);
            new_slice.setLiveness(slice.getEarliestLiveness(), slice.getLatestLiveness());
            new_slice.setIsPhysicalStageBased(slice.isPhysicalStageBased());
            if (last->hasMetaInit() || slice.hasMetaInit()) new_slice.setMetaInit();
            new_slice.setInitPrimitive(&slice.getInitPrimitive());

            new_slice.addRefs(slice.getRefs());
            for (auto unitAcc : last->getRefs()) {
                bool newUnit = new_slice.addRef(unitAcc.first, unitAcc.second);
                if (newUnit)
                    LOG_DEBUG4("\t\tMerged slices:\n\t\t\t" << *last << "  and\n\t\t\t" << slice
                                                            << "\n\t\t do not have the same units");
            }

            BUG_CHECK(new_slice.field_slice().contains(last->field_slice()),
                      "Merged alloc slice %1% does not contain hi slice %2%",
                      cstring::to_cstring(new_slice), cstring::to_cstring(*last));
            BUG_CHECK(new_slice.field_slice().contains(slice.field_slice()),
                      "Merged alloc slice %1% does not contain lo slice %2%",
                      cstring::to_cstring(new_slice), cstring::to_cstring(slice));
            LOG_DEBUG4("Merging " << last->field() << ": " << *last << " and " << slice << " into "
                                  << new_slice);
            last = new_slice;
        } else {
            merged_alloc.push_back(*last);
            last = slice;
        }
    }
    if (last) merged_alloc.push_back(*last);
}

void PHV::AllocUtils::sort_and_merge_alloc_slices(PhvInfo &phv) {
    // later passes assume that phv alloc info is sorted in field bit order,
    // msb first

    for (auto &f : phv) f.sort_alloc();
    // Merge adjacent field slices that have been allocated adjacently in the
    // same container.  This can happen when the field is involved in a set
    // instruction with another field that has been split---it needs to be
    // "split" to match the invariants on rotational clusters, but in practice
    // to the two slices remain adjacent.
    for (auto &f : phv) {
        safe_vector<PHV::AllocSlice> merged_alloc;
        // In table first phv allocation, field slice live range should be taken into consideration.
        // Therefore, after allocslice is sorted, all alloc slices need to be clustered by live
        // range. And then merge allocslices based on each cluster.
        ordered_map<PHV::LiveRange, safe_vector<PHV::AllocSlice>> liverange_classified;
        if (BFNContext::get().options().alt_phv_alloc == true) {
            for (auto &slice : f.get_alloc()) {
                liverange_classified[{slice.getEarliestLiveness(), slice.getLatestLiveness()}]
                    .push_back(slice);
            }
            for (auto &slices : Values(liverange_classified)) {
                merge_slices(slices, merged_alloc);
            }
        } else {
            merge_slices(f.get_alloc(), merged_alloc);
        }
        f.set_alloc(merged_alloc);
        f.sort_alloc();
    }
}

/// Log (at LOGGING(3)) the device-specific PHV resources.
static void log_device_stats() {
    if (!LOGGING(3)) return;

    const PhvSpec &phvSpec = Device::phvSpec();
    int numContainers = 0;
    int numIngress = 0;
    int numEgress = 0;
    for (auto id : phvSpec.physicalContainers()) {
        numContainers++;
        if (phvSpec.ingressOnly().getbit(id)) numIngress++;
        if (phvSpec.egressOnly().getbit(id)) numEgress++;
    }
    LOG_DEBUG3("There are " << numContainers << " containers.");
    LOG_DEBUG3("Ingress only: " << numIngress);
    LOG_DEBUG3("Egress  only: " << numEgress);
}

AllocResult AllocatePHV::brute_force_alloc(
    PHV::ConcreteAllocation &alloc, PHV::ConcreteAllocation &empty_alloc,
    std::vector<const PHV::SuperCluster::SliceList *> &unallocatable_lists,
    std::list<PHV::SuperCluster *> &cluster_groups,
    const std::list<PHV::ContainerGroup *> &container_groups, const int pipe_id) const {
    BruteForceStrategyConfig default_config{/*.name:*/ "default_alloc_config"_cs,
                                            /*.is_better:*/ default_alloc_score_is_better,
                                            /*.max_failure_retry:*/ 0,
                                            /*.max_slicing:*/ 128,
                                            /*.max_sl_alignment_try:*/ 1,
                                            /*.unsupported_devices:*/ std::nullopt,
                                            /*.pre_slicing_validation:*/ false,
                                            /*.enable_ara_in_overlays:*/ PhvInfo::darkSpillARA};
    BruteForceStrategyConfig less_fragment_backup_config{
        /*.name:*/ "less_fragment_alloc_config"_cs,
        /*.is_better:*/ less_fragment_alloc_score_is_better,
        /*.max_failure_retry:*/ 0,
        /*.max_slicing:*/ 256,
        /*.max_sl_alignment_try:*/ 5,
        /*.unsupported_devices:*/ std::nullopt,
        /*.pre_slicing_validation:*/ false,
        /*.enable_ara_in_overlays:*/ PhvInfo::darkSpillARA};
    std::vector<BruteForceStrategyConfig> configs = {
        default_config,
        less_fragment_backup_config,
    };
    if (!utils_i.settings.no_code_change && PhvInfo::darkSpillARA &&
        Device::currentDevice() != Device::TOFINO) {
        BruteForceStrategyConfig no_ara_config{
            /*.name:*/ "disable_ara_alloc_config"_cs,
            /*.is_better:*/ default_alloc_score_is_better,
            /*.max_failure_retry:*/ 0,
            /*.max_slicing:*/ 128,
            /*.max_sl_alignment_try:*/ 1,
            /*.unsupported_devices:*/ std::unordered_set<Device::Device_t>{Device::TOFINO},
            /*.pre_slicing_validation:*/ false,
            /*.enable_ara_in_overlays:*/ false};
        configs.push_back(no_ara_config);
    }

    AllocResult result(AllocResultCode::UNKNOWN, alloc.makeTransaction(), {});
    for (const auto &config : configs) {
        if (config.unsupported_devices &&
            config.unsupported_devices->count(Device::currentDevice())) {
            continue;
        }
        PhvInfo::darkSpillARA = PhvInfo::darkSpillARA && config.enable_ara_in_overlays;

        BruteForceAllocationStrategy *strategy = new BruteForceAllocationStrategy(
            config.name, utils_i, core_alloc_i, empty_alloc, config, pipe_id, phv_i);
        result = strategy->tryAllocation(alloc, cluster_groups, container_groups);
        if (result.status == AllocResultCode::SUCCESS) {
            break;
        } else if (result.status == AllocResultCode::FAIL_UNSAT_SLICING) {
            LOG_FEATURE("alloc_progress", 5, "Failed: Constraints not satisfied, stopping");
            break;
        } else {
            LOG_FEATURE("alloc_progress", 5,
                        "Failed: PHV allocation with " << config.name << " config");
            if (strategy->get_unallocatable_list()) {
                unallocatable_lists.push_back(*(strategy->get_unallocatable_list()));
                LOG_FEATURE(
                    "alloc_progress", 5,
                    TAB1 "Possibly unallocatable slice list: " << unallocatable_lists.back());
            }
        }
    }
    if (result.status == AllocResultCode::FAIL && unallocatable_lists.size() > 0) {
        // It's possible that the algorithm created unallocatable clusters during preslicings.
        // We can't detect them upfront because currently action phv constraints is not able to
        // print out packing limitations like TWO_SOURCES_AND_CONSTANT.
        // Until we can print a complete list of packing constraints from action phv constraints,
        // we run a allocation algorithm again with pre_slicing validation - try to allocate
        // all sliced clusters to a empty PHV while pre_slicing. It will ensure that all sliced
        // clusters are allocatable.
        // It's not enabled by default because it will make allocation X2 slower.
        bool all_same = unallocatable_lists.size() == 1 ||
                        std::all_of(unallocatable_lists.begin() + 1, unallocatable_lists.end(),
                                    [&](const PHV::SuperCluster::SliceList *sl) {
                                        return *sl == *(unallocatable_lists.front());
                                    });
        if (all_same) {
            LOG_DEBUG1("Pre-slicing validation is enabled for " << unallocatable_lists.front());
            // create a config that validate pre_slicing results.
            BruteForceStrategyConfig config{/*.name:*/ "less_fragment_validate_pre_slicing"_cs,
                                            /*.is_better:*/ less_fragment_alloc_score_is_better,
                                            /*.max_failure_retry:*/ 0,
                                            /*.max_slicing:*/ 256,
                                            /*.max_sl_alignment_try:*/ 5,
                                            /*.tofino_only:*/ std::nullopt,
                                            /*.pre_slicing_validation:*/ true,
                                            /*.enable_ara_in_overlays:*/ false};
            BruteForceAllocationStrategy *strategy = new BruteForceAllocationStrategy(
                config.name, utils_i, core_alloc_i, empty_alloc, config, pipe_id, phv_i);
            auto new_result = strategy->tryAllocation(alloc, cluster_groups, container_groups);
            if (new_result.status == AllocResultCode::SUCCESS) {
                result = new_result;
            } else {
                warning("found a unallocatable slice list: %1%",
                        cstring::to_cstring(unallocatable_lists.front()));
            }
        }
    }
    return result;
}

const IR::Node *AllocatePHV::apply_visitor(const IR::Node *root_, const char *) {
    if (utils_i.settings.physical_liverange_overlay) {
        BUG_CHECK(utils_i.settings.no_code_change,
                  "If physical_live_range_overlay is enabled,"
                  "then the no_code_change must be enabled.");
    }
    LOG_DEBUG1("--- BEGIN PHV ALLOCATION ----------------------------------------------------");
    root = root_->to<IR::BFN::Pipe>();
    BUG_CHECK(root, "IR root is not a BFN::Pipe: %s", root_);
    log_device_stats();

    if (utils_i.settings.single_gress_parser_group) {
        core_alloc_i.set_single_gress_parser_group();
    }

    if (utils_i.settings.prioritize_ara_inits) {
        core_alloc_i.set_prioritize_ARA_inits();
    }

    int pipeId = root->canon_id();

    // Make sure that fields are not marked as mutex with itself.
    for (const auto &field : phv_i) {
        BUG_CHECK(!utils_i.mutex()(field.id, field.id), "Field %1% can be overlaid with itself.",
                  field.name);
    }

    LOG_DEBUG1(utils_i.pragmas.pa_container_sizes());
    LOG_DEBUG1(utils_i.pragmas.pa_atomic());

    // clear allocation result to create an empty concrete allocation.
    PHV::AllocUtils::clear_slices(phv_i);
    alloc = new PHV::ConcreteAllocation(phv_i, utils_i.uses);
    PHV::ConcreteAllocation empty_alloc = *alloc;
    auto container_groups = PHV::AllocUtils::make_device_container_groups();
    std::list<PHV::SuperCluster *> cluster_groups = utils_i.make_superclusters();

    // Remove super clusters that are entirely allocated to CLOTs.
    ordered_set<PHV::SuperCluster *> to_remove;
    for (auto *sc : cluster_groups)
        if (PHV::AllocUtils::is_clot_allocated(utils_i.clot, *sc)) to_remove.insert(sc);
    for (auto *sc : to_remove) {
        cluster_groups.remove(sc);
        LOG_DEBUG4("Skipping CLOT-allocated super cluster: " << sc);
    }

    std::vector<const PHV::SuperCluster::SliceList *> unallocatable_lists;
    AllocResult result(AllocResultCode::UNKNOWN, alloc->makeTransaction(), {});
    result = brute_force_alloc(*alloc, empty_alloc, unallocatable_lists, cluster_groups,
                               container_groups, pipeId);
    alloc->commit(result.transaction);

    bool allocationDone = (result.status == AllocResultCode::SUCCESS);
    if (allocationDone) {
        PHV::AllocUtils::bind_slices(*alloc, phv_i);
        PHV::AllocUtils::update_slice_refs(phv_i, utils_i.defuse);
        PHV::AllocUtils::sort_and_merge_alloc_slices(phv_i);
        phv_i.set_done(false);
    } else {
        bool firstRoundFit = mau_i.didFirstRoundFit();
        if (firstRoundFit) {
            // Empty table allocation and merged gateways to be sent because the
            // first round of PHV allocation should be redone.
            ordered_map<cstring, ordered_set<int>> tables;
            ordered_map<cstring, std::pair<cstring, cstring>> mergedGateways;
            LOG_DEBUG1(
                "This round of PHV Allocation did not fit. However, the first round of PHV "
                "allocation did. Therefore, falling back onto the first round of PHV "
                "allocation.");
            throw PHVTrigger::failure(tables, tables, mergedGateways, firstRoundFit,
                                      true /* ignorePackConflicts */, false /* metaInitDisable */);
        }
        PHV::AllocUtils::bind_slices(*alloc, phv_i);
        PHV::AllocUtils::sort_and_merge_alloc_slices(phv_i);
    }

    // Redirect all following LOG*s into summary file
    // Print summaries
    auto logfile = createFileLog(pipeId, "phv_allocation_summary_"_cs, 1);
    if (result.status == AllocResultCode::SUCCESS) {
        LOG_DEBUG1("PHV ALLOCATION SUCCESSFUL");
        LOG1(*alloc);  // Not emitting to EventLog on purpose
    } else if (result.status == AllocResultCode::FAIL_UNSAT_SLICING) {
        formatAndThrowError(*alloc, result.remaining_clusters);
        formatAndThrowUnsat(result.remaining_clusters);
    }
    unallocated_i = result.remaining_clusters;
    phv_i.set_done(true);
    Logging::FileLog::close(logfile);

    return root;
}

bool AllocatePHV::diagnoseFailures(const std::list<const PHV::SuperCluster *> &unallocated,
                                   const PHV::AllocUtils &utils) {
    bool rv = false;
    // Prefer actionable error message if we can precisely identify failures for any supercluster.
    for (auto &sc : unallocated) rv |= diagnoseSuperCluster(sc, utils);
    return rv;
}

bool AllocatePHV::diagnoseSuperCluster(const PHV::SuperCluster *sc, const PHV::AllocUtils &utils) {
    auto info = SuperClusterActionDiagnoseInfo(sc);
    if (!info.scCannotBeSplitFurther) {
        return false;
    }
    LOG_DEBUG3("The following supercluster fails allocation and cannot be split further:\n" << sc);
    LOG_DEBUG3("Printing alignments of slice list fields within their containers:");
    for (auto kv : info.fieldAlignments) LOG_DEBUG3(TAB1 << kv.second << " : " << kv.first);
    std::stringstream ss;
    bool diagnosed =
        utils.actions.diagnoseSuperCluster(info.sliceListsOfInterest, info.fieldAlignments, ss);
    if (diagnosed) error("%1%", ss.str());
    return diagnosed;
}

namespace PHV {
namespace Diagnostics {

std::string printField(const PHV::Field *f) {
    return f->externalName() + "<" + std::to_string(f->size) + "b>";
}

std::string printSlice(const PHV::FieldSlice &slice) {
    auto *f = slice.field();
    if (slice.size() == f->size)
        return printField(f);
    else
        return printField(f) + "[" + std::to_string(slice.range().hi) + ":" +
               std::to_string(slice.range().lo) + "]";
}

static std::string printSliceConstraints(const PHV::FieldSlice &slice) {
    auto f = slice.field();
    return printSlice(slice) + ": " + (f->is_solitary() ? "solitary " : "") +
           (f->no_split() ? "no_split " : "") + (f->no_holes() ? "no_holes " : "") +
           (f->used_in_wide_arith() ? "wide_arith " : "") +
           (f->exact_containers() ? "exact_containers" : "");
}

/// @returns a sorted list of explanations, one for each set of conflicting
/// constraints in @sc, or the empty vector if there are no conflicting
/// constraints.
static std::string diagnoseSuperCluster(const PHV::SuperCluster &sc) {
    std::stringstream msg;

    // Dump constraints.
    std::set<const PHV::Field *> fields;
    for (auto *rc : sc.clusters())
        for (auto *ac : rc->clusters())
            for (const auto &slice : *ac) fields.insert(slice.field());
    std::vector<const PHV::Field *> sortedFields(fields.begin(), fields.end());
    std::sort(sortedFields.begin(), sortedFields.end(),
              [](const PHV::Field *f1, const PHV::Field *f2) { return f1->name < f2->name; });
    msg << "These fields have the following field-level constraints:" << std::endl;
    for (auto *f : fields)
        msg << "    " << printSliceConstraints(PHV::FieldSlice(f, StartLen(0, f->size)))
            << std::endl;
    msg << std::endl;

    msg << "These slices must be in the same PHV container group:" << std::endl;
    for (auto *rc : sc.clusters()) {
        for (auto *ac : rc->clusters()) {
            if (!ac->slices().size()) continue;

            // Print singleton aligned cluster.
            if (ac->slices().size() == 1) {
                msg << "    " << PHV::Diagnostics::printSlice(*ac->begin()) << std::endl;
                continue;
            }

            msg << "    These slices must also be aligned within their respective containers:"
                << std::endl;
            for (const auto &slice : *ac)
                msg << "        " << PHV::Diagnostics::printSlice(slice) << std::endl;
        }
    }
    msg << std::endl;

    // Print slice lists (adjacency constraints).
    for (auto *slist : sc.slice_lists()) {
        if (slist->size() <= 1) continue;
        msg << "These fields can (optionally) be packed adjacently in the same container to "
            << "satisfy exact_containers requirements:" << std::endl;
        for (const auto &slice : *slist)
            msg << "    " << PHV::Diagnostics::printSlice(slice) << std::endl;
        msg << std::endl;
    }

    // Highlight specific conflicts that are easy to identify.
    ordered_set<std::string> conflicts;
    for (auto *rc : sc.clusters()) {
        const auto &slices = rc->slices();
        if (!slices.size()) continue;

        // If two slices have different sizes but must go in the same container
        // group, then either (a) the larger one must be split, (b) the smaller
        // one must be packed with adjacent slices of its slice list, or (c)
        // the smaller one must not have the exact_containers requirement.
        for (auto s1 : slices) {
            for (auto s2 : slices) {
                if (s1 == s2) continue;
                ordered_set<const PHV::SuperCluster::SliceList *> s1lists;
                ordered_set<const PHV::SuperCluster::SliceList *> s2lists;
                for (auto *list : sc.slice_list(s1)) s1lists.insert(mergeContiguousSlices(list));
                for (auto *list : sc.slice_list(s2)) s2lists.insert(mergeContiguousSlices(list));

                if (s1.field()->no_split())
                    s1 = PHV::FieldSlice(s1.field(), StartLen(0, s1.field()->size));
                if (s2.field()->no_split())
                    s2 = PHV::FieldSlice(s2.field(), StartLen(0, s2.field()->size));

                auto &larger = s1.size() > s2.size() ? s1 : s2;
                auto largerLists = s1.size() > s2.size() ? s1lists : s2lists;
                auto &smaller = s1.size() < s2.size() ? s1 : s2;
                auto smallerLists = s1.size() < s2.size() ? s1lists : s2lists;

                // Skip same-sized slices.
                if (larger.size() == smaller.size()) continue;
                // Skip if larger field can be split.
                if (!larger.field()->no_split()) continue;
                // Skip if smaller field doesn't have exact_containers.
                if (!smaller.field()->exact_containers()) continue;

                // If the smaller field can't be packed or doesn't have
                // adjacent slices, that's a problem.
                std::string sm = printSlice(smaller);
                std::string lg = printSlice(larger);
                if (smaller.field()->is_solitary() && smaller.field()->size != larger.size()) {
                    std::stringstream ss;
                    ss << "Constraint conflict: ";
                    ss << "Field slices " << sm << " and " << lg
                       << " must be in the same PHV "
                          "container group, but "
                       << lg << " cannot be split, and " << sm
                       << " (a) must fill an entire container, (b) cannot be packed with "
                          "adjacent fields, and (c) is not the same size as "
                       << lg << ".";
                    conflicts.insert(ss.str());
                } else {
                    for (auto *sliceList : smallerLists) {
                        int listSize = PHV::SuperCluster::slice_list_total_bits(*sliceList);
                        if (listSize < larger.size()) {
                            std::stringstream ss;
                            ss << "Constraint conflict: ";
                            ss << "Field slices " << sm << " and " << lg
                               << " must be in the "
                                  "same PHV container group, but "
                               << lg
                               << " cannot be split, "
                                  "and "
                               << sm
                               << " (a) must fill an entire container with its "
                                  "adjacent fields, but (b) it and its adjacent fields are "
                                  "smaller than "
                               << lg << ".";
                            conflicts.insert(ss.str());
                            break;
                        }
                    }
                }
            }
        }
    }
    for (auto &conflict : conflicts) msg << conflict << std::endl << std::endl;
    return msg.str();
}

}  // namespace Diagnostics
}  // namespace PHV

void AllocatePHV::formatAndThrowError(const PHV::Allocation &alloc,
                                      const std::list<const PHV::SuperCluster *> &unallocated) {
    int unallocated_slices = 0;
    int unallocated_bits = 0;
    int ingress_phv_bits = 0;
    int egress_phv_bits = 0;
    int ingress_t_phv_bits = 0;
    int egress_t_phv_bits = 0;
    std::stringstream msg;
    std::stringstream errorMessage;

    msg << std::endl << "The following fields were not allocated: " << std::endl;
    for (auto *sc : unallocated)
        sc->forall_fieldslices([&](const PHV::FieldSlice &s) {
            msg << "    " << PHV::Diagnostics::printSlice(s) << std::endl;
            unallocated_slices++;
        });
    msg << std::endl;
    errorMessage << msg.str();

    bool have_tagalong = Device::phvSpec().hasContainerKind(PHV::Kind::tagalong);

    if (LOGGING(1)) {
        msg << "Fields successfully allocated: " << std::endl;
        msg << alloc << std::endl;
    }
    for (auto *super_cluster : unallocated) {
        for (auto *rotational_cluster : super_cluster->clusters()) {
            for (auto *cluster : rotational_cluster->clusters()) {
                for (auto &slice : cluster->slices()) {
                    bool can_be_tphv = have_tagalong && cluster->okIn(PHV::Kind::tagalong);
                    unallocated_bits += slice.size();
                    if (slice.gress() == INGRESS) {
                        if (!can_be_tphv)
                            ingress_phv_bits += slice.size();
                        else
                            ingress_t_phv_bits += slice.size();
                    } else {
                        if (!can_be_tphv)
                            egress_phv_bits += slice.size();
                        else
                            egress_t_phv_bits += slice.size();
                    }
                }
            }
        }
    }

    if (LOGGING(1)) {
        msg << std::endl << "..........Unallocated bits = " << unallocated_bits << std::endl;
        msg << "..........ingress phv bits = " << ingress_phv_bits << std::endl;
        msg << "..........egress phv bits = " << egress_phv_bits << std::endl;
        if (have_tagalong) {
            msg << "..........ingress t_phv bits = " << ingress_t_phv_bits << std::endl;
            msg << "..........egress t_phv bits = " << egress_t_phv_bits << std::endl;
        }
        msg << std::endl;
    }

    PHV::AllocationReport report(alloc, true);
    msg << report.printSummary() << std::endl;
    msg << "PHV allocation was not successful " << "(" << unallocated_slices
        << " field slices remaining)" << std::endl;
    LOG_DEBUG1(msg.str());
    error(
        "PHV allocation was not successful\n"
        "%1% field slices remain unallocated\n"
        "%2%",
        unallocated_slices, errorMessage.str());
}

void AllocatePHV::formatAndThrowUnsat(const std::list<const PHV::SuperCluster *> &unsat) {
    std::stringstream msg;
    msg << "Some fields cannot be allocated because of unsatisfiable constraints." << std::endl
        << std::endl;

    // Pretty-print the kinds of constraints.
    /*
    msg << R"(Constraints:
    solitary:   The field cannot be allocated in the same PHV container(s) as any
                other fields.  Fields that are shifted or are the destination of
                arithmetic operations have this constraint.

    no_split:   The field must be entirely allocated to a single PHV container.
                Fields that are shifted or are a source or destination of an
                arithmetic operation have this constraint.

    exact_containers:
                If any field slice in a PHV container has this constraint, then
                the container must be completely filled, and if it contains more
                than one slice, all slices must also have adjaceny constraints.
                All header fields have this constraint.

    adjacency:  If two or more fields are marked as adjacent, then they must be
                placed contiguously (in order) if placed in the same PHV
                container.  Fields in the same header are marked as adjacent.

    aligned:    If two or more fields are marked with an alignment constraint,
                then they must be placed starting at the same least-significant
                bit in their respective PHV containers.

    grouped:    Fields involved in the same instructions must be placed in the
                same PHV container group.  Note that the 'aligned' constraint
                implies 'grouped'.)"
                << std::endl
                << std::endl;
    */

    // Pretty-print the supercluster constraints.
    msg << "The following constraints are mutually unsatisfiable." << std::endl << std::endl;
    for (auto *sc : unsat) msg << PHV::Diagnostics::diagnoseSuperCluster(*sc);

    msg << "PHV allocation was not successful " << "(" << unsat.size() << " set"
        << (unsat.size() == 1 ? "" : "s") << " of unsatisfiable constraints remaining)"
        << std::endl;
    LOG_DEBUG3(msg.str());
    error("%1%", msg.str());
}

BruteForceAllocationStrategy::BruteForceAllocationStrategy(const cstring name,
                                                           const PHV::AllocUtils &utils,
                                                           const CoreAllocation &core,
                                                           const PHV::Allocation &empty_alloc,
                                                           const BruteForceStrategyConfig &config,
                                                           int pipeId, PhvInfo &phv)
    : AllocationStrategy(name, utils, core),
      empty_alloc_i(empty_alloc),
      config_i(config),
      pipe_id_i(pipeId),
      phv_i(phv) {}

ordered_set<bitvec> BruteForceAllocationStrategy::calc_slicing_schemas(
    const PHV::SuperCluster *sc, const std::set<PHV::ConcreteAllocation::AvailableSpot> &spots) {
    ordered_set<bitvec> rst;
    const std::vector<int> chunk_sizes = {7, 6, 5, 4, 3, 2, 1};

    auto gress = sc->gress();
    int size = sc->max_width();
    bool hasDeparsed = sc->deparsed();

    // available spots suggested slicing
    bitvec suggested_slice_schema;
    int covered_size = 0;
    for (const auto &spot : boost::adaptors::reverse(spots)) {
        if (spot.gress && spot.gress != gress) continue;
        if (hasDeparsed && spot.deparserGroupGress && spot.deparserGroupGress != gress) continue;
        if (spot.n_bits >= size) continue;
        if (!sc->okIn(spot.container.type().kind())) continue;
        covered_size += spot.n_bits;
        if (covered_size >= size) break;
        suggested_slice_schema.setbit(covered_size);
    }
    if (covered_size >= size && !(suggested_slice_schema.empty())) {
        LOG_DEBUG5("Adding schema: covered_size = " << covered_size << ", size = " << size
                                                    << ", schema = " << suggested_slice_schema);
        rst.insert(suggested_slice_schema);
    }

    // slice them to n-bit chunks.
    for (int sz : chunk_sizes) {
        bitvec slice_schema;
        for (int i = sz; i < sc->max_width(); i += sz) {
            slice_schema.setbit(i);
        }
        if (!(slice_schema.empty())) {
            LOG_DEBUG5("Adding schema:  sz = " << sz << ", schema = " << slice_schema);
            rst.insert(slice_schema);
        }
    }
    return rst;
}

std::list<PHV::SuperCluster *> BruteForceAllocationStrategy::allocDeparserZeroSuperclusters(
    PHV::Transaction &rst, std::list<PHV::SuperCluster *> &cluster_groups) {
    LOG_DEBUG4("Allocating Deparser Zero Superclusters");
    std::list<PHV::SuperCluster *> allocated_sc;
    for (auto *sc : cluster_groups) {
        LOG_DEBUG4(sc);
        if (auto partial_alloc = core_alloc_i.try_deparser_zero_alloc(rst, *sc, phv_i)) {
            allocated_sc.push_back(sc);
            LOG_DEBUG4(TAB1 "Committing allocation for deparser zero supercluster.");
            rst.commit(*partial_alloc);
        } else {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed: Unable to allocate deparser zero supercluster.");
        }
    }
    // Remove allocated deparser zero superclusters.
    for (auto *sc : allocated_sc) cluster_groups.remove(sc);
    return cluster_groups;
}

std::optional<PHV::Transaction> CoreAllocation::try_deparser_zero_alloc(
    const PHV::Allocation &alloc, PHV::SuperCluster &cluster, PhvInfo &phv) const {
    auto alloc_attempt = alloc.makeTransaction();
    if (PHV::AllocUtils::is_clot_allocated(utils_i.clot, cluster)) {
        LOG_DEBUG4(TAB1 "Skipping CLOT-allocated super cluster: " << cluster);
        return alloc_attempt;
    }
    for (auto *sl : cluster.slice_lists()) {
        int slice_list_offset = 0;
        for (const auto &slice : *sl) {
            LOG_DEBUG5(TAB2 "Slice in slice list: " << slice);
            std::map<gress_t, PHV::Container> zero = {{INGRESS, PHV::Container("B0")},
                                                      {EGRESS, PHV::Container("B16")}};
            std::vector<PHV::AllocSlice> candidate_slices;
            int slice_width = slice.size();
            int alloc_slice_width = 8;
            for (int i = 0; i < slice_width; i += alloc_slice_width) {
                int slice_bits_to_next_byte = ((slice_list_offset / 7) + 1) * 8 - slice_list_offset;
                int slice_bits_remaining = slice_width - i;
                alloc_slice_width = std::min(slice_bits_to_next_byte, slice_bits_remaining);
                LOG_DEBUG5(TAB3 "Alloc slice width: " << alloc_slice_width);
                LOG_DEBUG5(TAB3 "Slice list offset: " << slice_list_offset);
                le_bitrange container_slice = StartLen(slice_list_offset % 8, alloc_slice_width);
                le_bitrange field_slice = StartLen(i + slice.range().lo, alloc_slice_width);
                LOG_DEBUG4(TAB3 "Container slice: " << container_slice
                                                    << ", field slice: " << field_slice);
                BUG_CHECK(slice.gress() == INGRESS || slice.gress() == EGRESS,
                          "Found a field slice for %1% that is neither ingress nor egress",
                          slice.field()->name);
                auto alloc_slice =
                    PHV::AllocSlice(utils_i.phv.field(slice.field()->id), zero[slice.gress()],
                                    field_slice, container_slice);
                candidate_slices.push_back(alloc_slice);
                phv.addZeroContainer(slice.gress(), zero[slice.gress()]);
                slice_list_offset += alloc_slice_width;
            }
            for (auto &alloc_slice : candidate_slices)
                alloc_attempt.allocate(alloc_slice, nullptr, singleGressParserGroups);
        }
    }
    return alloc_attempt;
}

std::list<PHV::SuperCluster *> BruteForceAllocationStrategy::pounderRoundAllocLoop(
    PHV::Transaction &rst, std::list<PHV::SuperCluster *> &cluster_groups,
    const std::list<PHV::ContainerGroup *> &container_groups) {
    // PounderRound in JBay make some tests timeout because
    // there are so many unallocated clusters in some JBay tests.
    // If there are so many unallocated clusters, pounder round is unlikely to
    // work in this case, and it might make compiler timeout.
    const int N_CLUSTER_LIMITATION = 20;
    if (cluster_groups.size() > N_CLUSTER_LIMITATION) {
        return {};
    }
    if (Device::currentDevice() != Device::TOFINO) {
        return {};
    }

    auto score_ctx = ScoreContext(name, false, config_i.is_better);
    std::list<PHV::SuperCluster *> allocated_sc;
    for (auto *sc : cluster_groups) {
        // clusters with slice lists are not considered.
        if (sc->slice_lists().size()) {
            continue;
        }

        bool has_checksummed = sc->any_of_fieldslices(
            [&](const PHV::FieldSlice &fs) { return fs.field()->is_checksummed(); });
        if (has_checksummed) {
            continue;
        }

        auto available_spots = rst.available_spots();
        ordered_set<bitvec> slice_schemas = calc_slicing_schemas(sc, available_spots);
        // Try different slicing, from large to small
        for (const auto &slice_schema : slice_schemas) {
            LOG_DEBUG5(TAB1 "Splitting supercluster" << sc << "using schema " << slice_schema);
            auto slice_rst = PHV::Slicing::split_rotational_cluster(sc, slice_schema);
            if (!slice_rst) continue;

            if (LOGGING(5)) {
                LOG_DEBUG5(TAB1 "Pounder slicing: " << sc << " to:");
                for (auto *new_sc : *slice_rst) {
                    LOG_DEBUG5(TAB2 << new_sc);
                }
            }

            std::list<PHV::SuperCluster *> sliced_sc = *slice_rst;
            auto try_this_slicing = rst.makeTransaction();
            allocLoop(try_this_slicing, sliced_sc, container_groups, score_ctx);
            // succ
            if (sliced_sc.size() == 0) {
                rst.commit(try_this_slicing);
                allocated_sc.push_back(sc);
                break;
            }
        }
    }

    for (auto cluster_group : allocated_sc) cluster_groups.remove(cluster_group);
    return allocated_sc;
}

std::optional<const PHV::SuperCluster::SliceList *>
BruteForceAllocationStrategy::preslice_validation(
    const std::list<PHV::SuperCluster *> &clusters,
    const std::list<PHV::ContainerGroup *> &container_groups) const {
    for (PHV::SuperCluster *cluster : clusters) {
        int n_tried = 0;
        bool succ = false;
        auto itr_ctx = utils_i.make_slicing_ctx(cluster);
        itr_ctx->set_config(slicing_config);
        std::optional<const PHV::SuperCluster::SliceList *> last_invald;
        itr_ctx->iterate([&](const std::list<PHV::SuperCluster *> &sliced) {
            ++n_tried;
            for (auto *sc : sliced) {
                // we don't validate stridedAlloc cluster.
                if (!sc->needsStridedAlloc()) {
                    if (auto unallocatable = diagnose_slicing({sc}, container_groups)) {
                        itr_ctx->invalidate(*unallocatable);
                        last_invald = *unallocatable;
                        return n_tried < config_i.max_slicing;
                    }
                }
            }
            // all allocated
            succ = true;
            return false;
        });
        if (!succ) {
            if (last_invald) {
                return last_invald;
            } else {
                LOG_DEBUG1("preslice_validation cannot allocate "
                           << cluster << "but no unallocatable slice list was found");
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

std::list<PHV::SuperCluster *> BruteForceAllocationStrategy::preslice_clusters(
    const std::list<PHV::SuperCluster *> &cluster_groups,
    const std::list<PHV::ContainerGroup *> &container_groups,
    std::list<const PHV::SuperCluster *> &unsliceable) {
    LOG_DEBUG5("\n===================  Pre-Slicing ===================");
    std::list<PHV::SuperCluster *> rst;
    for (auto *sc : cluster_groups) {
        LOG_DEBUG5("Preslicing:\n" TAB1 << sc);
        // Try until we find one (1) satisfies pa_container_size pragmas.
        // (2) allocatable, because we do not take action constraints into account
        // in slicing iterator, we need to do a virtual allocation test to see whether
        // the presliced clusters can be allocated. It is enabled if
        // config.pre_slicing_validation is true.
        bool found = false;
        int n_tried = 0;
        std::list<PHV::SuperCluster *> sliced;

        auto itr_ctx = utils_i.make_slicing_ctx(sc);
        itr_ctx->set_config(slicing_config);
        itr_ctx->iterate([&](std::list<PHV::SuperCluster *> sliced_clusters) {
            n_tried++;
            if (n_tried > config_i.max_slicing) {
                return false;
            }
            if (config_i.pre_slicing_validation) {
                // validation did not pass
                if (auto unallocatable = preslice_validation(sliced_clusters, container_groups)) {
                    itr_ctx->invalidate(*unallocatable);
                    return true;
                }
            }
            found = true;
            sliced = sliced_clusters;
            return false;
        });
        if (found) {
            LOG_DEBUG5("--- into new slices -->");
            for (auto *new_sc : sliced) {
                LOG_DEBUG5(TAB1 << new_sc);
                rst.push_back(new_sc);
            }
        } else {
            LOG_FEATURE("alloc_progress", 5,
                        "Failed: Slicing tried " << n_tried << " but still failed");
            unsliceable.push_back(sc);
            print_or_throw_slicing_error(utils_i, sc, n_tried);
            break;
        }
    }
    return rst;
}

AllocResult BruteForceAllocationStrategy::tryAllocationFailuresFirst(
    const PHV::Allocation &alloc, const std::list<PHV::SuperCluster *> &cluster_groups_input,
    const std::list<PHV::ContainerGroup *> &container_groups,
    const ordered_set<const PHV::Field *> &failures) {
    // remove singleton un_referenced fields
    std::list<PHV::SuperCluster *> cluster_groups =
        PHV::AllocUtils::remove_unref_clusters(utils_i.uses, cluster_groups_input);

    // remove singleton metadata slice list
    // TODO: This was introduced because some metadata fields are placed
    // in supercluster but it should not. If this does not happen anymore, we should
    // remove this.
    cluster_groups = PHV::AllocUtils::remove_singleton_metadata_slicelist(cluster_groups);

    // slice and then sort clusters.
    std::list<const PHV::SuperCluster *> unsliceable;
    cluster_groups = preslice_clusters(cluster_groups, container_groups, unsliceable);

    // fail early if some clusters have unsatisfiable constraints.
    if (unsliceable.size()) {
        return AllocResult(AllocResultCode::FAIL_UNSAT_SLICING, alloc.makeTransaction(),
                           std::move(unsliceable));
    }

    // remove deparser zero superclusters.
    std::list<PHV::SuperCluster *> deparser_zero_superclusters;
    cluster_groups =
        remove_deparser_zero_superclusters(cluster_groups, deparser_zero_superclusters);

    cluster_groups =
        PHV::AllocUtils::create_strided_clusters(utils_i.strided_headers, cluster_groups);

    // Sorting clusters must happen after the deparser zero superclusters are removed.
    sortClusters(cluster_groups);

    // prioritize previously failed clusters
    const auto priority = [&](PHV::SuperCluster *sc) -> int {
        for (const auto &f : failures) {
            if (sc->contains(f)) {
                return 1;
            }
        }
        return 0;
    };
    cluster_groups.sort(
        [&](PHV::SuperCluster *l, PHV::SuperCluster *r) { return priority(l) > priority(r); });

    auto rst = alloc.makeTransaction();

    // Allocate deparser zero fields first.
    auto allocated_dep_zero_clusters =
        allocDeparserZeroSuperclusters(rst, deparser_zero_superclusters);

    if (deparser_zero_superclusters.size() > 0) {
        LOG_DEBUG1(
            "Failed: Deparser Zero field allocation: " << deparser_zero_superclusters.size());
    }

    // Packing opportunities for each field, if allocated in @p cluster_groups order.
    auto *packing_opportunities = new FieldPackingOpportunity(
        cluster_groups, utils_i.actions, utils_i.uses, utils_i.defuse, utils_i.mutex());
    log_packing_opportunities(packing_opportunities, cluster_groups);

    auto score_ctx = ScoreContext(name, false, config_i.is_better);
    score_ctx = score_ctx.with(packing_opportunities);
    auto allocated_clusters = allocLoop(rst, cluster_groups, container_groups, score_ctx);

    // Pounder Round
    if (cluster_groups.size() > 0) {
        LOG_DEBUG5(cluster_groups.size()
                   << " superclusters are unallocated before Pounder Round, they are:");
        for (auto *sc : cluster_groups) {
            LOG_DEBUG5(TAB1 << sc);
        }

        LOG_DEBUG5("Pounder Round");
        auto allocated_cluster_powders =
            pounderRoundAllocLoop(rst, cluster_groups, container_groups);
    }

    if (cluster_groups.size() > 0 || deparser_zero_superclusters.size() > 0) {
        return AllocResult(
            AllocResultCode::FAIL, std::move(rst),
            std::list<const PHV::SuperCluster *>{cluster_groups.begin(), cluster_groups.end()});
    }

    return AllocResult(
        AllocResultCode::SUCCESS, std::move(rst),
        std::list<const PHV::SuperCluster *>{cluster_groups.begin(), cluster_groups.end()});
}

AllocResult BruteForceAllocationStrategy::tryAllocation(
    const PHV::Allocation &alloc, const std::list<PHV::SuperCluster *> &cluster_groups_input,
    const std::list<PHV::ContainerGroup *> &container_groups) {
    ordered_set<const PHV::Field *> failed;
    AllocResult rst(AllocResultCode::UNKNOWN, alloc.makeTransaction(), {});
    cstring log_prefix = "allocation("_cs + name + "): "_cs;
    bool succ = false;
    int max_try = config_i.max_failure_retry + 1;
    for (int i = 0; i < max_try; i++) {
        LOG_DEBUG1(log_prefix << "Try allocation for the " << i + 1 << "th time");
        if (failed.size() > 0) {
            LOG_DEBUG1(TAB1 "Try again with failures prioritized");
            for (const auto &fs : failed) {
                LOG_DEBUG1(TAB2 << fs);
            }
        }
        rst = tryAllocationFailuresFirst(alloc, cluster_groups_input, container_groups, failed);
        if (rst.status != AllocResultCode::SUCCESS) {
            if (rst.status == AllocResultCode::FAIL) {
                for (const auto &sc : rst.remaining_clusters) {
                    sc->forall_fieldslices(
                        [&](const PHV::FieldSlice &fs) { failed.insert(fs.field()); });
                }
            } else {
                break;
            }
        } else {
            LOG_DEBUG1(log_prefix << "Succeeded");
            succ = true;
            break;
        }
    }
    if (!succ) {
        LOG_DEBUG1(log_prefix << "Failed: After " << max_try << " tries, " << "failure code "
                              << int(rst.status));
    }
    return rst;
}

void BruteForceAllocationStrategy::sortClusters(std::list<PHV::SuperCluster *> &cluster_groups) {
    // Critical Path result are not used.
    // auto critical_clusters = critical_path_clusters_i.calc_critical_clusters(cluster_groups);
    std::set<const PHV::SuperCluster *> has_solitary;
    std::set<const PHV::SuperCluster *> has_no_split;
    std::set<const PHV::SuperCluster *> has_prefer_container_size;
    std::map<const PHV::SuperCluster *, int> n_valid_starts;
    std::map<const PHV::SuperCluster *, int> n_required_length;
    std::set<const PHV::SuperCluster *> pounder_clusters;
    std::set<const PHV::SuperCluster *> non_sliceable;
    std::set<const PHV::SuperCluster *> has_pov;
    std::map<const PHV::SuperCluster *, int> n_extracted_uninitialized;
    std::map<const PHV::SuperCluster *, size_t> n_container_size_pragma;
    std::set<const PHV::SuperCluster *> has_container_type_pragma;

    // calc whether the cluster has pov bits.
    for (auto *cluster : cluster_groups) {
        cluster->forall_fieldslices([&](const PHV::FieldSlice &fs) {
            if (fs.field()->pov) has_pov.insert(cluster);
        });
    }

    // calc whether the cluster has container type pragma. Only for JBay.
    if (Device::currentDevice() != Device::TOFINO) {
        for (auto *cluster : cluster_groups) {
            cluster->forall_fieldslices([&](const PHV::FieldSlice &fs) {
                if (utils_i.pragmas.pa_container_type().required_kind(fs.field()))
                    has_container_type_pragma.insert(cluster);
            });
        }
    }

    // calc n_container_size_pragma.
    const auto &container_sizes = utils_i.pragmas.pa_container_sizes();
    for (auto *cluster : cluster_groups) {
        ordered_set<const PHV::Field *> fields;
        cluster->forall_fieldslices([&](const PHV::FieldSlice &fs) {
            if (container_sizes.is_specified(fs.field())) fields.insert(fs.field());
        });
        n_container_size_pragma[cluster] = fields.size();
    }

    // calc num_pack_conflicts
    for (auto *super_cluster : cluster_groups) super_cluster->calc_pack_conflicts();

    // calc n_extracted_uninitialized
    for (auto *cluster : cluster_groups) {
        n_extracted_uninitialized[cluster] = 0;
        cluster->forall_fieldslices([&](const PHV::FieldSlice &fs) {
            auto *f = fs.field();
            if (utils_i.uses.is_extracted(f) || utils_i.defuse.hasUninitializedRead(f->id)) {
                n_extracted_uninitialized[cluster]++;
            }
        });
    }

    // calc has_solitary and no_split.
    for (const auto *super_cluster : cluster_groups) {
        n_valid_starts[super_cluster] = (std::numeric_limits<int>::max)();
        for (const auto *rot : super_cluster->clusters()) {
            for (const auto *ali : rot->clusters()) {
                bitvec starts = ali->validContainerStart(PHV::Size::b32);
                int n_starts = std::accumulate(starts.begin(), starts.end(), 0,
                                               [](int a, int) { return a + 1; });
                n_valid_starts[super_cluster] = std::min(n_valid_starts[super_cluster], n_starts);
                n_valid_starts[super_cluster] = std::min(n_valid_starts[super_cluster], 5);

                for (const auto &slice : ali->slices()) {
                    if (slice.field()->is_solitary()) {
                        has_solitary.insert(super_cluster);
                    }
                    if (slice.field()->no_split() || slice.field()->has_no_split_at_pos()) {
                        has_no_split.insert(super_cluster);
                    }
                    if (slice.field()->prefer_container_size() != PHV::Size::null) {
                        has_prefer_container_size.insert(super_cluster);
                    }
                }
            }
        }
    }

    // calc pounder-able clusters.
    for (const auto *super_cluster : cluster_groups) {
        if (has_no_split.count(super_cluster) || has_solitary.count(super_cluster)) continue;
        if (super_cluster->exact_containers()) continue;
        if (n_valid_starts[super_cluster] <= 4) continue;
        if (super_cluster->slice_lists().size() > 1) continue;

        bool is_candidate = true;

        for (const auto *slice_list : super_cluster->slice_lists()) {
            if (slice_list->size() > 1) is_candidate = false;
        }

        for (const auto *rot : super_cluster->clusters()) {
            for (const auto *ali : rot->clusters()) {
                if (ali->slices().size() > 1) is_candidate = false;
                for (const auto &fs : ali->slices()) {
                    // pov bits are not
                    if (fs.field()->pov) {
                        is_candidate = false;
                    }
                }
            }
        }

        if (!is_candidate) continue;
        pounder_clusters.insert(super_cluster);
    }

    // TODO: This part of the code is moved from a previous implementation of checking
    // whether a super cluster can be split or not. The implementation was incorrect,
    // but if we change this to a correct version, many regressions were triggered, because
    // the order of allocation is drastically changed and tests were fitting by pragmas.
    // We will leave it here until we have time to fix regression issues.
    const auto is_sliceable_wrong_version = [](const PHV::SuperCluster *sc) {
        int sc_width = 0;
        for (const auto *slice_list : sc->slice_lists()) {
            int size = PHV::SuperCluster::slice_list_total_bits(*slice_list);
            if (slice_list->front().field()->exact_containers()) {
                sc_width = (sc_width < size) ? size : sc_width;
                if (size == 8) return false;
            }
        }
        if (!sc->exact_containers()) return true;
        for (const auto *slice_list : sc->slice_lists()) {
            int min_no_split = INT_MAX;
            int max_no_split = -1;
            int offset = 0;
            for (auto &slice : *slice_list) {
                offset += slice.size();
                if (!slice.field()->no_split()) continue;
                int start = offset - slice.size();
                min_no_split = (min_no_split > start) ? start : min_no_split;
                max_no_split = (max_no_split < offset) ? offset : max_no_split;
            }
            // Found no split slice in this slice list.
            if (min_no_split != INT_MAX && max_no_split != -1) {
                int roundupSize = 8 * ROUNDUP(max_no_split, 8);
                if (min_no_split < 8 && roundupSize >= sc_width) return false;
            }
        }
        return true;
    };

    // calc non_sliceable clusters
    // const auto& pa_container_sizes = core_alloc_i.pragmas().pa_container_sizes();
    for (const auto *cluster : cluster_groups) {
        if (!is_sliceable_wrong_version(cluster)) {
            non_sliceable.insert(cluster);
        }
        //// The correct version of is_sliceable check.
        // auto itr_ctx = PHV::Slicing::ItrContext(cluster, pa_container_sizes.field_to_layout(),
        //                                         has_pack_conflict_i, is_referenced_i);
        // int cnt = 0;
        // itr_ctx.iterate([&](std::list<PHV::SuperCluster*>) {
        //     cnt++;
        //     return cnt <= 1;
        // });
        // if (cnt <= 1) {
        //     non_sliceable.insert(cluster);
        // }
    }

    // calc required_length, i.e. max(max{fieldslice.size()}, {slicelist.size()}).
    for (const auto *super_cluster : cluster_groups) {
        n_required_length[super_cluster] = super_cluster->max_width();
        for (const auto *slice_list : super_cluster->slice_lists()) {
            BUG_CHECK(slice_list->size() > 0, "empty slice list");
            int length = PHV::SuperCluster::slice_list_total_bits(*slice_list);
            n_required_length[super_cluster] = std::max(n_required_length[super_cluster], length);
        }
    }

    // Other heuristics:
    // if (has_pov.count(l) != has_pov.count(r)) {
    //     return has_pov.count(l) > has_pov.count(r); }
    // if (pounder_clusters.count(l) != pounder_clusters.count(r)) {
    //     return pounder_clusters.count(l) < pounder_clusters.count(r); }
    // if (critical_clusters.count(l) != critical_clusters.count(r)) {
    //     return critical_clusters.count(l) > critical_clusters.count(r); }
    // if (l->max_width() != r->max_width()) {
    //     return l->max_width() > r->max_width(); }

    auto ClusterGroupComparator = [&](PHV::SuperCluster *l, PHV::SuperCluster *r) {
        if (Device::currentDevice() != Device::TOFINO) {
            // if (has_container_type_pragma.count(l) != has_container_type_pragma.count(r)) {
            //     return has_container_type_pragma.count(l) > has_container_type_pragma.count(r); }
            if (n_container_size_pragma.at(l) != n_container_size_pragma.at(r))
                return n_container_size_pragma.at(l) > n_container_size_pragma.at(r);
            if (has_prefer_container_size.count(l) != has_prefer_container_size.count(r))
                return has_prefer_container_size.count(l) > has_prefer_container_size.count(r);
            if (has_pov.count(l) != has_pov.count(r)) return has_pov.count(l) > has_pov.count(r);
        }
        if (has_solitary.count(l) != has_solitary.count(r)) {
            return has_solitary.count(l) > has_solitary.count(r);
        }
        if (has_no_split.count(l) != has_no_split.count(r)) {
            return has_no_split.count(l) > has_no_split.count(r);
        }
        if (non_sliceable.count(l) != non_sliceable.count(r)) {
            return non_sliceable.count(l) > non_sliceable.count(r);
        }
        if (bool(l->exact_containers()) != bool(r->exact_containers())) {
            return bool(l->exact_containers()) > bool(r->exact_containers());
        }
        // if it's header fields
        if (bool(l->exact_containers())) {
            if (n_required_length[l] != n_required_length[r]) {
                return n_required_length[l] > n_required_length[r];
            }
            if (l->slice_lists().size() != r->slice_lists().size()) {
                return l->slice_lists().size() > r->slice_lists().size();
            }
        } else {
            if (Device::currentDevice() == Device::TOFINO)
                if (has_pov.count(l) != has_pov.count(r))
                    return has_pov.count(l) > has_pov.count(r);
            // for non header field, aggregate size matters
            if (n_valid_starts.at(l) != n_valid_starts.at(r)) {
                return n_valid_starts.at(l) < n_valid_starts.at(r);
            }
            if (n_extracted_uninitialized[l] != n_extracted_uninitialized[r]) {
                return n_extracted_uninitialized[l] > n_extracted_uninitialized[r];
            }
            if (l->num_pack_conflicts() != r->num_pack_conflicts()) {
                return l->num_pack_conflicts() > r->num_pack_conflicts();
            }
            if (l->aggregate_size() != r->aggregate_size()) {
                return l->aggregate_size() > r->aggregate_size();
            }
            if (n_required_length[l] != n_required_length[r]) {
                return n_required_length[l] > n_required_length[r];
            }
        }
        if (l->num_constraints() != r->num_constraints()) {
            return l->num_constraints() > r->num_constraints();
        }
        return false;
    };
    cluster_groups.sort(ClusterGroupComparator);

    if (LOGGING(5)) {
        LOG_DEBUG5("============ Sorted SuperClusters ===============");
        int i = 0;
        std::stringstream logs;
        for (const auto &v : cluster_groups) {
            ++i;
            logs << i << "th " << " [";
            logs << "n_prefer_cnt_size: " << has_prefer_container_size.count(v) << ", ";
            logs << "is_solitary: " << has_solitary.count(v) << ", ";
            logs << "is_no_split: " << has_no_split.count(v) << ", ";
            logs << "is_non_sliceable: " << non_sliceable.count(v) << ", ";
            logs << "is_exact_container: " << v->exact_containers() << ", ";
            logs << "is_pounderable: " << pounder_clusters.count(v) << ", ";
            logs << "required_length: " << n_required_length[v] << ", ";
            logs << "n_valid_starts: " << n_valid_starts[v] << ", ";
            logs << "n_pack_conflicts: " << v->num_pack_conflicts() << ", ";
            logs << "n_extracted_uninitialized: " << n_extracted_uninitialized[v] << ", ";
            logs << "]\n";
            logs << v;
        }
        LOG_DEBUG5(logs.str());
        LOG_DEBUG5("========== end Sorted SuperClusters =============");
    }
}

/// Check if the giving slicing is valid for strided allocation.
/// To be valid for strided allocation, all fields within cluster
/// must have same slicing.
bool is_valid_stride_slicing(std::list<PHV::SuperCluster *> &slicing) {
    // TODO somehow for the singleton case, the slicing iterator
    // does not slice further, manually break each slice into its own
    // supercluster here ... yuck!
    if (slicing.size() == 1) {
        auto sc = slicing.front();

        std::list<PHV::SuperCluster *> sliced;

        BUG_CHECK(sc->clusters().size() == sc->slice_lists().size(), "malformed supercluster? %1%",
                  sc);

        auto cit = sc->clusters().begin();
        auto sit = sc->slice_lists().begin();

        while (cit != sc->clusters().end()) {
            auto scc = new PHV::SuperCluster({*cit}, {*sit});
            sliced.push_back(scc);
            cit++;
            sit++;
        }

        slicing = sliced;

        return true;
    }

    for (auto x : slicing) {
        bool found_equiv = false;

        for (auto y : slicing) {
            if (x == y) continue;

            if (x->slices().begin()->range() == y->slices().begin()->range()) {
                found_equiv = true;
                break;
            }
        }

        if (!found_equiv) return false;
    }

    return true;
}

std::optional<const PHV::SuperCluster::SliceList *> BruteForceAllocationStrategy::diagnose_slicing(
    const std::list<PHV::SuperCluster *> &slicing,
    const std::list<PHV::ContainerGroup *> &container_groups) const {
    ScoreContext score_ctx("dummy"_cs, true,
                           [](const AllocScore &, const AllocScore &) { return false; });
    LOG_DEBUG3("diagnose_slicing starts");
    auto tx = empty_alloc_i.makeTransaction();
    for (auto *sc : slicing) {
        // cannot diagnose fields with wide_arithmetic.
        if (sc->any_of_fieldslices(
                [](const PHV::FieldSlice &fs) { return fs.field()->used_in_wide_arith(); })) {
            continue;
        }
        bool allocated = false;
        for (const PHV::ContainerGroup *container_group : container_groups) {
            auto partial_alloc = core_alloc_i.try_alloc(tx, *container_group, *sc, 1, score_ctx);
            if (partial_alloc) {
                allocated = true;
                tx.commit(*partial_alloc);
                break;
            }
        }
        if (!allocated) {
            LOG_DEBUG3("diagnose_slicing found a unallocable super cluster: " << sc->uid);
            auto failing_sl =
                core_alloc_i.find_first_unallocated_slicelist(tx, container_groups, *sc);
            if (failing_sl) {
                LOG_DEBUG3("diagnose_slicing found a unallocable slice: " << *failing_sl);
            }
            return failing_sl;
        }
    }
    return std::nullopt;
}

bool BruteForceAllocationStrategy::tryAllocSlicing(
    const std::list<PHV::SuperCluster *> &slicing,
    const std::list<PHV::ContainerGroup *> &container_groups, PHV::Transaction &slicing_alloc,
    const ScoreContext &score_ctx) {
    LOG_DEBUG4("Try alloc slicing:");

    // Place all slices, then get score for that placement.
    for (auto *sc : slicing) {
        // Find best container group for this slice.
        auto best_slice_score = AllocScore::make_lowest();
        std::optional<PHV::Transaction> best_slice_alloc = std::nullopt;
        LOG_DEBUG4("Searching for container group to allocate supercluster with Uid " << sc->uid
                                                                                      << " into");

        for (PHV::ContainerGroup *container_group : container_groups) {
            LOG_FEATURE("alloc_progress", 5, "\nTrying container group: " << container_group);

            if (auto partial_alloc = core_alloc_i.try_alloc(slicing_alloc, *container_group, *sc,
                                                            config_i.max_sl_alignment, score_ctx)) {
                AllocScore score =
                    score_ctx.make_score(*partial_alloc, utils_i.phv, utils_i.clot, utils_i.uses,
                                         utils_i.field_to_parser_states,
                                         utils_i.parser_critical_path, utils_i.tablePackOpt);
                LOG_DEBUG4("Allocation score: " << score);
                if (!best_slice_alloc || score_ctx.is_better(score, best_slice_score)) {
                    best_slice_score = score;
                    best_slice_alloc = partial_alloc;
                    LOG_FEATURE("alloc_progress", 5,
                                "Allocated supercluster " << sc->uid << " to container group "
                                                          << container_group);
                    if (score_ctx.stop_at_first()) {
                        break;
                    }
                }
            } else {
                LOG_FEATURE("alloc_progress", 5,
                            "Failed to allocate supercluster " << sc->uid << " to container group "
                                                               << container_group);
            }
        }

        // Break if this slice couldn't be placed.
        if (best_slice_alloc == std::nullopt) {
            LOG_FEATURE("alloc_progress", 5,
                        "Failed: Cannot find placing for slices in supercluster " << sc->uid);
            return false;
        }

        // Otherwise, update the score.
        slicing_alloc.commit(*best_slice_alloc);
    }

    return true;
}

std::vector<std::list<PHV::SuperCluster *>> reorder_slicing_as_strides(
    unsigned num_strides, const std::list<PHV::SuperCluster *> &slicing) {
    std::vector<PHV::SuperCluster *> vec;
    for (auto sl : slicing) vec.push_back(sl);

    std::vector<std::list<PHV::SuperCluster *>> strides;

    for (unsigned i = 0; i < num_strides; i++) {
        std::list<PHV::SuperCluster *> stride;
        for (unsigned j = 0; j < slicing.size() / num_strides; j++) {
            stride.push_back(vec[j * num_strides + i]);
        }
        strides.push_back(stride);
    }

    if (LOGGING(4)) {
        LOG_DEBUG4("After strided reordering:");
        for (auto &stride : strides) {
            int i = 0;
            LOG_DEBUG4(TAB1 "Stride " << i++ << " :");
            for (auto *sc : stride) LOG_DEBUG4(TAB2 << sc);
        }
    }

    return strides;
}

bool BruteForceAllocationStrategy::tryAllocStride(
    const std::list<PHV::SuperCluster *> &stride,
    const std::list<PHV::ContainerGroup *> &container_groups, PHV::Transaction &stride_alloc,
    const ScoreContext &score_ctx) {
    LOG_DEBUG4("Try alloc slicing stride");

    auto best_score = AllocScore::make_lowest();
    std::optional<PHV::Transaction> best_alloc = std::nullopt;

    auto leader = stride.front();

    for (PHV::ContainerGroup *container_group : container_groups) {
        LOG_FEATURE("alloc_progress", 4, TAB1 "Try container group: " << container_group);

        auto leader_alloc = core_alloc_i.try_alloc(stride_alloc, *container_group, *leader,
                                                   config_i.max_sl_alignment, score_ctx);
        if (leader_alloc) {
            // alloc rest
            if (tryAllocStrideWithLeaderAllocated(stride, *leader_alloc, score_ctx)) {
                AllocScore score =
                    score_ctx.make_score(*leader_alloc, utils_i.phv, utils_i.clot, utils_i.uses,
                                         utils_i.field_to_parser_states,
                                         utils_i.parser_critical_path, utils_i.tablePackOpt);

                if (!best_alloc || score_ctx.is_better(score, best_score)) {
                    best_score = score;
                    best_alloc = leader_alloc;
                    LOG_FEATURE("alloc_progress", 5,
                                "Allocated slicing stride " << leader->uid << " to container group "
                                                            << container_group);
                    if (score_ctx.stop_at_first()) {
                        break;
                    }
                }
            }
        }
    }

    if (best_alloc == std::nullopt) {
        LOG_FEATURE("alloc_progress", 5, "Failed to allocate slicing stride");
        return false;
    }

    stride_alloc.commit(*best_alloc);
    return true;
}

bool BruteForceAllocationStrategy::tryAllocStrideWithLeaderAllocated(
    const std::list<PHV::SuperCluster *> &stride, PHV::Transaction &leader_alloc,
    const ScoreContext &score_ctx) {
    auto leader = stride.front();
    auto slices = leader->slices();
    auto status = leader_alloc.getTransactionStatus();

    PHV::Container prev;

    for (auto &kv : status) {
        for (auto sl : kv.second.slices) {
            if (sl.field() == slices.begin()->field() &&
                sl.field_slice() == slices.begin()->range()) {
                if (prev) BUG("strided slice gets allocated into multiple containers? %1%", sl);

                prev = sl.container();
            }
        }
    }

    for (auto *sc : stride) {
        if (sc == leader) continue;

        unsigned prev_index = Device::phvSpec().physicalAddress(prev, PhvSpec::PARSER);
        auto curr = Device::phvSpec().physicalAddressToContainer(prev_index + 1, PhvSpec::PARSER);

        if (!curr) {
            LOG_FEATURE("alloc_progress", 5, TAB1 "Failed: No next container for " << prev);
            return false;
        }

        // 8b containers are paired in 16b parser containers. If we have an 8b container, we need to
        // maintain the high/low half across the strided allocation.
        if ((Device::currentDevice() == Device::JBAY) &&  // NOLINT(whitespace/parens)
            prev.type().size() == PHV::Size::b8 && prev.index() % 2)
            curr = PHV::Container(curr->type(), curr->index() + 1);

        if (!leader_alloc.getStatus(*curr)) {
            LOG_FEATURE("alloc_progress", 5, TAB1 "Failed: No allocation status for " << *curr);
            return false;
        }

        PHV::ContainerGroup cg(curr->type().size(), {*curr});

        auto sc_alloc =
            core_alloc_i.try_alloc(leader_alloc, cg, *sc, config_i.max_sl_alignment, score_ctx);

        if (!sc_alloc) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Failed to allocate next stride slice in " << *curr);
            return false;
        }

        LOG_FEATURE("alloc_progress", 5, TAB1 "Allocated next stride slice in " << *curr);
        leader_alloc.commit(*sc_alloc);
        prev = *curr;
    }

    return true;
}

bool BruteForceAllocationStrategy::tryAllocSlicingStrided(
    unsigned num_strides, const std::list<PHV::SuperCluster *> &slicing,
    const std::list<PHV::ContainerGroup *> &container_groups, PHV::Transaction &slicing_alloc,
    const ScoreContext &score_ctx) {
    LOG_DEBUG4("Try alloc slicing strided: (num_stride = " << num_strides << ")");

    auto strides = reorder_slicing_as_strides(num_strides, slicing);

    int i = 0;
    for (auto &stride : strides) {
        auto stride_alloc = slicing_alloc.makeTransaction();
        bool succeeded = tryAllocStride(stride, container_groups, stride_alloc, score_ctx);

        if (succeeded) {
            slicing_alloc.commit(stride_alloc);
            LOG_DEBUG4(TAB1 "Stride " << i << " allocation ok");
        } else {
            LOG_FEATURE("alloc_progress", 5, TAB1 "Failed to allocate stride " << i);
            return false;
        }
        i++;
    }

    LOG_DEBUG4("All strides allocated!");

    return true;
}

std::optional<PHV::Transaction> BruteForceAllocationStrategy::tryVariousSlicing(
    PHV::Transaction &rst, PHV::SuperCluster *cluster_group,
    const std::list<PHV::ContainerGroup *> &container_groups, const ScoreContext &score_ctx,
    std::stringstream &alloc_history) {
    auto best_score = AllocScore::make_lowest();
    std::optional<PHV::Transaction> best_alloc = std::nullopt;
    std::optional<std::list<PHV::SuperCluster *>> best_slicing = std::nullopt;
    std::vector<const PHV::SuperCluster::SliceList *> diagnosed_unallocatables;
    int MAX_SLICING_TRY = config_i.max_slicing;

    /// TODO strided cluster can have many slices in the supercluster
    /// and the number of slicing can blow up (need a better way to stop
    /// than this crude heuristic).
    if (cluster_group->needsStridedAlloc() || BackendOptions().quick_phv_alloc)
        MAX_SLICING_TRY = 128;

    int n_tried = 0;

    // Try all possible slicings.
    // auto& pa_container_sizes = utils_i.pragmas.pa_container_sizes();

    auto itr_ctx = utils_i.make_slicing_ctx(cluster_group);
    itr_ctx->set_config(slicing_config);
    itr_ctx->iterate([&](std::list<PHV::SuperCluster *> slicing) {
        ++n_tried;
        if (n_tried > MAX_SLICING_TRY) {
            return false;
        }
        if (LOGGING(4)) {
            LOG_FEATURE("alloc_progress", 4, "Slicing attempt: " << n_tried);
            for (auto *sc : slicing) LOG_DEBUG4(sc);
        }

        if (cluster_group->needsStridedAlloc()) {
            if (!is_valid_stride_slicing(slicing)) {
                LOG_DEBUG4("Invalid stride slicing, continue ...");
                return true;
            }
        }

        LOG_DEBUG4("Valid slicing, try to place all slices in slicing");

        // Place all slices, then get score for that placement.
        auto slicing_alloc = rst.makeTransaction();
        bool succeeded = false;

        if (cluster_group->needsStridedAlloc()) {
            LOG_DEBUG5("invoke strided allocation");
            unsigned num_strides = slicing.size() / cluster_group->slices().size();
            succeeded = tryAllocSlicingStrided(num_strides, slicing, container_groups,
                                               slicing_alloc, score_ctx);
        } else {
            LOG_DEBUG5("invoke normal allocation");
            succeeded = tryAllocSlicing(slicing, container_groups, slicing_alloc, score_ctx);
        }

        if (!succeeded) {
            LOG_FEATURE("alloc_progress", 5, "Failed to allocate with this slicing");
            if (cluster_group->slice_lists().size() > 0) {
                LOG_DEBUG5(TAB1 "Check impossible slicelist");
                if (auto impossible_slicelist = diagnose_slicing(slicing, container_groups)) {
                    LOG_FEATURE("alloc_progress", 5,
                                TAB2 "found slicelist that is impossible to allocate: "
                                    << *impossible_slicelist);
                    itr_ctx->invalidate(*impossible_slicelist);
                    diagnosed_unallocatables.push_back(*impossible_slicelist);
                }
            }
            return true;
        } else {
            LOG_FEATURE("alloc_progress", 5, "Successfully allocate @sc with this slicing");
        }

        // If allocation succeeded, check the score.
        auto slicing_score = score_ctx.make_score(
            slicing_alloc, utils_i.phv, utils_i.clot, utils_i.uses, utils_i.field_to_parser_states,
            utils_i.parser_critical_path, utils_i.tablePackOpt);
        if (LOGGING(4)) {
            LOG_FEATURE("alloc_progress", 4,
                        "Best SUPERCLUSTER score for this slicing: " << slicing_score);
            LOG_FEATURE("alloc_progress", 4, "For the following SUPERCLUSTER slices: ");
            for (auto *sc : slicing) LOG_FEATURE("alloc_progress", 4, sc);
        }
        if (!best_alloc || score_ctx.is_better(slicing_score, best_score)) {
            best_score = slicing_score;
            best_alloc = slicing_alloc;
            best_slicing = slicing;
            LOG_FEATURE("alloc_progress", 4, "...and this is the best score seen so far.");
            if (score_ctx.stop_at_first()) {
                return false;
            }
        }
        return true;
    });

    // Fill the log before the transaction is merged
    if (best_alloc) {
        LOG_FEATURE("alloc_progress", 4, "SUCCESSFULLY allocated " << cluster_group);
        alloc_history << "Successfully Allocated\n";
        alloc_history << "By slicing into the following superclusters:\n";
        for (auto *sc : *best_slicing) {
            alloc_history << sc << "\n";
        }
        alloc_history << "Best Score: " << best_score << "\n";
        alloc_history << "Allocation Decisions:" << "\n";
        print_alloc_history(*best_alloc, *best_slicing, alloc_history);
    } else {
        LOG_FEATURE("alloc_progress", 4, "FAILED to allocate " << cluster_group);
        alloc_history << "FAILED to allocate " << cluster_group << "\n";
        alloc_history << "when the things are like: " << "\n";
        alloc_history << rst.getTransactionSummary() << "\n";
        // TODO: if a slice is unallocatable, then it must be unallocatable in
        // all different slicings, so that diagnosed_unallocatables.size() must be
        // equal or larger to n_tried.
        if (int(diagnosed_unallocatables.size()) > n_tried) {
            bool all_same =
                diagnosed_unallocatables.size() == 1 ||
                std::all_of(diagnosed_unallocatables.begin() + 1, diagnosed_unallocatables.end(),
                            [&](const PHV::SuperCluster::SliceList *sl) {
                                return *sl == *(diagnosed_unallocatables.front());
                            });
            if (all_same) {
                LOG_FEATURE("alloc_progress", 1,
                            "found possible unallocatable slice list: "
                                << diagnosed_unallocatables.front());
                if (!unallocatable_list_i) {
                    unallocatable_list_i = diagnosed_unallocatables.front();
                }
            }
        }
    }
    return best_alloc;
}

std::list<PHV::SuperCluster *> BruteForceAllocationStrategy::allocLoop(
    PHV::Transaction &rst, std::list<PHV::SuperCluster *> &cluster_groups,
    const std::list<PHV::ContainerGroup *> &container_groups, const ScoreContext &score_ctx) {
    // allocation history
    std::stringstream alloc_history;
    int n = 0;
    std::list<PHV::SuperCluster *> allocated;

    BruteForceOptimizationStrategy opt_strategy(*this, container_groups, score_ctx);
    PHV::Transaction try_alloc = rst.makeTransaction();
    for (PHV::SuperCluster *cluster_group : cluster_groups) {
        n++;
        alloc_history << n << ": " << "TRYING to allocate " << cluster_group;
        LOG_FEATURE("alloc_progress", 4, "TRYING to allocate " << cluster_group);

        std::optional<PHV::Transaction> best_alloc =
            tryVariousSlicing(try_alloc, cluster_group, container_groups, score_ctx, alloc_history);

        // If any allocation was found, commit it.
        if (best_alloc) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB1 "Success: allocated cluster group " << cluster_group);
            PHV::Transaction *clone = (*best_alloc).clone(try_alloc);
            opt_strategy.addTransaction(*clone, *cluster_group);
            try_alloc.commit(*best_alloc);
            allocated.push_back(cluster_group);
        }
    }

    // TODO: There must be a better way to remove elements from a list
    // while iterating, but `it = clusters_i.erase(it)` skips elements.
    for (auto cluster_group : allocated) cluster_groups.remove(cluster_group);

    auto logfile = createFileLog(pipe_id_i, "phv_allocation_history_"_cs, 1);
    LOG1("Allocation history of config " << config_i.name);
    LOG1(alloc_history.str());
    Logging::FileLog::close(logfile);

    if (cluster_groups.empty()) {
        // PHV Allocation succeed, no need to do any more steps
        rst.commit(try_alloc);
    } else {
        // PHV Allocation was not able to insert one or more superclusters. Try to move
        // transactions such that some of the actually unassigned supercluster would be able to
        // fit.
        opt_strategy.printContDependency();
        std::list<PHV::SuperCluster *> alloc_by_opt;
        alloc_by_opt = opt_strategy.optimize(cluster_groups, rst);
        if (alloc_by_opt.empty()) rst.commit(try_alloc);
        for (auto cluster_group : alloc_by_opt) allocated.push_back(cluster_group);
    }

    return allocated;
}

std::vector<const PHV::Field *> FieldPackingOpportunity::fieldsInOrder(
    const std::list<PHV::SuperCluster *> &sorted_clusters) const {
    std::set<const PHV::Field *> showed;
    std::vector<const PHV::Field *> rst;
    for (const auto *cluster : sorted_clusters) {
        cluster->forall_fieldslices([&](const PHV::FieldSlice &fs) {
            if (!showed.count(fs.field())) {
                showed.insert(fs.field());
                rst.push_back(fs.field());
            }
        });
    }
    return rst;
}

bool FieldPackingOpportunity::isExtractedOrUninitialized(const PHV::Field *f) const {
    return uses.is_extracted(f) || defuse.hasUninitializedRead(f->id);
}

bool FieldPackingOpportunity::canPack(const PHV::Field *f1, const PHV::Field *f2) const {
    // Being mutex is considered as no pack.
    if (mutex(f1->id, f2->id)) return false;
    // Same stage packing conflits.
    if (actions.hasPackConflict(PHV::FieldSlice(f1), PHV::FieldSlice(f2))) return false;
    // Extracted Uninitilized packing conflits
    if (isExtractedOrUninitialized(f1) && isExtractedOrUninitialized(f2)) return false;
    return true;
}

FieldPackingOpportunity::FieldPackingOpportunity(
    const std::list<PHV::SuperCluster *> &sorted_clusters, const ActionPhvConstraints &actions,
    const PhvUse &uses, const FieldDefUse &defuse, const SymBitMatrix &mutex)
    : actions(actions), uses(uses), defuse(defuse), mutex(mutex) {
    std::vector<const PHV::Field *> fields = fieldsInOrder(sorted_clusters);
    for (auto i = fields.begin(); i != fields.end(); ++i) {
        opportunities[*i] = 0;
        for (auto j = i + 1; j != fields.end(); ++j) {
            if (canPack(*i, *j)) {
                opportunities[*i]++;
            }
        }
    }

    for (auto i = fields.begin(); i != fields.end(); ++i) {
        int sum = 0;
        for (auto j = i + 1; j != fields.end(); ++j) {
            if (canPack(*i, *j)) sum++;
            opportunities_after[{*i, *j}] = opportunities[*i] - sum;
        }
    }
}

int FieldPackingOpportunity::nOpportunitiesAfter(const PHV::Field *f1, const PHV::Field *f2) const {
    if (opportunities_after.count({f1, f2})) {
        return opportunities_after.at({f1, f2});
    } else {
        LOG_DEBUG4("FieldPackingOpportunity for " << f1->name << " " << f2->name
                                                  << " has not benn calculated.");
        return 0;
    }
}

const IR::Node *IncrementalPHVAllocation::apply_visitor(const IR::Node *root, const char *) {
    // phv_i contains out-of-date information at this point, but CollectPhvInfo cannot
    // yet be rerun because of uncommited allocation information.
    //
    // Stale init points/init primitives can cause problems in ActionPhvConstraints::can_pack due to
    // out-of-date Action references. (The actions are updated by AddSliceInitialization and its
    // subpasses when the initializations are inserted. These initializations will be reflected in
    // actions identified by the contstraint tracker, so we no longer need the init info in the
    // slices.) Remove the initializations from the slices.
    static const PHV::ActionSet emptySet;
    static const PHV::DarkInitPrimitive emptyPrim;
    for (auto &f : phv_i) {
        for (auto &slice : f.get_alloc()) {
            slice.setInitPoints(emptySet);
            slice.setInitPrimitive(&emptyPrim);
        }
    }

    PHV::ConcreteAllocation alloc = make_concrete_allocation(phv_i, utils_i.uses);
    auto container_groups = PHV::AllocUtils::make_device_container_groups();
    std::list<PHV::SuperCluster *> cluster_groups;
    const int pipeId = root->to<IR::BFN::Pipe>()->canon_id();
    for (const auto &f : temp_vars_i) {
        cluster_groups.push_back(new PHV::SuperCluster(
            {new PHV::RotationalCluster({new PHV::AlignedCluster(
                PHV::Kind::normal, std::vector<PHV::FieldSlice>{PHV::FieldSlice(f)})})},
            {}));
    }
    BruteForceStrategyConfig config{
        /*.name:*/ "default_incremental_alloc"_cs,
        /*.is_better:*/ default_alloc_score_is_better,
        /*.max_failure_retry:*/ 0,
        /*.max_slicing:*/ 1,
        /*.max_sl_alignment_try:*/ 1,
        /*.tofino_only:*/ std::nullopt,
        /*.pre_slicing_validation:*/ false,
        /*.enable_ara_in_overlays:*/ false,
    };
    AllocResult result(AllocResultCode::UNKNOWN, alloc.makeTransaction(), {});

    BruteForceAllocationStrategy *strategy = new BruteForceAllocationStrategy(
        config.name, utils_i, core_alloc_i, alloc, config, pipeId, phv_i);
    result = strategy->tryAllocation(alloc, cluster_groups, container_groups);
    alloc.commit(result.transaction);
    if (result.status == AllocResultCode::FAIL) {
        LOG_FEATURE("alloc_progress", 1, "Failed: Cannot allocate all temp vars");
    } else {
        PHV::AllocUtils::clear_slices(phv_i);
        PHV::AllocUtils::bind_slices(alloc, phv_i);
        PHV::AllocUtils::update_slice_refs(phv_i, utils_i.defuse);
        PHV::AllocUtils::sort_and_merge_alloc_slices(phv_i);
    }

    auto logfile = createFileLog(pipeId, "phv_allocation_incremental_summary_"_cs, 1);
    if (result.status == AllocResultCode::SUCCESS) {
        LOG1("PHV ALLOCATION SUCCESSFUL");
        LOG2(alloc);
    } else {
        // Did we already tried without temporary creation during table allocation? If so, fail.
        if (settings_i.limit_tmp_creation) {
            error("failed to allocate temp vars created by table placement");
            for (const auto *sc : result.remaining_clusters) {
                sc->forall_fieldslices([](const PHV::FieldSlice &fs) {
                    error("unallocated: %1%", cstring::to_cstring(fs));
                });
            }
        } else {
            LOG1("PHV ALLOCATION FAIL, try without intermediate temp");
            settings_i.limit_tmp_creation = true;
        }
    }
    Logging::FileLog::close(logfile);

    return root;
}
