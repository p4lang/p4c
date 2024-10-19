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

#include "bf-p4c/phv/slicing/phv_slicing_dfs_iterator.h"

#include <algorithm>
#include <climits>
#include <iterator>
#include <numeric>
#include <queue>
#include <sstream>

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/logging/logging.h"
#include "bf-p4c/phv/error.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "lib/algorithm.h"
#include "lib/bitvec.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/range.h"

namespace {

/// DeferHelper will call @p defer on leaving scope.
struct DeferHelper {
 public:
    std::function<void()> defer;
    explicit DeferHelper(std::function<void()> defer) : defer(defer) {}
    virtual ~DeferHelper() { defer(); }
};

bool overlapped(const PHV::SuperCluster::SliceList *a, const PHV::SuperCluster::SliceList *b) {
    for (const auto &fs_a : *a) {
        for (const auto &fs_b : *b) {
            if (fs_a.field() == fs_b.field() && fs_a.range().overlaps(fs_b.range())) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

// SuperCluster with no slicelist has a different but much simpler iterator.
// Since there is no slice list, that means, there is only one rotational
// cluster in the super cluster. We only need to iterate all possible slicing
// by increasing the a bitvec representation of the cluster, where the n-th
// bit is one indicates "split at 8*(n+1)-th bit on the rotation cluster".
// Note that when there was pre-alignment of any field in the rotational cluster
// the nth bit will take max(pre-alignment) into account, See below for details.
namespace PHV {
namespace Slicing {
namespace Internal {

// inc @p bv by 1.
void inc(bitvec &bv) {
    int i = 0;
    for (; i <= (1 << 30); ++i) {
        if (bv.getbit(i)) {
            bv.clrbit(i);
        } else {
            break;
        }
    }
    bv.setbit(i);
}

// all_well_formed return true if all clusters are well-formed.
bool all_well_formed(const std::list<SuperCluster *> &clusters) {
    return std::all_of(clusters.begin(), clusters.end(),
                       [](const SuperCluster *s) { return SuperCluster::is_well_formed(s); });
}

// all_container_sized return true if all clusters are well-formed.
bool all_container_sized(const std::list<SuperCluster *> &clusters) {
    return std::all_of(clusters.begin(), clusters.end(), [](const SuperCluster *s) {
        for (const auto &sl : s->slice_lists()) {
            if (!sl->front().field()->exact_containers()) continue;
            const int sz = SuperCluster::slice_list_total_bits(*sl);
            if (sz != 8 && sz != 16 && sz != 32) return false;
        }
        return true;
    });
}

// Returns a bitvec that there are no more than 3 consecutive 0s, by
// setting 1 on any "0000" to "0001" from higher to lower bits.
// It enforce less or equal to 32b constraints on the bitvec.
bitvec enforce_le_32b(const bitvec &bv, int sentinel) {
    bitvec rst = bv;
    int zeroes = 0;
    for (int i = sentinel - 1; i >= 0; --i) {
        if (rst[i]) {
            zeroes = 0;
        } else {
            zeroes++;
            if (zeroes > 3) {
                rst.setbit(i);
                zeroes = 0;
            }
        }
    }
    return rst;
}

// a simple iterator that can only be used if @p sc does not have any slicelist.
// it iterate all possible slicing by increasing the a bitvec representation of
// slicing the cluster, where the n-th bit is one indicates
// "split at 8*(n+1)-th bit on the rotation cluster".
void no_slicelist_itr(const IterateCb &yield, const SuperCluster *sc) {
    int max_aligment = 0;
    sc->forall_fieldslices([&max_aligment](const FieldSlice &fs) {
        const int alignment = fs.alignment() ? (*fs.alignment()).align : 0;
        max_aligment = std::max(max_aligment, alignment);
    });
    const int max_width = sc->max_width() + max_aligment;
    int sentinel_idx = max_width / 8 - (1 - bool(max_width % 8));
    if (sentinel_idx < 0) {
        return;
    }
    bitvec compressed_schema;
    while (!compressed_schema[sentinel_idx]) {
        // Expand the compressed schema.
        bitvec split_schema;
        for (int i : enforce_le_32b(compressed_schema, sentinel_idx)) {
            split_schema.setbit((i + 1) * 8);
        }
        // Split the supercluster.
        auto res = split_rotational_cluster(sc, split_schema, max_aligment);
        // If successful, return it.
        if (res && all_well_formed(*res) && all_container_sized(*res)) {
            if (!yield(*res)) {
                return;
            }
        }
        inc(compressed_schema);
    }
}

void stride_cluster_itr(const IterateCb &yield, const SuperCluster *sc) {
    BUG_CHECK(!sc->slice_lists().empty(), "empty slice lists stride cluster: %1%", sc);
    int max_width = 0;
    for (const auto *sl : sc->slice_lists()) {
        const auto &head = sl->front();
        const int alignment = head.alignment() ? (*head.alignment()).align : 0;
        max_width = std::max(max_width, alignment + SuperCluster::slice_list_total_bits(*sl));
    }
    int sentinel_idx = max_width / 8 - (1 - bool(max_width % 8));
    if (sentinel_idx < 0) {
        return;
    }
    bitvec compressed_schema;
    while (!compressed_schema[sentinel_idx]) {
        // Expand the compressed schema.
        bitvec split_schema;
        for (int i : enforce_le_32b(compressed_schema, sentinel_idx)) {
            split_schema.setbit((i + 1) * 8);
        }
        SplitSchema sc_schema;
        for (const auto &sl : sc->slice_lists()) {
            sc_schema[sl] = split_schema;
        }
        // Split all slice lists in the same way.
        auto res = split(sc, sc_schema);
        // If successful, return it.
        if (res && all_well_formed(*res) && all_container_sized(*res)) {
            if (!yield(*res)) {
                return;
            }
        }
        inc(compressed_schema);
    }
}

struct ScSubRangeFsFinder {
    SuperCluster *sc;
    assoc::hash_map<const Field *, ordered_set<FieldSlice>> field_slices;
    explicit ScSubRangeFsFinder(SuperCluster *sc) : sc(sc) {
        sc->forall_fieldslices([&](const FieldSlice &fs) { field_slices[fs.field()].insert(fs); });
    }

    /// @returns the field slice in @a sc that covers @p fs.
    std::optional<FieldSlice> get_enclosing_fs(const FieldSlice &fs) {
        for (const auto &enclosing_fs : field_slices.at(fs.field())) {
            if (enclosing_fs.range().contains(fs.range())) {
                return enclosing_fs;
            }
        }
        return std::nullopt;
    }

    /// @returns rotational slices of @p fs in @a sc.
    std::vector<FieldSlice> get_rotational_slices(const FieldSlice &fs) {
        std::vector<FieldSlice> rst;
        auto enclosing = get_enclosing_fs(fs);
        if (!enclosing) return rst;
        const int offset = fs.range().lo - enclosing->range().lo;
        for (const auto &other : sc->cluster(*enclosing).slices()) {
            rst.push_back(PHV::FieldSlice(other.field(),
                                          StartLen(other.range().lo + offset, fs.range().size())));
        }
        return rst;
    }
};

}  // namespace Internal
}  // namespace Slicing
}  // namespace PHV

namespace PHV {
namespace Slicing {

// return the alignment of the first fieldslice for non exact containers lists.
int compute_pre_alignment(const PHV::SuperCluster::SliceList &sl) {
    if (sl.empty()) {
        return 0;
    }
    const auto &head = sl.front();
    return head.alignment() ? (*head.alignment()).align : 0;
}

// return a std::map<int, FieldSlice> that maps i to the fieldslice on the i-th bit of @p sl.
assoc::hash_map<int, FieldSlice> make_fs_bitmap(const SuperCluster::SliceList *sl) {
    assoc::hash_map<int, FieldSlice> rst;
    int offset = 0;
    for (const auto &fs : *sl) {
        for (int i = 0; i < fs.size(); i++) {
            rst[offset + i] = fs;
        }
        offset += fs.size();
    }
    return rst;
}

// can_satisfy_valid_container_range will return false for @p sc if its
// valid container range is smaller than [0..(previous_bits_in_slicelist + fs.size)]
// For example, for this cluster
// SUPERCLUSTER Uid: 1
//     slice lists:
//         [ f1<32> ^0 ^bit[0..23] meta solitary mocha [0:23]
//           f1<32> ^0 ^bit[0..7] meta solitary mocha [24:31] ]
// we need to split out the second slice because it's valid container range is [0..7],
// because it can only be allocated to the first 8 bits of a container, but, it is not
// possible within this slice list, because of the first slice.
// This case is usually seen when a metadata field is extracted in parser from differently
// sized header fields:
// parser_state1 {
//   f1<32> = (bit<32>)f2<24>;
// }
// parser_state2 {
//   f1<32> = f2<32>;
// }
bool can_satisfy_valid_container_range(const SuperCluster::SliceList *sl) {
    int n_preceding_bits = 0;
    // slicelists are in little-endian, but valid container ranges are network order,
    // i.e., big-endian, so we need to reversely iterate.
    for (const auto &fs : boost::adaptors::reverse(*sl)) {
        if (!fs.validContainerRange().contains(StartLen(n_preceding_bits, fs.size()))) {
            return false;
        }
        n_preceding_bits += fs.size();
    }
    return true;
}

/// PreSplitFunc is a function type that split a supercluter into a set of clusters if possible,
/// otherwise, it will return std::nullopt.
using PreSplitFunc = std::function<std::optional<std::list<SuperCluster *>>(SuperCluster *)>;

/// helper function to presplit clusters.
std::optional<ordered_set<SuperCluster *>> presplit_by(const ordered_set<SuperCluster *> &clusters,
                                                       PreSplitFunc split_func,
                                                       cstring split_name) {
    ordered_set<SuperCluster *> rst;
    for (auto *sc : clusters) {
        const auto after = split_func(sc);
        if (!after) {
            return std::nullopt;
        }
        rst.insert(after->begin(), after->end());
    }
    if (LOGGING(1)) {
        LOG1("after pre-split by " << split_name << ":");
        for (const auto &sc : rst) {
            LOG1(sc);
        }
    }
    return rst;
}

const bitvec AfterSplitConstraint::all_container_sizes =
    bitvec((1 << 8) | (1 << 16) | (1llu << 32));

AfterSplitConstraint::AfterSplitConstraint(const bitvec &sizes) : sizes(sizes) {
    BUG_CHECK(!sizes.empty(), "empty afterSplitConstraint is not allowed");
    for (const auto &v : sizes) {
        BUG_CHECK(all_container_sizes.getbit(v), "invalid container size: %1%", v);
    }
}

AfterSplitConstraint::AfterSplitConstraint(ConstraintType t, int v) {
    using ctype = AfterSplitConstraint::ConstraintType;
    if (t == ctype::NONE) {
        sizes = all_container_sizes;
    } else if (t == ctype::EXACT) {
        BUG_CHECK(all_container_sizes.getbit(v), "invalid container size: %1%", v);
        sizes = bitvec(1llu << v);
    } else if (t == ctype::MIN) {
        BUG_CHECK(all_container_sizes.getbit(v), "invalid container size: %1%", v);
        for (const auto &s : all_container_sizes) {
            if (s < v) continue;
            sizes.setbit(s);
        }
    } else {
        BUG("unknown type: %1%", (int)t);
    }
}

AfterSplitConstraint::ConstraintType AfterSplitConstraint::type() const {
    using ctype = AfterSplitConstraint::ConstraintType;
    if (sizes.popcount() == all_container_sizes.popcount()) {
        return ctype::NONE;
    } else if (sizes.popcount() == 1) {
        return ctype::EXACT;
    } else {
        return ctype::MIN;
    }
}

std::optional<AfterSplitConstraint> AfterSplitConstraint::intersect(
    const AfterSplitConstraint &other) const {
    bitvec rv = sizes & other.sizes;
    if (rv.empty()) {
        return std::nullopt;
    }
    return AfterSplitConstraint(rv);
}

bool AfterSplitConstraint::ok(const int n) const { return sizes.getbit(n); }

// NextSplitChoiceMetrics is used during DFS to sort best slicing choice
// from [8, 16, 32].
struct NextSplitChoiceMetrics {
    bool will_create_new_split = false;
    bool will_split_unreferenced = false;
    int n_avoidable_packings = 0;
    SplitChoice size;

    NextSplitChoiceMetrics(bool will_create_new_split, bool split_unreferenced,
                           int n_avoidable_packings, SplitChoice size)
        : will_create_new_split(will_create_new_split),
          will_split_unreferenced(split_unreferenced),
          n_avoidable_packings(n_avoidable_packings),
          size(size) {}

    // By default, we prefer to sort them by
    // 1.  try to split at field boundaries. This one implicitly work with to_invalidate
    //     because if we are trying not to split the field in the middle, and we
    //     still meet some packing conflicts, then we should backtrack to avoid
    //     the packing.
    // 2.0 try split between referenced and non-referenced field.
    // 2.1 (TODO) container sizes that are more abundant. It allows allocation
    //     algorithm to balance between containers. More importantly, at the end
    //     of allocation, we can prune a lot of invalid slicing. We need to add this
    //     if pre-slicing is removed in allocation algorithm.
    // 2.2 (TODO) except the above, for metadata slicelist, an important case is
    //     that metadata fieldslices may be better to be sliced in the same way
    //     as the header fields in the its rotational cluster.
    // 3.  larger container to encourage more packings.
    static bool default_heuristics(const NextSplitChoiceMetrics &a,
                                   const NextSplitChoiceMetrics &b) {
        if (a.will_create_new_split != b.will_create_new_split) {
            return a.will_create_new_split < b.will_create_new_split;
        } else if (a.will_split_unreferenced != b.will_split_unreferenced) {
            return a.will_split_unreferenced > b.will_split_unreferenced;
        } else {
            return a.size > b.size;
        }
    }

    /// minimal packing choices that
    /// 1.  try to split with minimal avoidable packings.
    /// 2.  otherwise same as default.
    static bool minimal_packing(const NextSplitChoiceMetrics &a, const NextSplitChoiceMetrics &b) {
        // Use n_avoidable_packings to determinate how many fieldslices will
        // be packed after split by the choice.
        // For example
        // [ a<18> [0:17],  b<14> [0:13]]
        // in terms of packing, it is the same to either split first 16 or 32:
        // + split at 16: pack a[17:17] with b;
        // + split at 32: pack a[0:17] with b;
        // we would prefer (firstly try) to split at 32 to create less fragments.
        const int a_packings = a.n_avoidable_packings;
        const int b_packings = b.n_avoidable_packings;
        if (a_packings != b_packings) {
            return a_packings < b_packings;
        } else {
            return default_heuristics(a, b);
        }
    }
};

std::ostream &operator<<(std::ostream &out, const NextSplitChoiceMetrics &c) {
    out << "{" << "new_split: " << c.will_create_new_split << ", "
        << "split_unref:" << c.will_split_unreferenced << ", "
        << "n_avoidable_packings:" << c.n_avoidable_packings << ", " << "size: " << int(c.size)
        << "}";
    return out;
}

std::vector<SplitChoice> DfsItrContext::make_choices(const SliceListLoc &target) const {
    static const std::vector<SplitChoice> all_choices = {
        SplitChoice::B,
        SplitChoice::H,
        SplitChoice::W,
    };
    const bool has_exact_containers = SuperCluster::slice_list_has_exact_containers(*target.second);
    const int pre_alignment = compute_pre_alignment(*target.second);
    // for metadata field slicelist, we must add the alignment to the slicelist.
    const int sl_sz = SuperCluster::slice_list_total_bits(*target.second) + pre_alignment;

    // marks 1 if that bit is in the middle of a fieldslice.
    bitvec middle_of_fieldslices;
    int offset = pre_alignment;
    for (const auto &fs : *target.second) {
        middle_of_fieldslices.setrange(offset + 1, fs.size() - 1);
        offset += fs.size();
        if (offset > 32) break;
    }

    // marks 1 if that bit is at a boundary of reference and unreferenced field,
    // that split [head...][rest...] into referenced list and unreferenced list.
    bitvec unreferenced_boundary;
    offset = pre_alignment;
    const bool is_head_referenced = is_used_i(target.second->front().field());
    for (const auto &fs : *target.second) {
        if (is_used_i(fs.field()) != is_head_referenced) {
            unreferenced_boundary.setbit(offset);
            break;
        }
        offset += fs.size();
        if (offset > 32) break;
    }

    // the number of pairs of fieldslices that are not necessarily to be packed.
    assoc::hash_map<SplitChoice, int> n_avoidable_packings;
    const auto update_n_avoidable_packings = [&](int lo, int hi, int v) {
        if (lo > hi) return;
        for (const auto &c : all_choices) {
            if (int(c) >= lo && int(c) <= hi) {
                n_avoidable_packings[c] = v;
            }
        }
    };
    // not necessary to set indexes before pre_alignment
    // because max alignment is 7, while smallest split is 8.
    offset = pre_alignment;
    int total_avoidable = 0;
    for (auto fs = target.second->begin(); fs != target.second->end(); fs++) {
        for (auto prev_fs = target.second->begin(); prev_fs != fs; prev_fs++) {
            total_avoidable += int(is_used_i(fs->field()) && is_used_i(prev_fs->field()) &&
                                   !phv_i.must_alloc_same_container(*fs, *prev_fs));
        }
        update_n_avoidable_packings(offset + 1, offset + fs->size(), total_avoidable);
        offset += fs->size();
        if (offset > 32) break;
    }
    update_n_avoidable_packings(offset + 1, 32, total_avoidable);

    bool has_tried_larger_sz = false;
    std::vector<NextSplitChoiceMetrics> choices;
    for (const auto &c : all_choices) {
        // exact container slice list cannot be allocated to larger containers.
        if (int(c) > sl_sz && has_exact_containers) {
            continue;
        }
        // do not add duplicated choices for non_exact_container slice lists.
        // e.g. if the sl is 16-bit-long, then split at 16 is the same as at 32.
        if (int(c) >= sl_sz && !has_exact_containers) {
            if (has_tried_larger_sz) {
                continue;
            } else {
                has_tried_larger_sz = true;
            }
        }
        choices.push_back({
            bool(middle_of_fieldslices[int(c)]),
            bool(unreferenced_boundary[int(c)]),
            n_avoidable_packings.at(c),
            c,
        });
    }

    if (LOGGING(5)) {
        LOG5("possible slice choices: ");
        for (const auto &c : choices) {
            LOG5(c);
        }
    }
    if (config_i.minimal_packing_mode) {
        std::sort(choices.begin(), choices.end(), NextSplitChoiceMetrics::minimal_packing);
    } else {
        std::sort(choices.begin(), choices.end(), NextSplitChoiceMetrics::default_heuristics);
    }

    // convert back to simple vector<SplitChoice>.
    std::vector<SplitChoice> rst;
    for (const auto &v : choices) {
        rst.push_back(v.size);
    }
    return rst;
}

// NextSplitTargetMetrics is used during DFS for picking the next. It's
// extremely important to pick the one with most constraints to form a
// smaller search tree, in a same spirit of algorithm X,
// https://en.wikipedia.org/wiki/Knuth%27s_Algorithm_X
struct NextSplitTargetMetrics {
    /// the next byte index that the fieldslice's container size
    /// has actually been constrained to only 1 option.
    /// If none of fieldslice has decided size, INT_MAX.
    int next_size_decided_byte_index = INT_MAX;
    int size = 0;
    bool has_exact_container = false;
    int n_after_split_constraints = 0;

    // pick slice list with exact container, closer to a size-decided byte,
    /// more constraints and has smaller size as next target to form a smaller search tree.
    bool operator>(const NextSplitTargetMetrics &other) {
        if (has_exact_container != other.has_exact_container) {
            return has_exact_container > other.has_exact_container;
        } else if (next_size_decided_byte_index != other.next_size_decided_byte_index) {
            return next_size_decided_byte_index < other.next_size_decided_byte_index;
        } else if (size != other.size) {
            return size < other.size;
        } else {
            return n_after_split_constraints > other.n_after_split_constraints;
        }
    }
};

std::optional<SliceListLoc> DfsItrContext::dfs_pick_next() const {
    using ctype = AfterSplitConstraint::ConstraintType;
    std::optional<SliceListLoc> rst = std::nullopt;
    NextSplitTargetMetrics best;

    for (auto *sc : to_be_split_i) {
        // Unfortunately using after_split_constraints here implies that
        // fixes and improvement of after_split constraints will
        // change the order of searching, which may have a butterfly effect
        // on slicing and allocation in some cases.
        auto after_split_constraints = collect_aftersplit_constraints(sc);
        if (!after_split_constraints) {
            return std::nullopt;
        }
        const auto next_decided_byte = [&](const SuperCluster::SliceList *sl) {
            int offset = 0;
            for (const auto &fs : *sl) {
                if (after_split_constraints->count(fs) &&
                    after_split_constraints->at(fs).type() == ctype::EXACT) {
                    return offset / 8;
                }
            }
            return INT_MAX;
        };

        for (auto *sl : sc->slice_lists()) {
            if (!need_further_split(sl)) {
                continue;
            }
            NextSplitTargetMetrics curr;
            curr.size = SuperCluster::slice_list_total_bits(*sl);
            curr.has_exact_container = sl->front().field()->exact_containers();
            if (config_i.smart_slicing) {
                curr.next_size_decided_byte_index = next_decided_byte(sl);
            }
            for (const auto &fs : *sl) {
                if ((*after_split_constraints).count(fs) &&
                    (*after_split_constraints).at(fs).type() !=
                        AfterSplitConstraint::ConstraintType::NONE) {
                    curr.n_after_split_constraints++;
                }
            }
            if (!rst || curr > best) {
                rst = make_pair(sc, sl);
                best = curr;
            }
        }
    }
    return rst;
}

void DfsItrContext::propagate_8bit_exact_container_split(SuperCluster *sc,
                                                         SuperCluster::SliceList *target,
                                                         SplitSchema *schema,
                                                         SplitDecision *decisions) const {
    if (!SuperCluster::slice_list_has_exact_containers(*target)) {
        return;
    }

    ordered_map<SuperCluster::SliceList *, std::vector<SuperCluster::SliceList *>> sl_to_byte_lists;
    assoc::hash_map<const Field *, ordered_map<FieldSlice, SuperCluster::SliceList *>>
        field_to_byte_lists;
    for (auto *sl : sc->slice_lists()) {
        if (!need_further_split(sl)) continue;
        sl_to_byte_lists[sl] = SuperCluster::slice_list_split_by_byte(*sl);
        for (auto *byte_sl : sl_to_byte_lists[sl]) {
            for (const auto &fs : *byte_sl) {
                field_to_byte_lists[fs.field()][fs] = byte_sl;
            }
        }
    }
    const auto get_byte_lists = [&](const FieldSlice &fs) {
        std::vector<SuperCluster::SliceList *> rst;
        if (!field_to_byte_lists.count(fs.field())) return rst;
        for (const auto &fs_list : field_to_byte_lists.at(fs.field())) {
            if (fs_list.first.range().overlaps(fs.range())) {
                rst.push_back(fs_list.second);
            }
        }
        return rst;
    };

    // propagate 8bit slice to all other byte slices.
    Internal::ScSubRangeFsFinder fs_query(sc);
    std::queue<SuperCluster::SliceList *> queue;
    assoc::hash_set<SuperCluster::SliceList *> visited;
    const auto push_new_byte_list = [&](SuperCluster::SliceList *sl) {
        if (visited.count(sl)) {
            return;
        }
        visited.insert(sl);
        queue.push(sl);
    };
    push_new_byte_list(sl_to_byte_lists.at(target).front());
    while (!queue.empty()) {
        const auto *byte_list = queue.front();
        queue.pop();
        for (const auto &fs : *byte_list) {
            auto rot_slices = fs_query.get_rotational_slices(fs);
            for (const auto &rot_fs : rot_slices) {
                auto rot_byte_lists = get_byte_lists(rot_fs);
                for (auto *rot_byte_list : rot_byte_lists) {
                    push_new_byte_list(rot_byte_list);
                }
            }
        }
    }

    // mark split and add additional decisions.
    for (const auto &sl_byte_lists : sl_to_byte_lists) {
        auto *sl = sl_byte_lists.first;
        const auto &byte_lists = sl_byte_lists.second;
        const int pre_alignment = compute_pre_alignment(*sl);
        const int sl_sz = SuperCluster::slice_list_total_bits(*sl);
        int byte_idx = 0;
        for (auto *byte_list : byte_lists) {
            if (visited.count(byte_list)) {
                if (8 * byte_idx - pre_alignment > 0) {
                    schema->at(sl).setbit(8 * byte_idx - pre_alignment);
                }
                if (8 * (byte_idx + 1) - pre_alignment < sl_sz) {
                    schema->at(sl).setbit(8 * (byte_idx + 1) - pre_alignment);
                }
                for (const auto &fs : *byte_list) {
                    decisions->emplace(
                        fs, AfterSplitConstraint(AfterSplitConstraint::ConstraintType::EXACT, 8));
                }
            }
            byte_idx++;
        }
    }
}

bool DfsItrContext::propagate_tail_split(SuperCluster *sc, const SplitDecision &constraints,
                                         const SplitDecision *decisions,
                                         const SuperCluster::SliceList *just_split_target,
                                         const int n_just_split_bits, SplitSchema *schema) const {
    /// returns decision of @p fs made by this round and all previous rounds.
    const auto get_decision = [&](const FieldSlice &fs, bool &found_conflict) {
        std::optional<AfterSplitConstraint> rst =
            !constraints.count(fs) ? std::nullopt : std::make_optional(constraints.at(fs));
        // only new decisions will have immaterial changes that we need to do range match.
        for (const auto &new_decision : *decisions) {
            const auto &decided_fs = new_decision.first;
            const auto &decided_sz = new_decision.second;
            if (decided_fs.field() == fs.field() && decided_fs.range().overlaps(fs.range())) {
                rst = !rst ? decided_sz : rst->intersect(decided_sz);
                if (!rst) {
                    found_conflict = true;
                    return rst;
                }
            }
        }
        if (split_decisions_i.count(fs)) {
            rst = !rst ? split_decisions_i.at(fs) : rst->intersect(split_decisions_i.at(fs));
            if (!rst) {
                found_conflict = true;
                return rst;
            }
        }
        return rst;
    };
    using ctype = AfterSplitConstraint::ConstraintType;
    Internal::ScSubRangeFsFinder fs_finder(sc);
    const auto get_decision_based_on_rot = [&](const FieldSlice &fs, bool &found_conflict) {
        std::optional<AfterSplitConstraint> rst = std::nullopt;
        for (const auto &rot_fs : fs_finder.get_rotational_slices(fs)) {
            auto rot_decided_sz = get_decision(rot_fs, found_conflict);
            if (found_conflict) {
                return rst;
            }
            if (!rot_decided_sz) continue;
            if (!rst) {
                rst = rot_decided_sz;
            } else {
                rst = rst->intersect(*rot_decided_sz);
                if (!rst) {
                    found_conflict = true;
                    return rst;
                }
            }
        }
        return rst;
    };
    for (auto *sl : sc->slice_lists()) {
        if (!need_further_split(sl)) continue;
        if (!SuperCluster::slice_list_has_exact_containers(*sl)) continue;
        const int sl_sz = SuperCluster::slice_list_total_bits(*sl);
        const int pre_alignment = compute_pre_alignment(*sl);
        const auto byte_lists = SuperCluster::slice_list_split_by_byte(*sl);
        int tail_idx = byte_lists.size() - 1;
        while (tail_idx >= 0) {
            // edge case: when the tail is part of the newly-made decision, skip.
            if (sl == just_split_target && tail_idx <= (n_just_split_bits / 8) - 1) {
                break;
            }
            bool tail_cut = false;
            const auto *tail_byte = byte_lists[tail_idx];
            for (const auto &fs : *tail_byte) {
                if (!fs.field()->exact_containers()) continue;
                bool found_conflict = false;
                const auto decided_sz = get_decision_based_on_rot(fs, found_conflict);
                if (found_conflict) {
                    LOG5("found conflicting size decisions @tail when checking " << fs);
                    return false;
                }
                if (!decided_sz || decided_sz->type() != ctype::EXACT) continue;
                const int to_split_sz = decided_sz->min();
                const int split_offset =
                    sl_sz - (byte_lists.size() - 1 - tail_idx) * 8 - to_split_sz - pre_alignment;
                if (split_offset <= 0) continue;
                /// Do not split cross the bytes that we just split out. If incorrect, it
                /// will fail very soon later.
                if (sl == just_split_target && split_offset < n_just_split_bits) {
                    continue;
                }
                schema->at(sl).setbit(split_offset);
                LOG5("@tail optimization found, because " << fs << " must be " << *decided_sz
                                                          << ", we split @" << split_offset
                                                          << " for " << sl);
                tail_idx -= to_split_sz / 8;
                tail_cut = true;
                break;
            }
            if (!tail_cut) break;
        }
    }
    return true;
}

std::optional<std::pair<SplitSchema, SplitDecision>> DfsItrContext::make_split_meta(
    SuperCluster *sc, SuperCluster::SliceList *target, int first_n_bits) const {
    using ctype = AfterSplitConstraint::ConstraintType;
    auto after_split_constraints = collect_aftersplit_constraints(sc);
    BUG_CHECK(after_split_constraints, "conflicting constraints should have been pruned");
    SplitSchema schema;
    SplitDecision decisions;
    const bool has_exact_conts = SuperCluster::slice_list_has_exact_containers(*target);
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        if (sl != target) {
            continue;
        }

        const int sz = SuperCluster::slice_list_total_bits(*sl);
        // For metadata fields with alignment, we should treat pre_alignment as virtual padding
        // before the field by setting the initial offset to the alignment, e.g.
        // meta<8> with alignment 3 cannot be allocated to 8-bit container, and the initial
        // offset will be 3.
        const int pre_alignment = compute_pre_alignment(*sl);
        if (pre_alignment + sz > first_n_bits) {
            schema[sl].setbit(first_n_bits - pre_alignment);
        }

        AfterSplitConstraint container_size_constraint(ctype::EXACT, first_n_bits);
        // If slice list does not have exact containers, then split this slicelist
        // introduce only the minimum container size constraint,
        // because they can be flexibly allocated to any larger or equal container.
        if (!has_exact_conts) {
            container_size_constraint = AfterSplitConstraint(ctype::MIN, first_n_bits);
        }

        int offset = pre_alignment;
        for (const auto &fs : *sl) {
            if (offset >= first_n_bits) {
                break;
            }
            if (after_split_constraints->count(fs) &&
                !after_split_constraints->at(fs).intersect(container_size_constraint)) {
                LOG5("cannot split " << fs << " to " << container_size_constraint
                                     << ", because it must be " << after_split_constraints->at(fs));
                return std::nullopt;
            }
            if (offset + fs.size() <= first_n_bits) {
                decisions[fs] = container_size_constraint;
            } else {
                const int length = first_n_bits - offset;
                auto partial_fs = FieldSlice(fs, StartLen(fs.range().lo, length));
                decisions[partial_fs] = container_size_constraint;

                // Needs to update slices that has already been split.
                for (const auto &other : sc->cluster(fs).slices()) {
                    if (split_decisions_i.count(other)) {
                        const auto head = FieldSlice(other, StartLen(other.range().lo, length));
                        const auto tail = FieldSlice(
                            other, StartLen(other.range().lo + length, other.size() - length));
                        decisions[head] = split_decisions_i.at(other);
                        decisions[tail] = split_decisions_i.at(other);
                        LOG6("fs update " << other << " into " << head << " and " << tail
                                          << " because " << fs << " is split into " << partial_fs);
                    }
                }
            }
            offset += fs.size();
        }
    }
    if (first_n_bits == 8) {
        propagate_8bit_exact_container_split(sc, target, &schema, &decisions);
    }
    if (config_i.smart_slicing) {
        if (!propagate_tail_split(sc, *after_split_constraints, &decisions, target, first_n_bits,
                                  &schema)) {
            return std::nullopt;
        }
    }
    return std::make_pair(schema, decisions);
}

std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_adjacent_no_pack(
    SuperCluster *sc) const {
    // group fieldslices by the byte it locates, and then check no_pack constraint
    // between adjacent byte groups. If there are no_pack constraints, mark split
    // at the byte boundary.
    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        auto fs_bitmap = make_fs_bitmap(sl);
        const int n_bits = SuperCluster::slice_list_total_bits(*sl);
        const int n_bytes = n_bits / 8 + int(n_bits % 8 != 0);
        std::vector<ordered_set<PHV::FieldSlice>> group_by_byte(n_bytes);
        for (int i = 0; i < n_bytes; i++) {
            // group fieldslices by the byte they are in.
            for (int bit = i * 8; bit < (i + 1) * 8; bit++) {
                if (fs_bitmap.count(bit)) {
                    group_by_byte[i].insert(fs_bitmap.at(bit));
                }
            }
            if (i > 0) {
                for (const auto &fi : group_by_byte[i - 1]) {
                    for (const auto &fj : group_by_byte[i]) {
                        if (fi != fj && !group_by_byte[i - 1].count(fj) &&
                            !group_by_byte[i].count(fi) && has_pack_conflict_i(fi, fj) &&
                            !phv_i.must_alloc_same_container(fi, fj)) {
                            schema[sl].setbit(i * 8);
                            LOG5("set split after " << i * 8 << "th bit because no_pack between "
                                                    << fi << " and " << fj);
                            break;
                        }
                    }
                    if (schema[sl].getbit(i * 8)) {
                        break;
                    }
                }
            }
        }
    }
    LOG5("split_by_adjacent_no_pack schema: " << schema);
    return PHV::Slicing::split(sc, schema);
}

std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_adjacent_deparsed_and_non_deparsed(
    SuperCluster *sc) const {
    const auto is_deparsed_or_digest = [](const FieldSlice &fs) {
        return (fs.field()->deparsed() || fs.field()->exact_containers() ||
                fs.field()->is_digest());
    };

    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        int offset = 0;
        auto itr = sl->begin();
        while (itr != sl->end()) {
            auto next = std::next(itr);
            if (next == sl->end()) {
                break;
            }
            if (is_deparsed_or_digest(*itr) != is_deparsed_or_digest(*next)) {
                schema[sl].setbit(offset + itr->size());
            }
            offset += itr->size();
            itr = next;
        }
    }
    LOG5("split_by_adjacent_deparsed_and_non_deparsed schema: " << schema);
    return PHV::Slicing::split(sc, schema);
}

std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_deparsed_bottom_bits(
    SuperCluster *sc) const {
    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        int offset = 0;
        auto itr = sl->begin();
        while (itr != sl->end()) {
            auto next = std::next(itr);
            if (next == sl->end()) {
                break;
            }
            if (itr->field()->deparsed_bottom_bits() && itr->range().lo == 0 && offset != 0) {
                schema[sl].setbit(offset);
            }
            offset += itr->size();
            itr = next;
        }
    }
    LOG5("split_by_deparsed_bottom_bits schema: " << schema);
    return PHV::Slicing::split(sc, schema);
}

std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_valid_container_range(
    SuperCluster *sc) const {
    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        int offset = 0;
        auto itr = sl->begin();
        while (itr != sl->end()) {
            const auto valid_range = itr->validContainerRange();
            if (valid_range.size() == itr->size()) {
                schema[sl].setbit(offset + itr->size());
            }
            offset += itr->size();
            itr = std::next(itr);
        }
        schema[sl].clrbit(0);
        schema[sl].clrbit(offset);
    }
    LOG5("split_by_valid_container_range schema: " << schema);
    return PHV::Slicing::split(sc, schema);
}

std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_long_fieldslices(
    SuperCluster *sc) const {
    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        int offset = 0;
        for (auto itr = sl->begin(); itr != sl->end();
             offset += itr->size(), itr = std::next(itr)) {
            const auto &fs = *itr;
            if (fs.size() >= 64) {
                schema[sl].setbit(offset);
                int splitted_bits = 0;
                for (const int c : {32, 16, 8}) {
                    while (splitted_bits + c <= fs.size()) {
                        schema[sl].setbit(offset + splitted_bits + c);
                        splitted_bits += c;
                    }
                }
            }
        }
        schema[sl].clrbit(0);
        schema[sl].clrbit(offset);
    }
    // high log level because we use this presplit only when we cannot find a solution.
    LOG1("split_long_fieldslices schema: " << schema);
    return PHV::Slicing::split(sc, schema);
}

