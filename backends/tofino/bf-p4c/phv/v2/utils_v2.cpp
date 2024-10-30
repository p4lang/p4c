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

#include <bf-p4c/phv/v2/utils_v2.h>

#include "bf-p4c/phv/utils/utils.h"
#include "lib/exceptions.h"

namespace PHV {
namespace v2 {

cstring to_str(const ErrorCode &e) {
    using ec = ErrorCode;
    switch (e) {
        case ec::NOT_ENOUGH_SPACE:
            return "NOT_ENOUGH_SPACE"_cs;
        case ec::ACTION_CANNOT_BE_SYNTHESIZED:
            return "ACTION_CANNOT_BE_SYNTHESIZED"_cs;
        case ec::CONTAINER_TYPE_MISMATCH:
            return "CONTAINER_TYPE_MISMATCH"_cs;
        case ec::CONTAINER_GRESS_MISMATCH:
            return "CONTAINER_GRESS_MISMATCH"_cs;
        case ec::CONTAINER_PARSER_WRITE_MODE_MISMATCH:
            return "CONTAINER_PARSER_WRITE_MODE_MISMATCH"_cs;
        case ec::CONTAINER_PARSER_PACKING_INVALID:
            return "CONTAINER_PARSER_PACKING_INVALID"_cs;
        case ec::FIELD_MAX_CONTAINER_BYTES_EXCEEDED:
            return "FIELD_MAX_CONTAINER_BYTES_EXCEEDED"_cs;
        case ec::ALIGNED_CLUSTER_NO_VALID_START:
            return "ALIGNED_CLUSTER_NO_VALID_START"_cs;
        case ec::ALIGNED_CLUSTER_CANNOT_BE_ALLOCATED:
            return "ALIGNED_CLUSTER_CANNOT_BE_ALLOCATED"_cs;
        case ec::NO_VALID_CONTAINER_SIZE:
            return "NO_VALID_CONTAINER_SIZE"_cs;
        case ec::NO_VALID_SC_ALLOC_ALIGNMENT:
            return "NO_VALID_SC_ALLOC_ALIGNMENT"_cs;
        case ec::WIDE_ARITH_ALLOC_FAILED:
            return "WIDE_ARITH_ALLOC_FAILED"_cs;
        case ec::NO_SLICING_FOUND:
            return "NO_SLICING_FOUND"_cs;
        case ec::INVALID_ALLOC_FOUND_BY_COPACKER:
            return "INVALID_ALLOC_FOUND_BY_COPACKER"_cs;
        case ec::CANNOT_APPLY_REQUIRED_COPACK_HINTS:
            return "CANNOT_APPLY_REQUIRED_COPACK_HINTS"_cs;
        default:
            BUG("unimplemented errorcode: %1%", int(e));
    }
}

constexpr const char *ScoreContext::tab_table[];
constexpr int ScoreContext::max_log_level;

std::string AllocError::str() const {
    std::stringstream ss;
    ss << "code:" << to_str(code) << ", msg:" << msg << ".";
    if (invalid_packing) {
        ss << "\nFound decisions of invalid packing:";
        for (const auto &sl : *invalid_packing) {
            ss << "\n\t" << sl;
        }
    }
    if (reslice_required) {
        ss << "\nNext reslice target:";
        for (const auto &sl : *reslice_required) {
            ss << "\n\t" << sl;
        }
    }
    return ss.str();
}

std::string AllocResult::err_str() const {
    BUG_CHECK(err, "cannot call err_str when err does not exists");
    return err->str();
}

bool AllocResult::operator==(const AllocResult &other) const {
    if (err == nullptr) return (other.err == nullptr);

    if (other.err == nullptr) return false;

    return (err->code == other.err->code);
}

std::string AllocResult::pretty_print_tx(const PHV::Transaction &tx, cstring prefix) {
    std::stringstream ss;
    std::string new_line = "";
    for (const auto &container_status : tx.get_actual_diff()) {
        // const auto& c = container_status.first;
        const auto &slices = container_status.second.slices;
        for (const auto &a : slices) {
            auto fs = PHV::FieldSlice(a.field(), a.field_slice());
            ss << new_line << prefix << "allocate: " << a.container() << "["
               << a.container_slice().lo << ":" << a.container_slice().hi << "] <- " << fs << " @["
               << a.getEarliestLiveness() << "," << a.getLatestLiveness() << "]";
            new_line = "\n";
        }
    }
    return ss.str();
}

std::string AllocResult::tx_str(cstring prefix) const {
    BUG_CHECK(tx, "cannot call tx_str when tx does not exists");
    return pretty_print_tx(*tx, prefix);
}

namespace {

std::vector<ScAllocAlignment> make_slicelist_alignment(const SuperCluster *sc,
                                                       const PHV::Size width,
                                                       const SuperCluster::SliceList *sl) {
    std::vector<ScAllocAlignment> rst;
    const auto valid_list_starts = sc->aligned_cluster(sl->front()).validContainerStart(width);
    for (const int le_offset_start : valid_list_starts) {
        int le_offset = le_offset_start;
        ScAllocAlignment curr;
        bool success = true;
        for (auto &slice : *sl) {
            const AlignedCluster &cluster = sc->aligned_cluster(slice);
            auto valid_start_options = cluster.validContainerStart(width);

            // if the slice 's cluster cannot be placed at the current offset.
            if (!valid_start_options.getbit(le_offset)) {
                success = false;
                break;
            }

            // Return if the slice is part of another slice list but was previously
            // placed at a different start location.
            if (curr.cluster_starts.count(&cluster) &&
                curr.cluster_starts.at(&cluster) != le_offset) {
                success = false;
                break;
            }

            // Otherwise, update the alignment for this slice's cluster.
            curr.cluster_starts[&cluster] = le_offset;
            le_offset += slice.size();
        }
        if (success) {
            rst.push_back(curr);
        }
    }
    return rst;
}

}  // namespace

std::optional<ScAllocAlignment> ScAllocAlignment::merge(const ScAllocAlignment &other) const {
    ScAllocAlignment rst;
    for (auto &alignment : std::vector<const ScAllocAlignment *>{this, &other}) {
        // may conflict
        for (auto &cluster_start : alignment->cluster_starts) {
            const auto *cluster = cluster_start.first;
            const int start = cluster_start.second;
            if (rst.cluster_starts.count(cluster) && rst.cluster_starts[cluster] != start) {
                return std::nullopt;
            }
            rst.cluster_starts[cluster] = start;
        }
    }
    return rst;
}

/// Compute super cluster alignment of @p sc for @p width containers.
/// Example
/// Super Cluster:
/// 1. [f0<9> ^0, f1<8> ^1]
/// 2. [f2<1> ^0, f3<8> ^1]
/// Rot: [ Aligned:[f3, f1] ]
/// => 32-bit container
/// 1: *0*, 8, 16
/// 2: 0, *8*, 16
/// Because f3 and f1 must be perfectly aligned, there are only two possible cluster alignment:
///  (1) slice list 1 at 0, 2 at 8
///  (2) slice list 1 at 8, 2 at 16.
std::vector<ScAllocAlignment> make_sc_alloc_alignment(
    const SuperCluster *sc, const PHV::Size width, const int max_n,
    const std::list<const SuperCluster::SliceList *> *sl_order) {
    // collect all possible alignments for each slice_list
    std::vector<std::vector<ScAllocAlignment>> all_alignments;
    if (sl_order == nullptr) {
        sl_order = new std::list<const SuperCluster::SliceList *>{sc->slice_lists().begin(),
                                                                  sc->slice_lists().end()};
    }
    for (const auto *sl : *sl_order) {
        auto curr_sl_alignment = make_slicelist_alignment(sc, width, sl);
        if (curr_sl_alignment.size() == 0) {
            LOG5("FAIL to build alignment for " << sl);
            break;
        }
        all_alignments.push_back(curr_sl_alignment);
    }
    // not all slice list has valid alignment, simply skip.
    if (all_alignments.size() < sc->slice_lists().size()) {
        return {};
    }

    // build alignment for the super cluster by trying all (limited to max_n)
    // possible combination of alignments of slice lists, by picking one alignment
    // for each slice lists, while making sure none of them are conflicting with others.
    std::vector<ScAllocAlignment> rst;
    std::function<void(int depth, const ScAllocAlignment &curr)> dfs =
        [&](int depth, const ScAllocAlignment &curr) {
            if (depth == int(all_alignments.size())) {
                rst.push_back(curr);
                return;
            }
            for (const auto &align_choice : all_alignments[depth]) {
                auto next = curr.merge(align_choice);
                if (next) {
                    dfs(depth + 1, *next);
                }
                if (int(rst.size()) == max_n) {
                    return;
                }
            }
        };
    dfs(0, ScAllocAlignment());
    return rst;
}

cstring ScAllocAlignment::pretty_print(cstring prefix, const SuperCluster *sc) const {
    std::stringstream ss;
    std::string new_line = "";
    for (const auto *sl : sc->slice_lists()) {
        ss << new_line << prefix << "[";
        new_line = "\n";
        std::string sep = "";
        for (const auto &fs : *sl) {
            ss << sep << "{" << fs.shortString() << ", "
               << cluster_starts.at(&sc->aligned_cluster(fs)) << "}";
            sep = ", ";
        }
        ss << "]";
    }
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const AllocError &e) {
    out << e.str();
    return out;
}

}  // namespace v2
}  // namespace PHV
