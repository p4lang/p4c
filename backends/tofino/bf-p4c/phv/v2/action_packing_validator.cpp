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

#include "bf-p4c/phv/v2/action_packing_validator.h"

#include <algorithm>
#include <optional>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/join.hpp>

#include "bf-p4c/logging/logging.h"
#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/solver/action_constraint_solver.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"
#include "lib/algorithm.h"
#include "lib/bitvec.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_set.h"
#include "lib/safe_vector.h"

namespace PHV {
namespace v2 {

/// Map a slice list to the assigned starting position in container. Use this to assign a
/// start to a floating slice list that its allocation are not fixed.
using SliceListAssignedStarts = ordered_map<const SuperCluster::SliceList*, int>;

namespace {

using Result = ActionPackingValidator::Result;
using Code = Result::Code;

/// helper function to merge two set of integers.
std::set<int> intersect(const std::set<int>& a, const std::set<int>& b) {
    std::set<int> rv;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(rv, rv.begin()));
    return rv;
}

std::optional<unsigned int> slice_list_alignment(const SuperCluster::SliceList* sl) {
    const auto& head = sl->front();
    if (head.alignment()) {
        return head.alignment()->align;
    }
    return std::nullopt;
}

// assuming we allocate sl to the container size.
int max_slice_list_packable_bits(const SuperCluster::SliceList* sl) {
    const auto alignment = slice_list_alignment(sl);
    int max_empty_bits = alignment ? *alignment : 0;
    int adjacent_empty_bits = 0;
    for (const auto& fs : *sl) {
        if (fs.field()->is_padding() || fs.field()->is_ignore_alloc()) {
            adjacent_empty_bits += fs.size();
        } else {
            adjacent_empty_bits = 0;
        }
        max_empty_bits = std::max(max_empty_bits, adjacent_empty_bits);
    }
    return max_empty_bits;
}

/// @returns if @p sources has only one phv source of move-based instruction, return that slice.
std::optional<FieldSlice> get_phv_move_src(const safe_vector<SourceOp>& sources) {
    if (sources.size() != 1 || sources.front().t != SourceOp::OpType::move) {
        return std::nullopt;
    }
    const auto& src = sources.front();
    if (src.ad_or_const) {
        return std::nullopt;
    }
    return *src.phv_src;
}

/// SliceListGroupProp contains misc properties for input slice lists.
/// An important concept here is `settlement` for slice lists. We say a slice list is settled
/// if we know that it will be allocated to one container as-is, i.e., cannot be split further.
/// For settled slice list with exact_containers constraint, we can infer
/// the size of container and the alignment of allocation. For settled metadata list, we will do
/// a union-find on src-dest relations to guess its container size, if possible.
/// For unsettled slice list, we do not make any assumption on their allocation: they can be
/// split or merged with other slice list before allocation. They are generally not useful except
/// for that for fieldslices sharing a byte in the list, they must be packed and allocated
/// to container. So, we split them by byte and and use each byte to validate unavoidable packing.
struct SliceListGroupProp {
    /// settled_sl is a set of slice list that they must be allocated without further slicing.
    ordered_set<const SuperCluster::SliceList*> settled_sl;

    /// byte_sl_unsettled is a set of byte-sized slice list that because their slicing has not
    /// been settled yet, meaning that they can be merged with adjacent (in the
    /// original slice list) slice lists before allocation.
    /// Lists of this set are byte-sized, i.e., their size is at most 8-bit, representing
    /// unavoidable packings due to byte parsing, deparsing, and special ALU constraints.
    ordered_set<const SuperCluster::SliceList*> unsettled_byte_sl;

    /// map field to all its fieldslice involved for this group of slice lists.
    ordered_map<const Field*, ordered_set<FieldSlice>> field_settled_slices;

    /// map fieldslice to its slice list.
    ordered_map<FieldSlice, const SuperCluster::SliceList*> fs_sl;

    /// map fieldslice to its offset (including prepending alignment) in the slice list.
    ordered_map<FieldSlice, int> fs_offset;

    /// slice lists grouped by their container sizes.
    UnionFind<const SuperCluster::SliceList*> same_container_size_group;

    /// map slice list to the number of bits required.
    ordered_map<const SuperCluster::SliceList*, int> sl_bits;