// Split by parser write mode compatibility
std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_parser_write_mode(
    SuperCluster *sc) {
    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        int offset = 0;
        int min_split_offset = 0;
        const FieldSlice *prev_non_pad = nullptr;
        for (auto itr = sl->begin(); itr != sl->end();
             offset += itr->size(), itr = std::next(itr)) {
            const auto &fs = *itr;
            if (!fs.field()->padding) {
                if (prev_non_pad &&
                    !check_write_mode_consistency_i.check_compatability(*prev_non_pad, fs)) {
                    // Attempt to split on a container boundary
                    int split_offset = offset;
                    for (auto size : {PHV::Size::b32, PHV::Size::b16, PHV::Size::b8}) {
                        int new_split = offset & ~(int(size) - 1);
                        if (new_split >= min_split_offset) {
                            split_offset = new_split;
                            break;
                        }
                    }
                    schema[sl].setbit(split_offset);
                }
                prev_non_pad = &fs;
                min_split_offset = offset + itr->size();
            }
        }
    }
    LOG1("split_parser_write_mode schema: " << schema);
    return PHV::Slicing::split(sc, schema);
}

// split by pa_container_size for those pragmas that are not up-casting,
// i.e. ignore cases like pa_container_sz(f1<8>, 32); Those will be left
// to be iterated by the allocation algorithm to figure out the best way.
std::optional<std::list<SuperCluster *>> DfsItrContext::split_by_pa_container_size(
    const SuperCluster *sc, const PHVContainerSizeLayout &pa_sz_layout) {
    // add wide_arith fields to pa_container_size requirements.
    PHVContainerSizeLayout pa = pa_sz_layout;
    sc->forall_fieldslices([&](const FieldSlice &fs) {
        const auto *field = fs.field();
        if (field->used_in_wide_arith()) {
            if (!pa.count(field)) {
                pa[field] = {32, 32};
            }
        }
    });
    // filter out exact_size pragmas;
    pa_container_size_upcastings_i.clear();
    // map (field, range) to AfterSplitConstraint::size when
    // pa_container_size pragma is applied.
    assoc::hash_map<const Field *, ordered_map<le_bitrange, int>> expected_size;
    assoc::hash_set<const Field *> exact_size_pragmas;
    for (const auto &kv : pa) {
        const auto *field = kv.first;
        const auto &sizes = kv.second;
        int total_sz =
            std::accumulate(sizes.begin(), sizes.end(), 0, [](int a, int s) { return a + int(s); });
        // invalid pragma, ignore, the earlier passes may not catch and error it,
        // because some pragma may be added by other passes, like meter_color,
        // and those passes do not check the validity.
        if (total_sz < field->size) {
            LOG6("Invalid pragma found on: "
                 << field << ", with total @pa_container_size pragma sizes: " << total_sz);
            return std::nullopt;
        } else if (total_sz == field->size) {
            LOG6("Found pa_container_size with same-size: " << field);
            exact_size_pragmas.insert(field);
            int offset = 0;
            for (const int sz : sizes) {
                expected_size[field][StartLen(offset, sz)] = sz;
                offset += sz;
            }
        } else {
            LOG6("Found upcasting pa_container_size pragma: " << field);
            // upcasting pragma, recorded for pruning.
            if (sizes.size() == 1) {
                pa_container_size_upcastings_i[field] = {StartLen(0, field->size), sizes.front()};
            } else {
                // multiple-sized upcasting pragma, only the tail needs to be upcasted.
                const int prefix_size = total_sz - sizes.back();
                pa_container_size_upcastings_i[field] = {StartLen(prefix_size, sizes.back()),
                                                         sizes.back()};
                // other than the tail, others will be split as specified, so we record them here.
                int offset = 0;
                for (const int sz : sizes) {
                    if (offset == prefix_size) {
                        break;
                    }
                    expected_size[field][StartLen(offset, sz)] = sz;
                    offset += sz;
                }
            }
        }
    }

    // split
    SplitSchema schema;
    for (auto *sl : sc->slice_lists()) {
        schema[sl] = bitvec();
        int offset = 0;
        const int sl_sz = SuperCluster::slice_list_total_bits(*sl);
        for (const auto &fs : *sl) {
            if (!pa.count(fs.field())) {
                offset += fs.size();
                continue;
            }
            const Field *field = fs.field();
            // exact pa_container_sizes should mark boundaries to split.
            // split at the beginning or the end of the field.
            if (exact_size_pragmas.count(field)) {
                if (fs.range().lo == 0) {
                    schema[sl].setbit(offset);
                }
                if (fs.range().hi == field->size - 1) {
                    schema[sl].setbit(offset + fs.size());
                }
            } else if (pa.at(field).size() > 1) {
                // expect for exact sizes pragma,
                // multiple-container-sized fields, if not exact_size_pragmas,
                // should mark the beginning boundary to split as well.
                if (fs.range().lo == 0) {
                    schema[sl].setbit(offset);
                }
            }

            // split in the middle of the field.
            int split_offset = 0;
            for (const auto &sz : pa.at(field)) {
                split_offset += int(sz);
                if (split_offset < fs.range().lo) {
                    continue;
                } else if (split_offset >= fs.range().lo && split_offset <= fs.range().hi + 1) {
                    schema[sl].setbit(offset + split_offset - fs.range().lo);
                } else {
                    break;
                }
            }
            offset += fs.size();
        }
        // do not mark the first or last bit.
        schema[sl].clrbit(0);
        schema[sl].clrbit(sl_sz);
    }

    LOG5("pa_container_sz slicing schema: " << schema);
    auto rst = PHV::Slicing::split(sc, schema);
    if (rst) {
        LOG5("split by pa_container_size result: ");
        for (const auto &sc : *rst) {
            LOG5(sc);
            sc->forall_fieldslices([&](const FieldSlice &fs) {
                if (!expected_size.count(fs.field())) {
                    return;
                }
                for (const auto &kv : expected_size.at(fs.field())) {
                    if (fs.range().intersectWith(kv.first).size() > 0) {
                        split_decisions_i[fs] = AfterSplitConstraint(
                            AfterSplitConstraint::ConstraintType::EXACT, kv.second);
                        LOG1("pa_container_size enforced AfterSplitConstraint: "
                             << fs << " must be " << split_decisions_i[fs]);
                    }
                }
            });
        }
    }
    return rst;
}

