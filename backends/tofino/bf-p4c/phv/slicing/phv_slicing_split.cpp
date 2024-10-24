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

#include "bf-p4c/phv/slicing/phv_slicing_split.h"

#include <cstddef>
#include <sstream>

#include "bf-p4c/lib/union_find.hpp"
#include "bf-p4c/phv/utils/utils.h"
#include "lib/log.h"

// TODO: These codes are ported from the old implementation without any changes.
// We should refactor and simplify them later.

namespace PHV {
namespace Slicing {

// Helper function to update internal state of split_super_cluster after
// splitting a rotational cluster.  Mutates each argument.
static void update_slices(
    const RotationalCluster *old, const SliceResult<RotationalCluster> &split_res,
    ordered_set<const RotationalCluster *> &new_clusters,
    ordered_set<SuperCluster::SliceList *> &slice_lists,
    ordered_map<FieldSlice, ordered_set<SuperCluster::SliceList *>> &slices_to_slice_lists,
    assoc::hash_map<const FieldSlice, const RotationalCluster *> &slices_to_clusters) {
    // Update the set of live clusters.
    auto old_it = new_clusters.find(old);
    if (old_it != new_clusters.end()) {
        LOG6("    ...erasing old cluster " << old);
        new_clusters.erase(old);
    }
    new_clusters.insert(split_res.lo);
    LOG6("    ...adding new lo cluster " << split_res.lo);
    new_clusters.insert(split_res.hi);
    LOG6("    ...adding new hi cluster " << split_res.hi);

    // Update the slices_to_clusters map.
    LOG6("    ...updating slices_to_clusters");
    for (auto &kv : split_res.slice_map) {
        BUG_CHECK(slices_to_clusters.find(kv.first) != slices_to_clusters.end(),
                  "Slice not in map: %1%", cstring::to_cstring(kv.first));
        LOG6("        - erasing " << kv.first);
        slices_to_clusters.erase(kv.first);
        auto &slice_lo = kv.second.first;
        LOG6("        - adding " << slice_lo);
        slices_to_clusters[slice_lo] = split_res.lo;
        if (auto &slice_hi = kv.second.second) {
            LOG6("        - adding " << *slice_hi);
            slices_to_clusters[*slice_hi] = split_res.hi;
        }
    }

    // Replace the old slices with the new, split slices in each slice
    // list.
    LOG6("    ...updating slice_lists");
    for (auto *slice_list : slice_lists) {
        for (auto slice_it = slice_list->begin(); slice_it != slice_list->end(); slice_it++) {
            auto &old_slice = *slice_it;
            if (split_res.slice_map.find(old_slice) != split_res.slice_map.end()) {
                auto &slice_lo = split_res.slice_map.at(old_slice).first;
                auto &slice_hi = split_res.slice_map.at(old_slice).second;
                LOG6("        - erasing " << old_slice);
                slice_it = slice_list->erase(slice_it);  // erase after log
                slice_it = slice_list->insert(slice_it, slice_lo);
                LOG6("        - adding " << slice_lo);
                if (slice_hi) {
                    slice_it++;
                    slice_it = slice_list->insert(slice_it, *slice_hi);
                    LOG6("        - adding " << *slice_hi);
                }
            }
        }
    }

    // Update the slices_to_slice_lists map.
    LOG6("    ...updating slices_to_slice_lists");
    for (auto &kv : split_res.slice_map) {
        // Slices in RotationalClusters but not in slice lists do not need to
        // be updated.
        if (slices_to_slice_lists.find(kv.first) == slices_to_slice_lists.end()) continue;
        slices_to_slice_lists[kv.second.first] = slices_to_slice_lists.at(kv.first);
        if (kv.second.second)
            slices_to_slice_lists[*kv.second.second] = slices_to_slice_lists.at(kv.first);
        slices_to_slice_lists.erase(kv.first);
    }
}

static std::list<PHV::SuperCluster *> merge_same_container_group(
    const std::list<PHV::SuperCluster *> clusters) {
    ordered_map<const PHV::Field *, ordered_set<PHV::SuperCluster *>> same_container_groups;
    UnionFind<PHV::SuperCluster *> cluster_uf;
    for (auto &sc : clusters) {
        bool has_same_container_group = false;
        sc->forall_fieldslices([&](const PHV::FieldSlice &fs) {
            if (fs.field()->same_container_group()) {
                same_container_groups[fs.field()].insert(sc);
                has_same_container_group = true;
            }
        });
        cluster_uf.insert(sc);
    }

    // union by same_container_groups.
    for (const auto &kv : same_container_groups) {
        const auto &clusters = kv.second;
        for (auto itr = clusters.begin(); std::next(itr) != clusters.end(); itr++) {
            cluster_uf.makeUnion(*itr, *std::next(itr));
        }
    }

    std::list<PHV::SuperCluster *> rst;
    for (auto &clusters : cluster_uf) {
        PHV::SuperCluster *merged = clusters.front();
        for (auto itr = std::next(clusters.begin()); itr != clusters.end(); itr++) {
            merged = merged->merge(*itr);
        }
        rst.push_back(merged);
    }
    return rst;
}

// return a list of clusters that participates in wide arithmetic are
// merged and ordered for allocation.
static std::list<PHV::SuperCluster *> merge_wide_arith(const std::list<PHV::SuperCluster *> sc) {
    LOG6("Merge wide arith super clusters");
    std::list<PHV::SuperCluster *> wide;
    std::list<PHV::SuperCluster *> rv;
    for (auto it = sc.begin(); it != sc.end(); ++it) {
        auto cluster = *it;
        if (std::find(wide.begin(), wide.end(), cluster) != wide.end()) {
            continue;
        }  // Already processed this cluster.

        bool used_in_wide_arith = cluster->any_of_fieldslices([&](const PHV::FieldSlice &fs) {
            return fs.field()->bit_used_in_wide_arith(fs.range().lo);
        });
        if (used_in_wide_arith) {
            LOG6("  SC is used in wide_arith:" << cluster);
            // Need to find other cluster that is included with it.
            wide.push_back(cluster);
            auto merged_cluster = cluster;
            for (auto it2 = it; it2 != sc.end(); ++it2) {
                auto cluster2 = *it2;
                bool needToMerge = merged_cluster->needToMergeForWideArith(cluster2);
                if (needToMerge) {
                    LOG6("Need to merge:");
                    LOG6("    SC1: " << merged_cluster);
                    LOG6("    SC2: " << cluster2);
                    merged_cluster = merged_cluster->mergeAndSortBasedOnWideArith(cluster2);
                    LOG6("After merge:");
                    LOG6("    SC1: " << merged_cluster);
                    wide.push_back(cluster2);  // already merged, so don't consider again.
                }
            }
            rv.push_back(merged_cluster);
        } else {
            rv.push_back(cluster);
        }
    }
    return rv;
}

// Returns a new list of SuperClusters, where any SuperCluster that
// (1) participates in wide arithmetic has been merged and ordered for allocation.
// (2) clusters of fieldslices of a same_container_group field will be merged.
static std::list<PHV::SuperCluster *> merge_by_constraints(
    const std::list<PHV::SuperCluster *> sc) {
    return merge_wide_arith(merge_same_container_group(sc));
}

/// Split a SuperCluster with slice lists according to @split_schema.
std::optional<std::list<SuperCluster *>> split(const SuperCluster *sc,
                                               const SplitSchema &split_schemas_input) {
    LOG6(split_schemas_input);

    // mutable copy.
    SplitSchema split_schemas = split_schemas_input;

    //// 1. deep copy states.
    ordered_set<const PHV::RotationalCluster *> new_clusters(sc->clusters());
    // Keep a map of slices to clusters (both old and new for this schema).
    assoc::hash_map<const PHV::FieldSlice, const PHV::RotationalCluster *> slices_to_clusters;
    for (auto *rotational : new_clusters)
        for (auto *aligned : rotational->clusters())
            for (auto &slice : *aligned) slices_to_clusters[slice] = rotational;

    // Deep copy all slice lists, so they can be updated without mutating sc.
    // Update split_schemas to point to the new slice lists, and build a map
    // of slices to new slice lists.
    // Track live RotationalClusters. Clusters that have been split are no
    // longer live.
    ordered_set<PHV::SuperCluster::SliceList *> slice_lists;
    ordered_map<PHV::FieldSlice, ordered_set<PHV::SuperCluster::SliceList *>> slices_to_slice_lists;
    for (auto *old_list : sc->slice_lists()) {
        BUG_CHECK(old_list->size(), "Empty slice list in SuperCluster %1%",
                  cstring::to_cstring(sc));
        // Make new list.
        auto *new_list = new PHV::SuperCluster::SliceList(old_list->begin(), old_list->end());
        slice_lists.insert(new_list);

        // Update split_schema.
        if (split_schemas.count(old_list)) {
            split_schemas[new_list] = split_schemas.at(old_list);
            split_schemas.erase(old_list);
        }
        // Build map.
        for (auto &slice : *new_list) {
            slices_to_slice_lists[slice].insert(new_list);
        }
    }

    //// 2. split.
    // Split each slice list according to its schema.  If a slice is split,
    // then split its RotationalCluster.  Produces a new set of slice lists and
    // rotational clusters.  Fail if a proposed split would violate
    // constraints, like `no_split`.
    for (auto &kv : split_schemas) {
        auto *slice_list = kv.first;
        bitvec split_schema = kv.second;

        // If there are no bits set in the split schema, then no split has been
        // requested.
        if (split_schema.empty()) continue;

        // Remove this list, which will be replaced with new lists.
        slice_lists.erase(slice_list);

        // Iterate through split positions.
        int offset = 0;
        auto *slice_list_lo = new PHV::SuperCluster::SliceList();
        bitvec::nonconst_bitref next_split = split_schema.begin();
        BUG_CHECK(*next_split >= 0, "Trying to split slice list at negative index");
        auto next_slice = slice_list->begin();

        // This loop stutter-steps both `next_slice` and `next_split`.
        while (next_slice != slice_list->end()) {
            auto slice = *next_slice;
            // After processing the last split, just place all remaining slices
            // into slice_list_lo.
            if (next_split == split_schema.end()) {
                slice_list_lo->push_back(slice);
                ++next_slice;
                continue;
            }

            // Otherwise, process slices up to the next split position, then
            // advance the split position.
            if (offset < *next_split && offset + slice.size() <= *next_split) {
                // Slice is completely before the split position.
                LOG6("    ...(" << offset << ") adding to slice list: " << slice);
                slice_list_lo->push_back(slice);
                ++next_slice;
                offset += slice.size();
            } else if (offset == *next_split) {
                // Split position falls between slices.  Advance next_split BUT
                // NOT next_slice.
                LOG6("    ...(" << offset << ") split falls between slices");

                // Check that this position doesn't split adjacent slices of a
                // no_split field.
                if (next_slice != slice_list->begin()) {
                    auto last_it = next_slice;
                    last_it--;
                    if (last_it->field() == next_slice->field() &&
                        next_slice->field()->no_split()) {
                        LOG6("    ...(" << offset
                                        << ") field cannot be split: " << next_slice->field());
                        return std::nullopt;
                    }
                }

                // Otherwise, create new slice list and advance the split position.
                slice_lists.insert(slice_list_lo);
                slice_list_lo = new PHV::SuperCluster::SliceList();
                LOG6("    ...(" << offset << ") starting new slice list");

                // TODO: next_split++ fails to resolve to
                // next_split.operator++().  Not sure why.
                next_split.operator++();
            } else if (offset < *next_split && *next_split < offset + slice.size()) {
                // The split position falls within a slice and will need to be
                // split.  Advance next_split and set next_slice to point to the
                // top half of the post-split subslice.
                LOG6("    ...(" << offset << ") found slice to split at idx " << *next_split << ": "
                                << slice);

                // Split slice.
                auto *rotational = slices_to_clusters.at(slice);
                auto split_result = rotational->slice(*next_split - offset);
                if (!split_result) {
                    LOG6("    ...(" << offset << ") but split failed");
                    return std::nullopt;
                }
                BUG_CHECK(split_result->slice_map.find(slice) != split_result->slice_map.end(),
                          "Bad split schema: slice map does not contain split slice");

                // Update this slice list (which has been
                // removed and is no longer part of slice_lists), taking care to ensure
                // the next_slice iterator is updated to point to the new *lower* subslice.
                for (auto it = slice_list->begin(); it != slice_list->end(); ++it) {
                    auto s = *it;
                    if (split_result->slice_map.find(s) == split_result->slice_map.end()) continue;
                    bool is_this_slice = it == next_slice;
                    // Replace s with its two new subslices.
                    auto &subs = split_result->slice_map.at(s);
                    it = slice_list->erase(it);
                    LOG6("    ...erasing " << s << " in this slice list");
                    it = slice_list->insert(it, subs.first);
                    LOG6("    ...adding " << subs.first << " in this slice list");
                    if (is_this_slice) next_slice = it;
                    if (subs.second) {
                        ++it;
                        it = slice_list->insert(it, *subs.second);
                        LOG6("    ...adding " << *subs.second << " in this slice list");
                    }
                }

                // Advance the iterator to the next slice, which is either the
                // new upper slice (if it exists).
                ++next_slice;

                // Add current list, make new list, advance next_split.
                auto &new_slices = split_result->slice_map.at(slice);
                slice_list_lo->push_back(new_slices.first);
                LOG6("    ...(" << offset << ") adding to slice list: " << slice);
                slice_lists.insert(slice_list_lo);
                slice_list_lo = new PHV::SuperCluster::SliceList();
                LOG6("    ...(" << offset << ") starting new slice list");
                // TODO: next_split++ fails to resolve to
                // next_split.operator++().  Not sure why.
                next_split.operator++();
                offset += new_slices.first.size();

                // Update all slices/clusters in new_clusters,
                // slices_to_clusters, and slice_lists.
                update_slices(rotational, *split_result, new_clusters, slice_lists,
                              slices_to_slice_lists, slices_to_clusters);
            } else {
                // Adding this to ensure the above logic (which is a bit
                // complicated) covers all cases.  Note that *next_split < offset
                // should never be true, as other cases should advance next_split.
                std::stringstream ss;
                for (int x : split_schema) ss << x << " ";
                BUG("Bad split.\nOffset: %3%\nNext split: %4%\nSplit schema: %1%\nSlice list: %2%",
                    ss.str(), cstring::to_cstring(slice_list), offset, *next_split);
            }
        }

        BUG_CHECK(next_split == split_schema.end(),
                  "Slicing schema tries to slice at %1% but slice list is %2%b long", *next_split,
                  offset);

        if (slice_list_lo->size()) slice_lists.insert(slice_list_lo);
    }

    //// 3. union find the result.
    // We need to ensure that all the slice lists and clusters that overlap
    // (i.e. share slices) end up in the same SuperCluster.
    UnionFind<ListClusterPair> uf;
    slices_to_slice_lists.clear();

    // Populate UF universe.
    auto *empty_slice_list = new PHV::SuperCluster::SliceList();
    for (auto *slice_list : slice_lists) {
        for (auto &slice : *slice_list) {
            BUG_CHECK(slices_to_clusters.find(slice) != slices_to_clusters.end(),
                      "No slice to cluster map for %1%", cstring::to_cstring(slice));
            auto *cluster = slices_to_clusters.at(slice);
            uf.insert({slice_list, cluster});
            slices_to_slice_lists[slice].insert(slice_list);
        }
    }
    for (auto *rotational : new_clusters) uf.insert({empty_slice_list, rotational});

    // Union over slice lists.
    for (auto *slice_list : slice_lists) {
        BUG_CHECK(slices_to_clusters.find(slice_list->front()) != slices_to_clusters.end(),
                  "No slice to cluster map for front slice %1%",
                  cstring::to_cstring(slice_list->front()));
        auto first = uf.find({slice_list, slices_to_clusters.at(slice_list->front())});
        for (auto &slice : *slice_list) {
            BUG_CHECK(slices_to_clusters.find(slice) != slices_to_clusters.end(),
                      "No slice to cluster map for slice %1%", cstring::to_cstring(slice));
            uf.makeUnion(first, {slice_list, slices_to_clusters.at(slice)});
        }
    }

    // Union over clusters.
    for (auto *rotational : new_clusters) {
        ListClusterPair first = {empty_slice_list, rotational};
        for (auto *aligned : rotational->clusters()) {
            for (auto &slice : *aligned) {
                for (auto *slice_list : slices_to_slice_lists[slice])
                    uf.makeUnion(first, {slice_list, rotational});
            }
        }
    }

    std::list<PHV::SuperCluster *> rv;
    for (auto &pairs : uf) {
        ordered_set<const PHV::RotationalCluster *> clusters;
        ordered_set<PHV::SuperCluster::SliceList *> slice_lists;
        for (auto &pair : pairs) {
            if (pair.first->size()) slice_lists.insert(pair.first);
            clusters.insert(pair.second);
        }
        rv.push_back(new PHV::SuperCluster(std::move(clusters), std::move(slice_lists)));
    }

    // 4. merge clusters by constraints.
    return merge_by_constraints(rv);
}

/// Split the RotationalCluster in a SuperCluster without a slice list
/// according to @split_schema.
std::optional<std::list<PHV::SuperCluster *>> split_rotational_cluster(const PHV::SuperCluster *sc,
                                                                       bitvec split_schema,
                                                                       int max_aligment) {
    // This method cannot handle super clusters with slice lists.
    if (sc->slice_lists().size() > 0) return std::nullopt;

    BUG_CHECK(sc->clusters().size() != 0, "SuperCluster with no RotationalClusters: %1%",
              cstring::to_cstring(sc));
    BUG_CHECK(sc->clusters().size() == 1,
              "SuperCluster with no slice lists but more than one RotationalCluster: %1%",
              cstring::to_cstring(sc));

    // An empty split schema means no split is necessary.
    if (split_schema.empty())
        return std::list<PHV::SuperCluster *>(
            {new PHV::SuperCluster(sc->clusters(), sc->slice_lists())});

    // Otherwise, if this SuperCluster doesn't have any slice lists, then slice
    // the rotational clusters directly.
    std::list<PHV::SuperCluster *> rv;
    auto *remainder = *sc->clusters().begin();
    int offset = max_aligment;
    for (int next_split : split_schema) {
        BUG_CHECK(next_split >= 0, "Trying to split remainder cluster at negative index");
        auto res = remainder->slice(next_split - offset);
        if (!res) return std::nullopt;
        offset = next_split;
        rv.push_back(new PHV::SuperCluster({res->lo}, {}));
        remainder = res->hi;
    }

    rv.push_back(new PHV::SuperCluster({remainder}, {}));
    return rv;
}

}  // namespace Slicing
}  // namespace PHV

// Helper for split_super_cluster;
std::ostream &operator<<(std::ostream &out, const PHV::Slicing::ListClusterPair &pair) {
    out << std::endl;
    out << "(    " << pair.first << std::endl;
    out << ",    " << pair.second << "    )";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::Slicing::ListClusterPair *pair) {
    if (pair)
        out << *pair;
    else
        out << "-null-listclusterpair-";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::Slicing::SplitSchema &schema) {
    out << "Bit schema:\n";
    for (auto &kv : schema) {
        int size = 0;
        for (auto slice : *kv.first) {
            size += slice.size();
        }
        std::stringstream xx;
        for (int idx = 0; idx < size; ++idx) {
            xx << (kv.second[idx] ? "1" : "-");
        }
        out << "    " << xx.str() << "\n";
        out << "    " << kv.first << "\n";
    }
    out << "\n";
    return out;
}