    /// map slice list to its container size. If its container size can not be decided
    /// within this group, sl_cont_size.count(sl) == 0.
    ordered_map<const SuperCluster::SliceList*, int> sl_cont_size;

    /// map slice list to its container ID for the solver. If its container size can not be decided
    /// within this group, sl_containers.count(sl) == 0. This ID is just for convenience. You
    /// can use other IDs in solver later, as long as relevant slice lists have unique IDs.
    /// All these container IDs will start with a character "C".
    ordered_map<const SuperCluster::SliceList*, solver::ContainerID> sl_container;

    /// map fieldslice to its sources.
    ordered_map<FieldSlice, ActionClassifiedSources> fs_sources;

    /// map slice list to all actions that will modify its fieldslices.
    ordered_map<const SuperCluster::SliceList*, ordered_set<const IR::MAU::Action*>> sl_actions;

    /// return decided container of @p sl.
    std::optional<solver::ContainerID> container(const SuperCluster::SliceList* sl) const {
        if (sl_container.count(sl)) {
            return sl_container.at(sl);
        }
        return std::nullopt;
    }

    /// return decided container of @p fs.
    std::optional<solver::ContainerID> container(const FieldSlice& fs) const {
        if (fs_sl.count(fs)) {
            return container(fs_sl.at(fs));
        }
        return std::nullopt;
    }

    /// return decided container size of @p sl.
    std::optional<int> container_size(const SuperCluster::SliceList* sl) const {
        if (sl_cont_size.count(sl)) {
            return sl_cont_size.at(sl);
        }
        return std::nullopt;
    }

    /// return decided container size of @p fs.
    std::optional<int> container_size(const FieldSlice& fs) const {
        if (fs_sl.count(fs)) {
            return container_size(fs_sl.at(fs));
        }
        return std::nullopt;
    }

    /// return the offset (the index of starting bit, including prepending alignment)
    /// of @p fs in its slice list.
    int offset(const FieldSlice& fs) const {
        BUG_CHECK(fs_offset.count(fs), "fieldslice does not exist: %1%", fs);
        return fs_offset.at(fs);
    }

    /// return the number of bits that the @p sl can be shifted if allocated
    /// to the decided container.
    int floating_range(const SuperCluster::SliceList* sl) const {
        BUG_CHECK(container(sl), "container size not decided: %1%", sl);
        BUG_CHECK(sl_bits.count(sl), "unknown slice list: %1%", sl);
        const auto& front = sl->front();
        if (front.field()->deparsed_bottom_bits() && front.range().lo == 0) {
            return 0;
        } else {
            return sl_cont_size.at(sl) - sl_bits.at(sl);
        }
    }

    /// returns a fieldslice which contains the @p fs.
    /// The returned slice is used as key for properties in this class.
    std::optional<PHV::FieldSlice> find_enclosing_settled_fs(const FieldSlice& fs) const {
        if (!field_settled_slices.count(fs.field())) {
            return std::nullopt;
        }
        for (const auto& enclosing_fs : field_settled_slices.at(fs.field())) {
            if (enclosing_fs.range().overlaps(fs.range())) {
                BUG_CHECK(enclosing_fs.range().contains(fs.range()), "non fine sliced fs: %1%", fs);
                return enclosing_fs;
            }
        }
        return std::nullopt;
    }