void DfsItrContext::iterate(const IterateCb &cb) {
    BUG_CHECK(!has_itr_i, "One ItrContext can only generate one iterator.");
    has_itr_i = true;

    LOG3("Making Itr for " << sc_i);
    LOG7("Homogeneous slicing enabled: " << config_i.homogeneous_slicing
                                         << " Rejected slice options: ");
    need_to_check_duplicate();
    for (auto sz : reject_sizes) LOG7(int(sz));

    // no slice list supercluster, a simpler case.
    if (sc_i->slice_lists().size() == 0) {
        LOG3("empty slicelist SuperCluster");
        Internal::no_slicelist_itr(cb, sc_i);
        return;
    }

    if (sc_i->needsStridedAlloc()) {
        LOG3("stride SuperCluster");
        Internal::stride_cluster_itr(cb, sc_i);
        return;
    }

    // slice_list validation
    for (const auto &sl : sc_i->slice_lists()) {
        if (SuperCluster::slice_list_has_exact_containers(*sl)) {
            int total_bits = SuperCluster::slice_list_total_bits(*sl);
            if (total_bits % 8 != 0) {
                LOG1("invalid supercluster, non_byte_aligned exact_containers slicelist: " << sl);
                return;
            }
        }
    }

    // apply pa_container_sizes-driven preslicing on clusters with
    // slicelists only.
    auto init_split = split_by_pa_container_size(sc_i, pa_i);
    if (!init_split) {
        LOG1("split by pa_container_sizes failed, iteration stopped.");
        return;
    }
    to_be_split_i.insert(init_split->begin(), init_split->end());

    // presplit by adjacent no_pack fields at byte boundary.
    auto after_pre_split = presplit_by(
        to_be_split_i, [&](SuperCluster *sc) { return split_by_adjacent_no_pack(sc); },
        "adjacent_no_pack"_cs);
    if (!after_pre_split) {
        LOG1("split by adjacent_no_pack fields failed, iteration stopped.");
        return;
    }
    to_be_split_i = *after_pre_split;

    // presplit by deparsed bottom bits.
    after_pre_split = presplit_by(
        to_be_split_i, [&](SuperCluster *sc) { return split_by_deparsed_bottom_bits(sc); },
        "deparsed_bottom_bits"_cs);
    if (!after_pre_split) {
        LOG1("split by deparsed_bottom_bits fields failed, iteration stopped.");
        return;
    }
    to_be_split_i = *after_pre_split;

    // presplit by deparsed/non_deparsed fields
    after_pre_split = presplit_by(
        to_be_split_i,
        [&](SuperCluster *sc) { return split_by_adjacent_deparsed_and_non_deparsed(sc); },
        "adjacent_deparsed_and_non_deparsed"_cs);
    if (!after_pre_split) {
        LOG1("split by adjacent_deparsed_and_non_deparsed fields failed, iteration stopped.");
        return;
    }
    to_be_split_i = *after_pre_split;

    // presplit by parser write mode compatibility
    after_pre_split = presplit_by(
        to_be_split_i, [&](SuperCluster *sc) { return split_by_parser_write_mode(sc); },
        "parser_write_mode"_cs);
    if (!after_pre_split) {
        LOG1("split by parser_write_mode fields failed, iteration stopped.");
        return;
    }
    to_be_split_i = *after_pre_split;

    // TODO: this is temporarily disabled because although it is doing the right thing,
    // PHV allocation and Table Placement failed to fit several customer profile.
    // We should enable this when alt-phv-alloc is stable.
    // presplit by valid container range constraint that a field cannot be packed with adjacent
    // field, when its valid container range is equal to the size of the field.
    // after_pre_split = presplit_by(
    //     to_be_split_i,
    //     [&](SuperCluster* sc) { return split_by_valid_container_range(sc); },
    //     "valid_container_range_limit"_cs);
    // if (!after_pre_split) {
    //     LOG1("split by valid_container_range_limit fields failed, iteration stopped.");
    //     return;
    // }
    // to_be_split_i = *after_pre_split;

    // start searching.
    auto res = dfs(cb, to_be_split_i);
    LOG1("DFS Result: " << res << ", n_steps_since_last_solution: " << n_steps_since_last_solution
                        << ", max_search_steps_per_solution: "
                        << config_i.max_search_steps_per_solution);

    // true indicates that we had troubles in finding a solution. Try aggressively presplit
    // large fieldslice to 32-bit chunks first and then rerun dfs.
    // TODO: this is a HACK to prevent extremely long compilation because
    // DFS was not effectively searching through critical slicing choices
    // that can produce a valid slicing.
    // An example is that when there are multiple bit<128> fields being searched for different
    // slicing between two critical choices, and the slicing of bit<128> fields does not matter.
    // TODO: There should be a algorithmic way to prune those cases.
    if (n_steps_since_last_solution > config_i.max_search_steps_per_solution) {
        LOG1(
            "failed to find one valid solution within step limit. "
            "Retry with pre-splitting large fieldslice");
        bool is_any_long_field_split = false;
        after_pre_split = presplit_by(
            to_be_split_i,
            [&](SuperCluster *sc) {
                auto splitted = split_by_long_fieldslices(sc);
                is_any_long_field_split = splitted && splitted->size() > 1;
                return splitted;
            },
            "split_by_long_fieldslices"_cs);
        if (!after_pre_split) {
            LOG1("split by split_by_long_fieldslices fields failed, iteration stopped.");
            return;
        }
        if (!is_any_long_field_split) {
            LOG1(
                "no optimization applied while we cannot find a solution in limited steps, "
                "iteration stopped.");
            return;
        }
        to_be_split_i = *after_pre_split;

        // restart searching.
        n_steps_since_last_solution = 0;
        dfs(cb, to_be_split_i);
    }
}

bool DfsItrContext::need_further_split(const SuperCluster::SliceList *sl) const {
    for (const auto &fs : *sl) {
        if (split_decisions_i.count(fs) > 0) {
            return false;
        }
    }

    const int sl_sz = SuperCluster::slice_list_total_bits(*sl);
    const auto &head_alignment = sl->front().alignment();
    const int head_offset = (head_alignment ? (*head_alignment).align : 0);
    if (SuperCluster::slice_list_has_exact_containers(*sl)) {
        if (sl_sz <= 8) {
            return false;
        }
    } else {
        const int span = head_offset + sl_sz;
        if (span <= 8) {
            return false;
        }
    }

    // It marks bits of no_split point to 1 on the no_split bitvec.
    bitvec no_split;
    int offset = sl->front().field()->exact_containers() ? 0 : head_offset;
    std::optional<FieldSlice> prev = std::nullopt;
    for (const auto &fs : *sl) {
        if (fs.field()->no_split()) {
            if (prev && fs.field() == (*prev).field()) {
                no_split.setbit(offset);
            }
            for (int i = 1; i < fs.size(); i++) {
                no_split.setbit(offset + i);
            }
        }
        prev = fs;
        offset += fs.size();
    }
    bool all_no_split = true;
    for (const int i : {8, 16, 32}) {
        if (i >= offset) {
            // TODO: use offset here because metadata may have aligment at head.
            break;
        }
        if (!no_split[i]) {
            all_no_split = false;
            break;
        }
    }
    if (all_no_split) {
        return false;
    }
    return true;
}