    /// return the container ID and the offset of the starting bit of @p fs, if
    /// the enclosing slice list will be allocated at 0th bit of container.
    std::optional<std::pair<solver::ContainerID, int>> find_allocation(
        const FieldSlice& fs,
        const SliceListAssignedStarts& sl_assigned_start = {}) const {
        auto enclosing_fs = find_enclosing_settled_fs(fs);
        if (!enclosing_fs) {
            return std::nullopt;
        }
        auto* enclosing_sl = fs_sl.at(*enclosing_fs);
        const auto alloc_container = container(*enclosing_fs);
        if (!alloc_container) {
            return std::nullopt;
        }
        const int alloc_offset = offset(*enclosing_fs);
        if (floating_range(enclosing_sl) > 0) {
            if (sl_assigned_start.count(enclosing_sl)) {
                auto fixed_start = sl_assigned_start.at(enclosing_sl);
                return std::pair<solver::ContainerID, int>{
                    *alloc_container,
                    fixed_start + alloc_offset + fs.range().lo - enclosing_fs->range().lo};
            }
            return std::nullopt;
        }
        return std::pair<solver::ContainerID, int>{
            *alloc_container, alloc_offset + fs.range().lo - enclosing_fs->range().lo};
    }
};

/// Construct a SliceListProp in @p prop.
Result make_slicelist_group_prop(
    SliceListGroupProp* prop, const ActionSourceTracker& sources,
    const ordered_set<const SuperCluster::SliceList*>& slice_lists,
    const ordered_set<const SuperCluster::SliceList*>& can_be_split_further) {
    static const std::set<int> container_sizes = {8, 16, 32};

    // save slice_lists that cannot be split further to sl_settled.
    for (const auto* sl : slice_lists) {
        prop->settled_sl.insert(sl);
    }

    // split @p can_be_split_further by byte and save to unsettled.
    for (const auto* sl : can_be_split_further) {
        std::vector<SuperCluster::SliceList*> byte_lists =
            SuperCluster::slice_list_split_by_byte(*sl);
        for (const auto* byte_list : byte_lists) {
            prop->unsettled_byte_sl.insert(byte_list);
        }
    }

    // compute and save misc properties.
    for (const auto* sl : boost::range::join(prop->settled_sl, prop->unsettled_byte_sl)) {
        int offset = slice_list_alignment(sl).value_or(0);
        for (const auto& fs : *sl) {
            prop->fs_sl[fs] = sl;
            prop->fs_offset[fs] = offset;
            offset += fs.size();
            prop->fs_sources[fs] = sources.get_sources(fs);
            for (const auto* act : Keys(prop->fs_sources[fs])) {
                prop->sl_actions[sl].insert(act);
            }
        }
        prop->sl_bits[sl] = offset;
    }

    // compute a set of valid container sizes for all settled lists.
    ordered_map<const SuperCluster::SliceList*, std::set<int>> sl_valid_cont_sizes;
    for (const auto* sl : prop->settled_sl) {
        const auto& head = sl->front();
        const int sl_sz = prop->sl_bits.at(sl);
        if (head.field()->exact_containers()) {
            BUG_CHECK(container_sizes.count(sl_sz),
                      "exact container slicelist with invalid length: %1%, bits: %2%", sl, sl_sz);
            sl_valid_cont_sizes[sl] = {sl_sz};
        } else {
            for (const int v : boost::adaptors::reverse(container_sizes)) {
                if (v < sl_sz) {
                    break;
                }
                sl_valid_cont_sizes[sl].insert(v);
            }
        }
    }

    // build a mapping from field to its settled sliced. We need it when
    // looking up sources container while checking unsettled lists.
    for (const auto* sl : prop->settled_sl) {
        for (const auto& fs : *sl) {
            prop->field_settled_slices[fs.field()].insert(fs);
        }
    }

    // build a union find for settled slicelists grouped by their container sizes.
    for (const auto* sl : prop->settled_sl) {
        prop->same_container_size_group.insert(sl);
    }
    for (const auto* dest_sl : prop->settled_sl) {
        for (const auto& fs : *dest_sl) {
            const auto& action_sources = prop->fs_sources.at(fs);
            for (const auto& sources : Values(action_sources)) {
                for (const auto& source : sources) {
                    if (!source.phv_src) continue;
                    if (!prop->fs_sl.count(*source.phv_src)) continue;
                    const auto* src_sl = prop->fs_sl.at(*source.phv_src);
                    // union-find within settled lists only.
                    if (!prop->settled_sl.count(src_sl)) continue;
                    prop->same_container_size_group.makeUnion(dest_sl, src_sl);
                }
            }
        }
    }

    // speculate container size for settled slicelists
    for (const auto& group : prop->same_container_size_group) {
        std::set<int> valid_sizes = container_sizes;
        for (const auto* sl : group) {
            valid_sizes = intersect(valid_sizes, sl_valid_cont_sizes.at(sl));
        }
        if (valid_sizes.size() > 1) {
            // cannot decide container slices for this group.
            if (LOGGING(3)) {
                for (const auto* sl : group) {
                    LOG3("skip " << sl << " because of too many valid container sizes: "
                         << valid_sizes.size());
                }
            }
            continue;
        } else if (valid_sizes.size() == 0) {
            return Result(Code::BAD, "container size conflicts"_cs);
        }
        const int cont_size = *valid_sizes.begin();
        for (const auto* sl : group) {
            prop->sl_cont_size[sl] = cont_size;
        }
    }

    // assign unique container names for settled slice list with speculated container size.
    int unique_id = 0;
    for (const auto& sl_cont_size : prop->sl_cont_size) {
        const auto* sl = sl_cont_size.first;
        const int cont_size = sl_cont_size.second;
        prop->sl_container[sl] =
            "C" + cstring::to_cstring(cont_size) + '_' + cstring::to_cstring(unique_id++);
        LOG5("decide the container of " << sl << " to be " << prop->sl_container[sl]);
    }

    return Result(Code::OK);
}

// validate whether there are mixed type of operations for @p sl in @p action.
// Returns Result with Code::Bad when there are mixed operations, otherwise returns Code::Ok.
Result validate_mixed_op(const ordered_map<FieldSlice, ActionClassifiedSources>& fs_action_sources,
                         const SuperCluster::SliceList* sl, const IR::MAU::Action* action) {
    // mixed operands on one container destination is invalid.
    std::set<SourceOp::OpType> ops;
    for (const auto& fs : *sl) {
        if (!fs_action_sources.at(fs).count(action)) {
            continue;
        }
        const auto& sources = fs_action_sources.at(fs).at(action);
        for (const auto& s : sources) {
            ops.insert(s.t);
        }
        if (ops.size() > 1) {
            auto rst = Result(Code::BAD, (boost::format("mixed operands: %1%") % sl).str());
            rst.invalid_action = action;
            rst.invalid_packing->insert(sl);
            return rst;
        }
    }
    return Result(Code::OK);
}

Result validate_num_sources_of_sl_and_action(
    const ordered_map<FieldSlice, ActionClassifiedSources>& fs_action_sources,
    const ordered_map<FieldSlice, const SuperCluster::SliceList*>& settled_fs_sl,
    const ordered_map<FieldSlice, const SuperCluster::SliceList*>& unsettled_fs_sl,
    const SuperCluster::SliceList* sl, const IR::MAU::Action* action) {
    bool has_ad_or_const_src = false;
    ordered_set<const SuperCluster::SliceList*> settled_exact_containers_sources;
    ordered_set<const SuperCluster::SliceList*> unsettled_exact_containers_sources;
    bool has_meta_src = false;
    for (const auto& fs : *sl) {
        if (!fs_action_sources.at(fs).count(action)) {
            continue;
        }
        const auto& sources = fs_action_sources.at(fs).at(action);
        if (sources.size() != 1 || sources.front().t != SourceOp::OpType::move) {
            continue;
        }
        const auto& src = sources.front();
        if (src.ad_or_const) {
            has_ad_or_const_src = true;
        } else {
            const auto& phv_src = *src.phv_src;
            if (!phv_src.field()->exact_containers()) {
                has_meta_src = true;
                continue;
            }
            if (settled_fs_sl.count(fs)) {
                settled_exact_containers_sources.insert(settled_fs_sl.at(fs));
            } else if (unsettled_fs_sl.count(fs)) {
                unsettled_exact_containers_sources.insert(unsettled_fs_sl.at(fs));
            } else {
                BUG("cannot find slice list for %1%", fs);
            }
        }
    }
    const int total = int(has_ad_or_const_src) + int(has_meta_src) +
                      unsettled_exact_containers_sources.size() +
                      settled_exact_containers_sources.size();
    LOG5("validate num sources: " << sl << " of action " << action->externalName());
    LOG5("ad_or_const_src: " << int(has_ad_or_const_src));
    LOG5("has_meta_src: " << int(has_meta_src));
    LOG5("unsettled_exact_containers_sources: " << unsettled_exact_containers_sources.size());
    LOG5("settled_exact_containers_sources: " << settled_exact_containers_sources.size());
    if (total > 2) {
        return Result(
            Code::BAD,
            (boost::format("Action %1% will have %2% sources (too many) for slice list %3%") %
             action->externalName() % total % sl).str());
    }
    return Result(Code::OK);
}

Result validate_num_sources(
    const SliceListGroupProp& prop,
    const ordered_set<const SuperCluster::SliceList*>& settled,
    const ordered_set<const SuperCluster::SliceList*>& unsettled) {
    ordered_map<FieldSlice, const SuperCluster::SliceList*> settled_fs_sl;
    for (const auto* sl : settled) {
        for (const auto& fs : *sl) {
            settled_fs_sl[fs] = sl;
        }
    }
    ordered_map<FieldSlice, const SuperCluster::SliceList*> unsettled_fs_sl;
    for (const auto* sl : unsettled) {
        for (const auto& fs : *sl) {
            unsettled_fs_sl[fs] = sl;
        }
    }
    // only check settled sl.
    for (const auto* sl : settled) {
        // skip slicelist that does not have packing.
        if (sl->size() == 1) continue;
        // skip slice list not written by actions.
        if (!prop.sl_actions.count(sl)) continue;
        for (const auto* action : prop.sl_actions.at(sl)) {
            auto rst = validate_num_sources_of_sl_and_action(
                    prop.fs_sources, settled_fs_sl, unsettled_fs_sl, sl, action);
            rst.invalid_action = action;
            rst.invalid_packing->insert(sl);
            if (rst.code == Code::BAD) {
                return rst;
            }
        }
    }
    return Result(Code::OK);
}

/// The returned bitvec[i] is true only when i-the bit is in a byte that will *NOT* be
/// written by more than 1 source, e.g., 1 action data and 1 container, or two
/// container sources. For those bytes, we can always slice the destination more to avoid.
/// being corrupted by the writing instruction.
bitvec compute_single_source_bytes_bv(const SliceListGroupProp& prop,
                                      const IR::MAU::Action* action,
                                      const SuperCluster::SliceList* sl,
                                      const int dest_offset,
                                      const SliceListAssignedStarts& sl_assigned_start = {}) {
    // using container name to represent sources, $ad_const represents action data.
    std::map<int, ordered_set<cstring>> dest_byte_written_by;
    for (const auto& fs : *sl) {
        if (!prop.fs_sources.at(fs).count(action)) continue;
        const int start_bit_idx = dest_offset + prop.fs_offset.at(fs);
        // ClosedOpen range of written bytes: [start, end)
        const int start_byte = start_bit_idx / 8;
        const int end_byte = ROUNDUP(start_bit_idx + fs.size(), 8);

        // currently we support move-based instruction only.
        const auto& sources = prop.fs_sources.at(fs).at(action);
        if (sources.size() > 1 || sources.front().t != SourceOp::OpType::move) {
            continue;
        }
        const auto& src = sources.front();
        for (int i = start_byte; i < end_byte; i++) {
            if (src.ad_or_const) {
                dest_byte_written_by[i].insert("$ad_const"_cs);
            } else {
                const auto& src_fs = *src.phv_src;
                const auto src_alloc = prop.find_allocation(src_fs, sl_assigned_start);
                if (!src_alloc) {
                    continue;
                }
                dest_byte_written_by[i].insert(src_alloc->first);
            }
        }
    }
    bitvec rv;
    for (const auto& kv : dest_byte_written_by) {
        if (kv.second.size() <= 1) {
            rv.setrange(kv.first * 8, 8);
        }
    }
    return rv;
}

// validate_slicelist_for_action returns results with Code::Bad if it found that @p action
// that cannot be synthesized, when  @p sl is allocated to
// container @p dest of @p dest_cont_size bits, starting from the @p dest_offset bit.
// @p prop must have all properties of slice list that their allocation can be speculated.
Result validate_slicelist_for_action(const PhvUse& uses,
                                     const SliceListGroupProp& prop,
                                     const solver::ContainerID dest,
                                     const int dest_cont_size,
                                     const int dest_offset,
                                     const SuperCluster::SliceList* sl,
                                     const IR::MAU::Action* action,
                                     const bool loose_mode,
                                     const SliceListAssignedStarts& sl_assigned_start = {}) {
    // setup solver
    solver::ActionMoveSolver solver;
    // loose mode allows live bits of bytes that were not written by
    // (1) both ad_const and container, or
    // (2) more than 1 container source,
    // in this action to be corrupted. Because the corruption can be avoid
    // if we slice more on destination.
    bitvec no_corruption_check_bits;
    if (loose_mode) {
        no_corruption_check_bits =
            compute_single_source_bytes_bv(prop, action, sl, dest_offset, sl_assigned_start);
    }
    // dest_live assumes that as long as the field is used, it will be live after this action.
    // This is sometimes stricter than the reality, because field slice can be dead after this
    // action.
    bitvec dest_live;
    for (const auto& fs : *sl) {
        const int start_bit_idx = dest_offset + prop.fs_offset.at(fs);
        const bool is_written = prop.fs_sources.at(fs).count(action);
        if (is_written) {
            dest_live.setrange(start_bit_idx, fs.size());
            continue;
        }
        const bool is_not_padding = !fs.field()->is_padding() && !fs.field()->is_ignore_alloc() &&
                                    uses.is_referenced(fs.field());
        if (!is_not_padding) continue;
        // we will mark bits of live but unwritten field slices to be *live*
        // (cannot be corrupted), only if they are not part of the no check list.
        for (int i = start_bit_idx; i < start_bit_idx + fs.size(); i++) {
            if (!no_corruption_check_bits.getbit(i)) {
                dest_live.setbit(i);
            }
        }
    }
    solver.set_container_spec(dest, dest_cont_size, dest_live);
    LOG5("dest live bits bitvec: " << dest_live);

    auto* involved_lists = new ordered_set<const SuperCluster::SliceList*>{sl};
    // setup assignments
    for (const auto& fs : *sl) {
        // skip fs not written in @p action.
        if (!prop.fs_sources.at(fs).count(action)) {
            continue;
        }
        const auto& sources = prop.fs_sources.at(fs).at(action);
        // currently we support move-based instruction only.
        if (sources.size() > 1 || sources.front().t != SourceOp::OpType::move) {
            continue;
        }
        const auto dest_operand = solver::make_container_operand(
            dest, StartLen(dest_offset + prop.fs_offset.at(fs), fs.size()));
        const auto& src = sources.front();
        LOG5("for destination: " << fs << " and source: " << src);
        if (src.ad_or_const) {
            LOG5("adding const assignment");
            solver.add_assign(dest_operand, solver::make_ad_or_const_operand());
        } else {
            const auto& src_fs = *src.phv_src;
            const auto src_alloc = prop.find_allocation(src_fs, sl_assigned_start);
            // skip when there is a source that its allocation is not decided that
            // (1) either it does not have decided container size,
            // (2) or its float_range is large than 0.
            // TODO: although we can verify packing even if there are sources that
            // floating range is large than 0, by iterating combinations of them, we do not,
            // because the time complexity will become nonlinear.
            if (!src_alloc) {
                LOG5("adding unallocated source to " << dest_operand);
                solver.add_src_unallocated_assign(dest_operand.container, dest_operand.range);
                continue;
            }
            involved_lists->insert(prop.fs_sl.at(*prop.find_enclosing_settled_fs(src_fs)));
            const auto src_operand = solver::make_container_operand(
                src_alloc->first, StartLen(src_alloc->second, src_fs.size()));
            if (src_alloc->first != dest) {
                solver.set_container_spec(src_alloc->first, dest_cont_size, bitvec());
            }
            solver.add_assign(dest_operand, src_operand);
            LOG5("adding phv assignment from " << src_operand << " to " << dest_operand);
        }
    }
    auto rst = solver.solve();
    if (rst.err) {
        LOG3("solver error: " << rst.err->msg);
    } else {
        LOG3("solver result: okay.");
    }
    if (!rst.ok()) {
        auto err_msg = (boost::format("impossible to synthesize instruction for destination"
                                      " %1% in action %2%") %
                        sl % action->externalName()).str();
        auto rst = Result(Code::BAD, err_msg);
        rst.invalid_action = action;
        rst.invalid_packing = involved_lists;
        return rst;
    }
    return Result(Code::OK);
}

std::optional<const SuperCluster::SliceList*> find_the_only_floating_src_sl(
    const SliceListGroupProp& prop,
    const SuperCluster::SliceList* sl,
    const IR::MAU::Action* action) {
    ordered_set<const SuperCluster::SliceList*> floating_src_slice_lists;
    ordered_set<const SuperCluster::SliceList*> fixed_src_slice_lists;
    for (const auto& fs : *sl) {
        // skip fs not written in @p action.
        if (!prop.fs_sources.at(fs).count(action)) {
            continue;
        }
        const auto& sources = prop.fs_sources.at(fs).at(action);
        // currently we support move-based instruction only.
        if (sources.size() > 1 || sources.front().t != SourceOp::OpType::move) {
            continue;
        }
        const auto& src = sources.front();
        if (src.ad_or_const) {
            continue;
        }
        const auto& src_fs = *src.phv_src;
        auto enclosing_fs = prop.find_enclosing_settled_fs(src_fs);
        if (!enclosing_fs) {
            continue;
        }
        auto* enclosing_sl = prop.fs_sl.at(*enclosing_fs);
        const auto alloc_container = prop.container(*enclosing_fs);
        if (!alloc_container) {
            continue;
        }
        if (prop.floating_range(enclosing_sl) == 0) {
            fixed_src_slice_lists.insert(enclosing_sl);
            continue;
        }
        // found a floating.
        floating_src_slice_lists.insert(enclosing_sl);
    }
    if (floating_src_slice_lists.empty()) {
        LOG3("no floating source slice list.");
        return std::nullopt;
    }
    if (floating_src_slice_lists.size() > 1) {
        LOG3("found too many floating slice lists: " << floating_src_slice_lists.size());
        return std::nullopt;
    }
    // if this floating_sl might be packed with other source slice lists, we do not
    // check further, because the number of packing combinations can be huge.
    const auto* floating_sl = floating_src_slice_lists.front();
    LOG3("Found only one floating slice lists: " << floating_sl);
    const int floating_sl_bits = PHV::SuperCluster::slice_list_total_bits(*floating_sl);
    for (const auto* fixed_src : fixed_src_slice_lists) {
        if (max_slice_list_packable_bits(fixed_src) >= floating_sl_bits) {
            LOG3("not sure whether we should we pack " << fixed_src << " with above floating sl");
            return std::nullopt;
        }
    }
    return floating_sl;
}

Result validate_slicelist_for_action_allow_one_floating_src(
    const PhvUse& uses, const SliceListGroupProp& prop, const solver::ContainerID dest,
    const int dest_cont_size, const int dest_offset, const SuperCluster::SliceList* sl,
    const IR::MAU::Action* action, bool loose_mode) {
    const auto the_only_floating_src_sl = find_the_only_floating_src_sl(prop, sl, action);
    if (!the_only_floating_src_sl) {
        return validate_slicelist_for_action(
                uses, prop, dest, dest_cont_size, dest_offset, sl, action, loose_mode);
    }
    const int step = slice_list_alignment(sl) ? 8 : 1;
    const int floating_range = prop.floating_range(*the_only_floating_src_sl);
    Result rst;
    for (int offset = 0; offset <= floating_range; offset += step) {
        LOG3("Verify with assigning " << *the_only_floating_src_sl << " to start at " << offset);
        SliceListAssignedStarts assigned_starts;
        assigned_starts[*the_only_floating_src_sl] = offset;
        rst = validate_slicelist_for_action(uses, prop, dest, dest_cont_size, dest_offset, sl,
                                            action, loose_mode, assigned_starts);
        if (rst.code == Code::OK || rst.code == Code::UNKNOWN) {
            return rst;
        }
    }
    return rst;
}

Result validate_one_byte_slice_list(const PhvUse& uses,
                                    const SliceListGroupProp& prop,
                                    const SuperCluster::SliceList* byte_list) {
    // find the cont size for this byte.
    std::optional<int> cont_size = std::nullopt;
    for (const auto& fs : *byte_list) {
        const auto& action_sources = prop.fs_sources.at(fs);
        for (const auto& sources : Values(action_sources)) {
            // currently we support move-based instruction only.
            auto phv_src = get_phv_move_src(sources);
            if (!phv_src) {
                continue;
            }
            auto enclosing_src_fs = prop.find_enclosing_settled_fs(*phv_src);
            if (!enclosing_src_fs) {
                continue;
            }
            auto src_cont_size = prop.container_size(*enclosing_src_fs);
            if (!src_cont_size) {
                continue;
            }
            if (!cont_size) {
                cont_size = *src_cont_size;
                continue;
            }
            if (*cont_size != *src_cont_size) {
                return Result(Code::BAD, "multiple different src container sizes"_cs);
            }
        }
    }
    if (!cont_size) {
        return Result(Code::UNKNOWN, "container size not decided yet"_cs);
    }

    const int byte_list_sz = prop.sl_bits.at(byte_list);  // it could be less than 8 bits.
    LOG5("byte_list decided container size: " << *cont_size);
    for (const auto* action : prop.sl_actions.at(byte_list)) {
        auto check_mixed_op = validate_mixed_op(prop.fs_sources, byte_list, action);
        if (check_mixed_op.code == Code::BAD) {
            return check_mixed_op;
        }
        Result rst;
        const int step = slice_list_alignment(byte_list) ? 8 : 1;
        for (int offset = 0; offset <= *cont_size - byte_list_sz; offset += step) {
            LOG5("check action: " << action->externalName() << ", dest alloc offset: " << offset);
            rst = validate_slicelist_for_action(
                    uses, prop, "byte_dest"_cs, *cont_size, offset, byte_list, action, false);
            if (rst.code == Code::OK || rst.code == Code::UNKNOWN) {
                LOG5("found valid allocation at offset " << offset);
                break;
            }
        }
        if (rst.code == Code::BAD) {
            return rst;
        } else if (rst.code == Code::UNKNOWN) {
            LOG5("skip validation of " << byte_list << ", because " << rst.err);
        }
    }
    return Result(Code::OK);
}

}  // namespace

Result ActionPackingValidator::can_pack(
    const ordered_set<const SuperCluster::SliceList*>& slice_lists,
    const ordered_set<const SuperCluster::SliceList*>& can_be_split_further,
    const bool loose_mode) const {
    // NOTE: do not skip even if slice_lists.size() is 1. It can be useful later when
    // checking @p can_be_split_further.
    if (LOGGING(6) && slice_lists.size() > 0) {
        LOG6("check can_pack for:");
        for (const auto& sl : slice_lists) {
            LOG6("settled: " << sl);
        }
        for (const auto& sl : can_be_split_further) {
            LOG6("unsettled: " << sl);
        }
    }
    SliceListGroupProp prop;
    auto rst = make_slicelist_group_prop(&prop, sources_i, slice_lists, can_be_split_further);
    if (rst.code == Code::BAD) {
        return rst;
    }
    // validate that all settled slice_lists shall not have more than 2 sources.
    rst = validate_num_sources(prop, slice_lists, can_be_split_further);
    if (rst.code == Code::BAD) {
        return rst;
    }

    // validate using action solver for every pair of (settled slice_list, action)
    for (const auto* sl : prop.settled_sl) {
        // skip slicelist that does not have packing.
        if (sl->size() == 1) continue;
        // skip slice list not written by actions.
        if (!prop.sl_actions.count(sl)) continue;
        // skip slicelist that its container size cannot be decided.
        if (!prop.container(sl)) continue;
        LOG5("validate packing of slice list: " << sl
             << ". Its container: " << *prop.container(sl));
        // for metadata slice list, their alignment in the container may not be fixed,
        // we need to verify all of them so that if there is any alignment that the action
        // can be synthesized, we will allow it to pass validation.
        const int floating_range = prop.floating_range(sl);
        const int step = slice_list_alignment(sl) ? 8 : 1;
        // validate every action involved in this slice list.
        for (const auto* action : prop.sl_actions.at(sl)) {
            auto mixed_op_rst = validate_mixed_op(prop.fs_sources, sl, action);
            if (mixed_op_rst.code == Code::BAD) {
                return mixed_op_rst;
            }
            // If there is one offset that the action can be synthesized, return OK.
            Result rst;
            for (int offset = 0; offset <= floating_range; offset += step) {
                LOG5("check action: " << action->externalName()
                                      << ", dest alloc offset: " << offset);
                rst = validate_slicelist_for_action_allow_one_floating_src(
                    uses_i, prop, *prop.container(sl), *prop.container_size(sl), offset, sl,
                    action, loose_mode);
                if (rst.code == Code::OK || rst.code == Code::UNKNOWN) {
                    break;
                }
            }
            if (rst.code == Code::BAD) {
                return rst;
            } else if (rst.code == Code::UNKNOWN) {
                LOG5("skip validation of " << sl << ", because " << rst.err);
            }
        }
    }
    // validate same-byte-packing fieldslices in @p can_be_split_further list.
    for (const auto* byte_sl : prop.unsettled_byte_sl) {
        LOG5("validate same-byte packing: " << byte_sl);
        auto rst = validate_one_byte_slice_list(uses_i, prop, byte_sl);
        if (rst.code == Code::BAD) {
            return rst;
        } else if (rst.code == Code::UNKNOWN) {
            LOG5("skip validation of byte_list " << byte_sl << " because " << rst.err);
        }
    }
    return Result(Code::OK);
}

}  // namespace v2
}  // namespace PHV