// check_pack_conflict checks whether there is any pack_conflict in the
// slice list that can be avoided.
// Note that there can be unavoidable pack_conflicts, for example, in this case
// even if f1 and f2 has pack conflict, because they are adjacent in byte,
// it's considered as unavoidable, function will return false.
// ingress::f1<20> ^0 meta no_split [0:10]
// ingress::f1<20> ^3 meta no_split [11:18]
// ingress::f1<20> ^3 meta no_split [19:19]
// ingress::f2<12> ^0 meta [0:11]
bool DfsItrContext::check_pack_conflict(const SuperCluster::SliceList *sl) const {
    int offset = 0;
    for (auto itr = sl->begin(); itr != sl->end(); offset += itr->size(), itr++) {
        auto next = std::next(itr);
        if (next != sl->end() && next->field() == itr->field()) {
            continue;
        }
        // If the end of fs reaches a byte boundary, skip.
        if ((offset + itr->size()) % 8 != 0) {
            const int tail_byte_num = (offset + itr->size()) / 8;
            // skip fields in the same byte, because no_pack is a soft constraints, when
            // fields are in a same byte, then it's impossible to allocate them without
            // violating this constraint.
            int next_offset = offset + itr->size();
            while (next != sl->end() && next_offset / 8 == tail_byte_num) {
                next_offset += next->size();
                next++;
            }
        }

        for (; next != sl->end(); next++) {
            if (has_pack_conflict_i(*itr, *next)) {
                LOG6("pack conflict: " << *itr << " and " << *next);
                return true;
            }
        }
    }
    return false;
}

bool DfsItrContext::dfs_prune_unwell_formed(const SuperCluster *sc) const {
    // cannot be split and is not well-formed.
    bool can_split_further = false;
    for (auto *sl : sc->slice_lists()) {
        if (need_further_split(sl)) {
            can_split_further = true;
        } else {
            //// won't be further split but
            // contains packing conflicts.
            if (check_pack_conflict(sl)) {
                LOG5("DFS pruned: found pack conflict in: " << sl);
                return true;
            }
            // impossible to satisfy valid container range.
            if (!can_satisfy_valid_container_range(sl)) {
                LOG5("DFS pruned: slice list cannot satisfy valid container range: " << sl);
                return true;
            }
            // two slices in aligned cluster in same slice list.
            assoc::hash_set<const PHV::AlignedCluster *> seen;
            for (auto &slice : *sl) {
                const auto *cluster = &sc->aligned_cluster(slice);
                if (seen.count(cluster) != 0) {
                    LOG5("DFS pruned: slice list has two slices from the same aligned cluster: \n"
                         << sl);
                    return true;
                }
                seen.insert(cluster);
            }
        }
    }
    auto err = new PHV::Error();
    if (!can_split_further && !SuperCluster::is_well_formed(sc, err)) {
        LOG5("DFS pruned: cannot split further and is not well formed: " << sc << ", because "
                                                                         << err->str());
        return true;
    }
    return false;
}

bool DfsItrContext::dfs_prune_invalid_parser_packing(const SuperCluster *sc) const {
    for (auto *sl : sc->slice_lists()) {
        if (need_further_split(sl)) {
            continue;
        }
        v2::FieldSliceAllocStartMap fs_starts;
        int offset = 0;
        for (const auto &fs : *sl) {
            // ideally we should filter out field slices that will never be live in
            // parser. But since these fields are all in a slice list, those cases
            // are rare: likely either all lived in parser or all not live, expect:
            // slice list was formed from: (1) field list resubmit/mirror...
            // (2) optimization-driven packing.
            fs_starts[fs] = offset;
            offset += fs.size();
        }
        if (auto err = parser_packing_validator_i.can_pack(fs_starts, true)) {
            LOG5("DFS pruned: invalid parser packing found in " << *sl << ", because "
                                                                << err->str());
            return true;
        }
    }
    return false;
}

std::optional<SplitDecision> DfsItrContext::collect_aftersplit_constraints(
    const SuperCluster *sc) const {
    using ctype = AfterSplitConstraint::ConstraintType;
    // sz decided by other fieldslice in the super cluster.
    SplitDecision decided_sz;
    bool has_conflicting_decisions = false;

    // record AfterSplitConstraint from pa_container_size upcasting pragma.
    sc->forall_fieldslices([&](const FieldSlice &fs) {
        if (pa_container_size_upcastings_i.count(fs.field())) {
            le_bitrange range;
            int size;
            std::tie(range, size) = pa_container_size_upcastings_i.at(fs.field());
            if (range.intersectWith(fs.range()).size() > 0) {
                decided_sz[fs] = AfterSplitConstraint(ctype::EXACT, size);
            }
        }
    });

    // collect implicit container sz constraints.
    if (!collect_implicit_container_sz_constraint(&decided_sz, sc)) {
        return std::nullopt;
    }

    // record AfterSplitConstraint from slice list that does not need any further
    // split but the decision was not made by DFS, i.e., split_decisions_i
    // does not count any fs of the field.
    for (const auto *sl : sc->slice_lists()) {
        if (!split_decisions_i.count(sl->front()) && !need_further_split(sl)) {
            const int sl_sz = SuperCluster::slice_list_total_bits(*sl);
            int constraint_sz = ROUNDUP(sl_sz, 8) * 8;
            constraint_sz = constraint_sz == 24 ? 32 : constraint_sz;
            const ctype constraint_t =
                SuperCluster::slice_list_has_exact_containers(*sl) ? ctype::EXACT : ctype::MIN;
            AfterSplitConstraint constraint = AfterSplitConstraint(constraint_t, constraint_sz);
            for (const auto &fs : *sl) {
                if (decided_sz.count(fs)) {
                    auto intersection = constraint.intersect(decided_sz.at(fs));
                    if (!intersection) {
                        LOG5("DFS pruned(conflicted decisions with pragma): "
                             << "pragma constraint must " << decided_sz.at(fs) << ", but " << fs
                             << " must " << constraint);
                        return std::nullopt;
                    } else {
                        decided_sz[fs] = *intersection;
                    }
                } else {
                    decided_sz[fs] = constraint;
                }
            }
        }
    }

    // build mapping for same_container_group fields.
    assoc::hash_map<const Field *, ordered_set<FieldSlice>> same_container_group_slices;
    sc->forall_fieldslices([&](const FieldSlice &fs) {
        if (fs.field()->same_container_group()) {
            same_container_group_slices[fs.field()].insert(fs);
        }
    });

    // collect constraints on all fieldslices.
    sc->forall_fieldslices([&](const FieldSlice &fs) {
        if (has_conflicting_decisions) {
            return;
        }
        AfterSplitConstraint constraint;
        // propagate decision collected above, that was not made by DFS, to other fs in cluster.
        if (decided_sz.count(fs)) {
            constraint = decided_sz.at(fs);
        }
        if (split_decisions_i.count(fs)) {
            auto intersection = constraint.intersect(split_decisions_i.at(fs));
            if (!intersection) {
                LOG5("DFS pruned(conflicted decisions on same field): "
                     << fs << " must be both " << constraint << " and "
                     << split_decisions_i.at(fs));
                has_conflicting_decisions = true;
                return;
            }
            constraint = *intersection;
        }
        if (constraint.type() == AfterSplitConstraint::ConstraintType::NONE) {
            return;
        }

        ordered_set<FieldSlice> others;
        for (const auto &other : sc->cluster(fs).slices()) {
            others.insert(other);
        }
        // For slices of a field with same_container_group constraint, all those slices
        // will end up in a same supercluster, so they share same aftersplit constraints
        // just slices in a rotation cluster.
        if (fs.field()->same_container_group()) {
            for (const auto &other : same_container_group_slices.at(fs.field())) {
                others.insert(other);
            }
        }
        // propagate constraint.
        for (const auto &other : others) {
            if (!decided_sz.count(other)) {
                decided_sz[other] = constraint;
                continue;
            }

            auto intersection = decided_sz.at(other).intersect(constraint);
            if (!intersection) {
                LOG5("DFS pruned(conflicted decisions): " << other << " must "
                                                          << decided_sz.at(other) << ", but " << fs
                                                          << " must " << constraint);
                has_conflicting_decisions = true;
                return;
            } else {
                decided_sz[other] = *intersection;
            }
        }
    });
    if (has_conflicting_decisions) {
        return std::nullopt;
    }
    return decided_sz;
}

bool DfsItrContext::collect_implicit_container_sz_constraint(SplitDecision *decided_sz,
                                                             const SuperCluster *sc) const {
    const auto intersect_and_save = [&](const AfterSplitConstraint &c, const FieldSlice &fs) {
        if (!decided_sz->count(fs)) {
            decided_sz->emplace(fs, c);
            return true;
        }
        auto intersection = decided_sz->at(fs).intersect(c);
        if (!intersection) {
            LOG5("DFS pruned(conflicting implict container_sz): " << fs << " was decided to "
                                                                  << decided_sz->at(fs) << ", but "
                                                                  << fs << " can only be " << c);
            return false;
        } else {
            decided_sz->emplace(fs, *intersection);
        }
        return true;
    };

    bool sat = true;
    sc->forall_fieldslices([&](const FieldSlice &fs) {
        std::list<int> possible_sizes(AfterSplitConstraint::all_container_sizes.begin(),
                                      AfterSplitConstraint::all_container_sizes.end());
        possible_sizes.remove_if([&](const int n) {
            // no_split field size is larger than container.
            // NOTE: works for <= 32-bit fields only because
            // wide_arithmetic fields might be no_split and more than 32 bits.
            if (fs.field()->no_split() && fs.field()->size <= 32 && fs.field()->size > n) {
                return true;
            }
            // deparsed_bottom_bits + valid_container_range
            // This special case is mostly for egress::eg_intr_md.egress_port.
            // NOTE: when a field slide does not have valid container range,
            // its validContainer range is ZeroToMax(). So it is dangerous to do addition on
            // the `hi` field (although when it's ClosedRange it should be safe because the
            // implementation of ClosedRange::ZeroToMax's hi is
            // std::numeric_limits<int>::max() - 1).
            // Anyway, so we guard it with range <= 32, which is the max of container width.
            if (fs.field()->deparsed_bottom_bits() && fs.range().lo == 0) {
                if (fs.validContainerRange().hi <= 32 && n > fs.validContainerRange().hi + 1) {
                    return true;
                }
            }
            return false;
        });
        // when possible_sizes = 0, the field can never be allocated.
        if (possible_sizes.empty()) {
            LOG1("found impossible to allocate field: " << fs);
            sat = false;
        } else {
            // check if exists a valid container size for fs.
            if (decided_sz->count(fs)) {
                auto after_split = decided_sz->at(fs);
                if (!std::any_of(possible_sizes.begin(), possible_sizes.end(),
                                 [&](int n) { return after_split.ok(n); })) {
                    LOG5("DFS pruned(unsat implict container_sz): " << fs << " was decided to "
                                                                    << after_split << ", but " << fs
                                                                    << " can not satisfy.");
                    sat = false;
                    return;
                }
            }
            bitvec possible_sizes_bv;
            for (int i : possible_sizes) {
                possible_sizes_bv[i] = true;
            }
            AfterSplitConstraint constraint = AfterSplitConstraint(possible_sizes_bv);
            if (!intersect_and_save(constraint, fs)) {
                sat = false;
            }
        }
    });
    return sat;
}

bool DfsItrContext::dfs_prune_unsat_slicelist_max_size(const SplitDecision &constraints,
                                                       const SuperCluster *sc) const {
    // the maximum bits of slices that a fieldslice can reside in.
    assoc::hash_map<FieldSlice, int> max_slicelist_bits;
    for (auto *sl : sc->slice_lists()) {
        int sz = SuperCluster::slice_list_total_bits(*sl);
        for (const auto &fs : *sl) {
            if (fs.field()->exact_containers()) {
                max_slicelist_bits[fs] = sz;
            }
        }
    }

    // (2) if max_slicelist_bits of other fields
    // in the same rot cluster is smaller than decided
    // sz, then it's impossible to split more.
    return sc->any_of_fieldslices([&](const FieldSlice &fs) {
        if (constraints.count(fs) && max_slicelist_bits.count(fs)) {
            const auto &constraint = constraints.at(fs);
            if (constraint.type() != AfterSplitConstraint::ConstraintType::NONE &&
                constraint.min() > max_slicelist_bits.at(fs)) {
                LOG5("DFS pruned(unsat_decision_sl_sz): "
                     << fs << " must be allocated to " << constraint
                     << ", but the slicelist it resides is only " << max_slicelist_bits.at(fs));
                return true;
            }
        }
        return false;
    });
}

// return true if constraints for a slice list cannot be *all* satisfied.
bool DfsItrContext::dfs_prune_unsat_slicelist_constraints(const SplitDecision &decided_sz,
                                                          const SuperCluster *sc) const {
    // TODO: can be further improved by constraints that exact_container
    // slice lists starts with byte-boundary. But it maybe too complicated.
    for (auto *sl : sc->slice_lists()) {
        // apply on exact container slicelists only.
        if (!sl->front().field()->exact_containers()) {
            continue;
        }
        // bit to fieldlist mapping.
        auto fs_bitmap = make_fs_bitmap(sl);
        int sl_sz = SuperCluster::slice_list_total_bits(*sl);
        std::vector<AfterSplitConstraint> constraint_vec(sl_sz);
        // start_bit_left_bound is maintained during the next for loop, representing the
        // leftbound of starting bit of the fieldslice (fs) that is being iterated.
        int start_bit_left_bound = 0;
        int offset = 0;
        for (const auto &fs : *sl) {
            if (!decided_sz.count(fs) ||
                decided_sz.at(fs).type() == AfterSplitConstraint::ConstraintType::NONE) {
                offset += fs.size();
                continue;
            }
            const auto sz_req = decided_sz.at(fs);

            // check constraint conflict.
            if (!sz_req.intersect(constraint_vec[offset])) {
                LOG5("DFS pruned(unsat_constraint_1): " << fs << ", decided sz: " << sz_req
                                                        << ", but the starting bit requires "
                                                        << constraint_vec[offset]);
                return true;
            }

            // seek back to the left-most possible starting bit.
            int leftmost_start = offset;
            for (; leftmost_start >= start_bit_left_bound; leftmost_start--) {
                const auto prev_fs = fs_bitmap.at(leftmost_start);
                if (!phv_i.must_alloc_same_container(prev_fs, fs) &&
                    has_pack_conflict_i(prev_fs, fs)) {
                    break;
                }
                auto intersection = constraint_vec[leftmost_start].intersect(sz_req);
                if (!intersection) {
                    break;
                }
            }
            leftmost_start++;
            // leftmost_start plus the container size must cover the whole fieldslice.
            if (sz_req.type() == AfterSplitConstraint::ConstraintType::MIN) {
                leftmost_start = std::max(leftmost_start, offset + fs.size() - 32);
            } else {
                leftmost_start = std::max(leftmost_start, offset + fs.size() - sz_req.min());
            }

            // check if there is enough room to satisfy se_req.
            if (leftmost_start + sz_req.min() > sl_sz) {
                if (LOGGING(5)) {
                    LOG5("DFS pruned(unsat_constraint_2): not enough room at tail. "
                         << "slice list: " << sl << "\n, fieldslice " << fs
                         << " must be allocated to " << sz_req << ", offset: " << offset);
                    std::stringstream ss;
                    ss << "constraint map: \n";
                    for (size_t i = 0; i < constraint_vec.size(); i++) {
                        ss << i << ": " << constraint_vec[i] << "\n";
                    }
                    LOG5(ss.str());
                }
                return true;
            }

            // check forwarding pack_conflict.
            for (int i = leftmost_start; i < leftmost_start + sz_req.min(); i++) {
                const auto next_fs = fs_bitmap.at(i);
                if (!phv_i.must_alloc_same_container(next_fs, fs) &&
                    has_pack_conflict_i(next_fs, fs)) {
                    LOG5("DFS pruned(unsat_constraint_3): not enough room after "
                         << i << ", slice list: " << sl << "\n, fieldslice " << fs
                         << " must be allocated to " << sz_req << ", offset: " << offset
                         << "\n, plus no_pack with " << fs_bitmap.at(i));
                    return true;
                }
            }

            // mark size constraints on the vector.
            for (int i = leftmost_start; i < leftmost_start + sz_req.min(); i++) {
                constraint_vec[i] = *constraint_vec[i].intersect(sz_req);
            }

            // update start_bit_left_bound by assuming this field slice will be allocated
            // starting from the left most possible position.
            // We can only update this value when the container size requirement for field slice
            // is EXACT and the size of the field equals to the container size.
            if (sz_req.type() == AfterSplitConstraint::ConstraintType::EXACT &&
                sz_req.min() == fs.size()) {
                start_bit_left_bound = leftmost_start + fs.size();
            }

            // update offset.
            offset += fs.size();
        }
    }
    return false;
}

// RangeLookupableConstraints supports range lookup on field slices.
struct RangeLookupableConstraints {
    assoc::hash_map<const Field *, assoc::map<le_bitrange, AfterSplitConstraint>> constraints;

    explicit RangeLookupableConstraints(const SplitDecision &original) {
        for (const auto &c : original) {
            constraints[c.first.field()][c.first.range()] = c.second;
        }
    }

    std::optional<AfterSplitConstraint> lookup(const FieldSlice &fs) const {
        if (!constraints.count(fs.field())) {
            return std::nullopt;
        }
        for (const auto &c : constraints.at(fs.field())) {
            if (c.first.contains(fs.range())) {
                return c.second;
            }
        }
        return std::nullopt;
    }
};

bool DfsItrContext::dfs_prune_unsat_exact_list_size_mismatch(const SplitDecision &decided_sz,
                                                             const SuperCluster *sc) const {
    const auto constraints = RangeLookupableConstraints(decided_sz);
    for (auto *sl : sc->slice_lists()) {
        // Note that exact_list_size mismatch can only be created by metadata lists that
        // merge two exact lists with different sizes into one supercluster. Also,
        // exact_containers list has been checked by collect_after_split_constraint already.
        if (sl->front().field()->exact_containers()) {
            continue;
        }
        std::vector<SuperCluster::SliceList *> targets;
        if (!need_further_split(sl)) {
            targets.push_back(sl);
        } else {
            targets = SuperCluster::slice_list_split_by_byte(*sl);
        }
        for (auto *target : targets) {
            AfterSplitConstraint exact_size_req;
            for (const auto &fs : *target) {
                auto fs_constraint = constraints.lookup(fs);
                if (fs_constraint) {
                    auto intersect = exact_size_req.intersect(*fs_constraint);
                    if (!intersect) {
                        LOG5("DFS pruned(unsat_exact_size_mismatch): at "
                             << fs << ", it must be " << *fs_constraint << " but there are "
                             << exact_size_req << " before this fieldslice");
                        return true;
                    } else {
                        exact_size_req = *intersect;
                    }
                }
            }
        }
    }
    return false;
}

bool DfsItrContext::dfs_prune_invalid_packing(const SuperCluster *sc) {
    ordered_set<const SuperCluster::SliceList *> decided_packings;
    ordered_set<const SuperCluster::SliceList *> undecided_lists;
    for (auto *sl : sc->slice_lists()) {
        if (!need_further_split(sl)) {
            decided_packings.insert(sl);
        } else {
            // if the size of list has not been decided yet, we can still
            // check for fieldslices within one byte because they will be
            // packed in one container no matter how we slice the cluster.
            undecided_lists.insert(sl);
        }
    }
    auto rst = action_packing_validator_i.can_pack(decided_packings, undecided_lists,
                                                   config_i.loose_action_packing_check_mode);
    switch (rst.code) {
        case ActionPackingValidatorInterface::Result::Code::OK: {
            return false;
        }
        case ActionPackingValidatorInterface::Result::Code::BAD: {
            LOG5("DFS pruned(invalid packing): " << rst.err);
            if (config_i.smart_backtracking_mode) {
                for (const auto *sl : *rst.invalid_packing) {
                    invalidate(sl);
                }
            }
            return true;
        }
        default:
            LOG6("skip invalid packing prune, because cannot check packing: " << rst.err);
            return false;
    }
}

bool DfsItrContext::dfs_prune(const ordered_set<SuperCluster *> &unchecked) {
    for (const auto *sc : unchecked) {
        // unwell_formed
        if (dfs_prune_unwell_formed(sc)) {
            LOG5("dfs prune unwell formed");
            return true;
        }

        // decided slice list will create invalid parser packing.
        if (!config_i.disable_packing_check && dfs_prune_invalid_parser_packing(sc)) {
            LOG5("dfs prune invalid parser packing ");
            return true;
        }

        // conflicting constraints
        auto after_split_constraints = collect_aftersplit_constraints(sc);
        if (!after_split_constraints) {
            LOG5("conflicting aftersplit constraints");
            return true;
        }

        // unsat constraints due to slice list sz limit.
        if (dfs_prune_unsat_slicelist_max_size(*after_split_constraints, sc)) {
            LOG5("dfs prune unsat slicelist max size");
            return true;
        }

        // constraints for a slice list cannot be *all* satisfied.
        if (dfs_prune_unsat_slicelist_constraints(*after_split_constraints, sc)) {
            LOG5("dfs prune unsat slicelist constraints");
            return true;
        }

        // prune when metadata list joins two size-mismatched exact lists.
        if (dfs_prune_unsat_exact_list_size_mismatch(*after_split_constraints, sc)) {
            LOG5("dfs prune unsat exact list size mismatch");
            return true;
        }

        // prune invalid packing
        if (!config_i.disable_packing_check && dfs_prune_invalid_packing(sc)) {
            LOG5("dfs prune invalid packing");
            return true;
        }
    }
    return false;
}

std::vector<SuperCluster *> DfsItrContext::get_well_formed_no_more_split() const {
    std::vector<SuperCluster *> rst;
    for (const auto &sc : to_be_split_i) {
        bool no_more_split = true;
        for (auto *sl : sc->slice_lists()) {
            if (need_further_split(sl)) {
                no_more_split = false;
                break;
            }
        }
        if (no_more_split && SuperCluster::is_well_formed(sc)) {
            rst.push_back(sc);
        }
    }
    return rst;
}

void DfsItrContext::need_to_check_duplicate() {
    if (check_duplicate != -1) return;
    check_duplicate = 0;
    ordered_set<FieldSlice> fs_in_sl;
    ordered_set<FieldSlice> fs_in_sc;
    // We only want to check slicing plan duplications for superclusters that have many slice lists
    // which will result in prolonged the slicing plan search time.
    if (sc_i->slices().size() < 5) return;
    for (auto fs : sc_i->slices()) {
        if (fs.size() != 32) return;
        fs_in_sc.insert(fs);
    }

    for (auto sl : sc_i->slice_lists()) {
        if (sl->size() > 1) return;
        fs_in_sl.insert(sl->front());
    }

    if (sc_i->clusters().size() > 1) return;
    for (auto fs : sc_i->clusters().front()->slices()) {
        if (fs_in_sl.count(fs)) fs_in_sl.erase(fs);
        fs_in_sc.erase(fs);
    }
    if (fs_in_sl.size() > 0) return;
    if (fs_in_sc.size() > 0) return;
    check_duplicate = 1;
}

bool DfsItrContext::check_duplicate_slicing_plan(const ordered_set<SuperCluster *> done) {
    if (done.size() > 1) return false;
    BUG_CHECK(check_duplicate != -1, "check_duplicate has not been set");
    if (!check_duplicate) return false;

    bool all_32b_container = false;

    for (auto sl : done.front()->slice_lists()) {
        if (SuperCluster::slice_list_total_bits(*sl) == 32) {
            all_32b_container = true;
            break;
        }
    }
    if (all_32b_container) {
        if (check_duplicate == 1) {
            check_duplicate = 2;
            return false;
        } else if (check_duplicate == 2) {
            return true;
        }
    }

    return false;
}

// return false if iteration should be terminated.
bool DfsItrContext::dfs(const IterateCb &yield, const ordered_set<SuperCluster *> &unchecked) {
    // prune when we reached the limit of steps.
    n_steps_i++;
    if (n_steps_i > config_i.max_search_steps) {
        return false;
    }

    // prune when we spend too much time in finding one solution.
    // It usually means that we made wrong decisions at the beginning of DFS, that
    // our prune strategy cannot detect and it takes too long time to backtrack to
    // overwrite wrong decisions.
    n_steps_since_last_solution++;
    if (n_steps_since_last_solution > config_i.max_search_steps_per_solution) {
        return false;
    }

    // track dfs search depth for debug.
    dfs_depth_i++;
    LOG5("ENTER DFS: " << dfs_depth_i);
    DeferHelper log_dfs_stack([this]() {
        LOG5("LEAVE DFS: " << dfs_depth_i);
        dfs_depth_i--;
    });

    // pruning
    if (dfs_prune(unchecked)) {
        return true;
    }

    // move super clusters that cannot be split and well-formed to done.
    auto well_formed_cannot_split = get_well_formed_no_more_split();
    for (auto *sc : well_formed_cannot_split) {
        to_be_split_i.erase(sc);
        done_i.insert(sc);
    }
    DeferHelper revert_well_formed_cannot_split([&]() {
        for (auto *sc : well_formed_cannot_split) {
            to_be_split_i.insert(sc);
            done_i.erase(sc);
        }
    });

    // found one solution.
    if (to_be_split_i.empty()) {
        if (check_duplicate_slicing_plan(done_i)) {
            return true;
        }
        LOG4("found a solution after " << n_steps_i << " steps");
        n_steps_since_last_solution = 0;
        return yield(std::list<SuperCluster *>(done_i.begin(), done_i.end()));
    }

    // search
    auto target = dfs_pick_next();
    if (!target) {
        return true;
    }
    auto *sc = (*target).first;
    auto *sl = (*target).second;
    LOG5("dfs_depth-" << dfs_depth_i << ": will split " << sl);

    // remove this sc from to_be_split.
    to_be_split_i.erase(sc);
    DeferHelper resotre_target([&]() { to_be_split_i.insert(sc); });

    // try all possible way to split out the first N bits of the slicelist
    // then recursion.
    auto choices = make_choices(*target);
    bool has_non_rejected_slice_choice = false;
    std::optional<SplitChoice> max_slice_choice = std::nullopt;
    std::stringstream ss;
    ss << "{ ";
    for (const auto &c : choices) {
        if (!reject_sizes.count(c)) has_non_rejected_slice_choice = true;
        if (!max_slice_choice)
            max_slice_choice = c;
        else if (c > max_slice_choice)
            max_slice_choice = c;
        ss << int(c) << " ";
    }
    ss << "}";
    LOG5("dfs_depth-" << dfs_depth_i << ": possible choices: " << ss.str());
    for (const auto &choice : choices) {
        LOG5("dfs_depth-" << dfs_depth_i << ": try to split @" << int(choice));

        // If slicing choice has been rejected by other slicelist do not use it unless:
        //   1. there are no non-rejected choices or
        //   2. the rejected choice is the maximum available slicing choice
        if ((has_non_rejected_slice_choice && reject_sizes.count(choice)) &&
            (choice != max_slice_choice)) {
            LOG5("Split size " << int(choice) << " has been previously rejected");
            continue;
        }

        // create metadata for making a split.
        const auto split_meta = make_split_meta(sc, sl, int(choice));
        if (!split_meta) {
            LOG5("dfs_depth-" << dfs_depth_i << ": cannot split by " << int(choice));
            if (config_i.homogeneous_slicing) {
                reject_sizes.insert(choice);
                LOG5("Adding split option " << int(choice) << " to reject_sizes");
            }
            continue;
        }
        auto rst = PHV::Slicing::split(sc, split_meta->first);
        if (rst) {
            LOG5("dfs_depth-" << dfs_depth_i << ": decide to split out first " << int(choice)
                              << " bits of " << sl);
            to_be_split_i.insert(rst->begin(), rst->end());
            split_decisions_i.insert(split_meta->second.begin(), split_meta->second.end());
            slicelist_on_stack_i.push_back(sl);
            DeferHelper restore_results([&]() {
                for (const auto &sc : *rst) {
                    to_be_split_i.erase(sc);
                }
                for (auto &v : split_meta->second) {
                    split_decisions_i.erase(v.first);
                }
                slicelist_on_stack_i.pop_back();
            });
            ordered_set<SuperCluster *> newly_created;
            newly_created.insert(rst->begin(), rst->end());
            if (!dfs(yield, newly_created)) {
                LOG5("cannot find a solution");
                return false;
            }
            if (to_invalidate != nullptr) {
                // If the current sl is the invalidation target, then we reset to_invalid.
                // Otherwise, we backtrack to the layer that its sl equals to invalidation.
                if (to_invalidate == sl) {
                    LOG1("to_invalidate stop at dfs-depth-"
                         << dfs_depth_i << ", last choice: " << int(choice) << " on " << sl);
                    to_invalidate_sl_counter.erase(to_invalidate);
                    to_invalidate = nullptr;
                } else {
                    LOG1("Found to_invalidate: " << to_invalidate);
                    LOG1("Backtracking from : " << sl);
                    return true;
                }
            }
        } else {
            LOG5("cannot split by " << int(choice));
        }
    }
    return true;
}

void DfsItrContext::invalidate(const SuperCluster::SliceList *sl) {
    // current to_invalidate slice list has already exceeded the limit, must
    // backtrack to the point, so ignoring all other to invalidate.
    if (to_invalidate && to_invalidate_sl_counter[to_invalidate] > to_invalidate_max_ignore) {
        return;
    }
    bool ignore = false;
    const SuperCluster::SliceList *target_sl = nullptr;
    for (const auto &sl_on_stack : boost::adaptors::reverse(slicelist_on_stack_i)) {
        // if there is already a to-invalidate slice list, then we will only update
        // it if the new @p sl is on higher stack frame: try to backtrack less.
        if (to_invalidate && to_invalidate == sl_on_stack) {
            ignore = true;
        }
        if (overlapped(sl, sl_on_stack)) {
            target_sl = sl_on_stack;
            break;
        }
    }
    if (target_sl) {
        // if we found the slice list on stack.
        if (ignore) {
            // if it is deeper than current to_invalidate, just inc counter.
            to_invalidate_sl_counter[target_sl]++;
            // but if the counter exceeds the limit, we will always use the deeper one.
            if (to_invalidate_sl_counter[target_sl] > to_invalidate_max_ignore) {
                to_invalidate = target_sl;
            }
        } else {
            // if it is lower than current to_invalidate, replace current with this new
            // slice list and inc counter for the current to_invalidate.
            if (to_invalidate) {
                to_invalidate_sl_counter[to_invalidate]++;
                // but if the counter exceeds the limit, do not replace.
                if (to_invalidate_sl_counter[to_invalidate] > to_invalidate_max_ignore) {
                    return;
                }
            }
            to_invalidate = target_sl;
        }
    }
    return;
}

std::ostream &operator<<(std::ostream &out, const PHV::Slicing::AfterSplitConstraint &c) {
    out << "{";
    std::string sep = "";
    for (const auto &s : c.sizes) {
        out << sep << s;
        sep = ",";
    }
    out << "}";
    return out;
}

}  // namespace Slicing
}  // namespace PHV
