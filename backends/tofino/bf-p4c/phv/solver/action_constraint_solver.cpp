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

#include "phv/solver/action_constraint_solver.h"

#include <chrono>
#include <sstream>
#include <unordered_set>

#include <boost/format/format_fwd.hpp>

#include "bf-p4c/ir/bitrange.h"
#include "lib/bitvec.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_set.h"

namespace solver {

namespace {

/// SourceClassifiedAssigns is a helper class to classify assignments by its
/// sources.
struct SourceClassifiedAssigns {
    std::vector<Assign> ad_or_const;
    ordered_map<ContainerID, std::vector<Assign>> containers;

    // return the number of sources.
    int n_sources() const { return int(!ad_or_const.empty()) + containers.size(); }

    // n can be 1 or 2, represent the src1 and src2 for the instruction.
    // prefer to use ad_or_const as src.
    const std::vector<Assign> &get_src(int n) const {
        BUG_CHECK(n_sources() >= n, "get src out of index %1%", n);
        BUG_CHECK(n <= 2 && n > 0, "SourceClassifiedAssigns can only return src1 or src2");
        if (!ad_or_const.empty()) {
            if (n == 1) {
                return ad_or_const;
            } else {
                return containers.begin()->second;
            }
        } else {
            if (n == 1) {
                return containers.begin()->second;
            } else {
                return std::next(containers.begin())->second;
            }
        }
    }
};

SourceClassifiedAssigns classify_by_sources(const std::vector<Assign> &assigns) {
    SourceClassifiedAssigns rst;
    for (const auto &assign : assigns) {
        if (assign.src.is_ad_or_const) {
            rst.ad_or_const.push_back(assign);
        } else {
            rst.containers[assign.src.container].push_back(assign);
        }
    }
    return rst;
}

int left_rotate_offset(const Assign &assign, int container_sz) {
    if (assign.src.is_ad_or_const) {
        return 0;
    }
    int n_left_shift_bits = assign.dst.range.lo - assign.src.range.lo;
    if (n_left_shift_bits < 0) {
        n_left_shift_bits += container_sz;  // wrap-around case
    }
    return n_left_shift_bits;
}

int right_rotate(int dest_idx, int n_bits, int container_sz) {
    int src_idx = dest_idx - n_bits;
    if (src_idx < 0) {
        src_idx += container_sz;  // wrap-around case
    }
    return src_idx;
}

bitvec byte_rotate_merge_src1_mask(const std::vector<Assign> &assigns) {
    bitvec mask;
    for (const auto &v : assigns) {
        const int lo = (v.dst.range.lo / 8) * 8;
        const int width = (v.dst.range.hi / 8) * 8 - lo + 8;
        mask.setrange(lo, width);
    }
    return mask;
}

void assign_input_bug_check(const ordered_map<ContainerID, ContainerSpec> &specs,
                            const Assign &assign) {
    for (const auto op : {assign.src, assign.dst}) {
        if (op.is_ad_or_const) {
            BUG_CHECK(op.container == "", "action data or const must have nil container", op);
            continue;
        }
        BUG_CHECK(op.container != "", "empty container string not allowed unless ad_or_const: %1%",
                  op);
        BUG_CHECK(specs.count(op.container), "container used missing spec: %1%", op.container);
        BUG_CHECK(op.range.lo <= op.range.hi, "range must be less or equal to  hi: %1%", op);
        BUG_CHECK(op.range.lo >= 0 && op.range.hi < specs.at(op.container).size,
                  "out of index range: %1% of %2%, container size: %3%", op, assign,
                  specs.at(op.container).size);
    }
    // check assigned destination bits are live because we should never move bits
    // to non-field bit position in a container - usually it's a bug from caller.
    for (int i = assign.dst.range.lo; i <= assign.dst.range.hi; i++) {
        BUG_CHECK(specs.at(assign.dst.container).live[i],
                  "container %1%'s %2%th bit is not claimed live, but was set by %3%",
                  assign.dst.container, i, assign);
    }

    BUG_CHECK(!assign.dst.is_ad_or_const, "destination cannot be const or action data: %1%");
    if (!assign.src.is_ad_or_const) {
        BUG_CHECK(assign.dst.range.size() == assign.src.range.size(),
                  "assignment range mismatch: %1%", assign);
        BUG_CHECK(specs.at(assign.dst.container).size == specs.at(assign.src.container).size,
                  "container size mismatch: %1%", assign);
    }
}

/// return the first container bit that are live but not set for by assigns. A non-none return
/// value means that the bit@i is corrupted because of the whole container set instruction.
std::optional<int> invalid_whole_container_set(const std::vector<Assign> &assigns,
                                               const ContainerSpec &c,
                                               const bitvec &src_unallocated_bits) {
    bitvec set_bits;
    for (const auto &assign : assigns) {
        set_bits.setrange(assign.dst.range.lo, assign.dst.range.size());
    }
    for (int i = 0; i < c.size; i++) {
        if (c.live[i] && !src_unallocated_bits[i] && !set_bits[i]) {
            return i;
        }
    }
    return std::nullopt;
}

// convert empty container id to readable format.
ContainerID pretty_print(ContainerID c) { return c == ""_cs ? "ad_or_const"_cs : c; }

}  // namespace

cstring DepositField::to_cstring() const {
    return (boost::format("%1% %2%, %3%, %4%, %5%, %6%") % name() % dest % pretty_print(src1) %
            left_rotate % mask % src2)
        .str();
}

cstring BitmaskedSet::to_cstring() const {
    return (boost::format("%1% %2%, %3%, %4%, %5%") % name() % dest % pretty_print(src1) % src2 %
            mask)
        .str();
}
cstring ByteRotateMerge::to_cstring() const {
    return (boost::format("%1% %2%, (%3%, %4%), (%5%, %6%), %7%") % name() % dest %
            pretty_print(src1) % shift1 % src2 % shift2 % mask)
        .str();
}
cstring ContainerSet::to_cstring() const {
    return (boost::format("%1% %2%, %3%") % name() % dest % pretty_print(src)).str();
}

Operand make_ad_or_const_operand() { return Operand{true, ""_cs, le_bitrange()}; }

Operand make_container_operand(ContainerID c, le_bitrange r) { return Operand{false, c, r}; }

Result ActionSolverBase::try_container_set(const ContainerID dest,
                                           const RotateClassifiedAssigns &offset_assigns) const {
    // source must be aligned.
    if (offset_assigns.size() != 1 || !offset_assigns.count(0)) {
        std::stringstream ss;
        ss << "destination " << dest
           << " has too many unaligned sources: " << offset_assigns.size();
        return Result(Error(ErrorCode::too_many_unaligned_sources, ss.str()));
    }
    // only one source.
    const auto &source_classified = classify_by_sources(offset_assigns.at(0));
    if (source_classified.n_sources() > 1) {
        std::stringstream ss;
        ss << "destination " << dest << " has too many sources: " << source_classified.n_sources();
        return Result(Error(ErrorCode::too_many_container_sources, ss.str()));
    }
    // does not write other live bits.
    const auto &assigns = source_classified.get_src(1);
    auto invalid_write_bit =
        invalid_whole_container_set(assigns, specs_i.at(dest), src_unallocated_bits.at(dest));
    if (invalid_write_bit) {
        std::stringstream ss;
        ss << "whole container write on destination " << dest
           << " will corrupt bit : " << *invalid_write_bit;
        return Result(Error(ErrorCode::invalid_whole_container_write, ss.str()));
    }
    return Result(new ContainerSet(dest, assigns.front().src.container));
}

std::optional<Error> ActionSolverBase::check_whole_container_set_with_none_source_allocated()
    const {
    // check partial write for destinations when none of their sources has been allocated.
    for (const auto &dest_unallocated_bv : src_unallocated_bits) {
        const auto &dest = dest_unallocated_bv.first;
        const auto &unallocated_bv = dest_unallocated_bv.second;
        if (dest_assigns_i.count(dest)) continue;
        // do not check if there is no source unallocated.
        if (unallocated_bv.empty()) continue;
        auto invalid_write_bit = invalid_whole_container_set({}, specs_i.at(dest), unallocated_bv);
        if (invalid_write_bit) {
            std::stringstream ss;
            ss << "whole container write on destination " << dest
               << " will corrupt bit : " << *invalid_write_bit;
            return Error(ErrorCode::invalid_whole_container_write, ss.str());
        }
    }
    return std::nullopt;
}

void ActionSolverBase::add_assign(const Operand &dst, Operand src) {
    // Assume that ad_or_const sources are always aligned with dst.
    if (src.is_ad_or_const) {
        src.range = dst.range;
    }
    // move-based instructions can ignore move to itself.
    if (src == dst) {
        return;
    }
    const Assign assign{dst, src};
    LOG5("Solver add new assign: " << assign);
    assign_input_bug_check(specs_i, assign);
    const int offset = left_rotate_offset(assign, specs_i.at(dst.container).size);
    dest_assigns_i[dst.container][offset].push_back(assign);
}

void ActionSolverBase::add_src_unallocated_assign(const ContainerID &c, const le_bitrange &range) {
    BUG_CHECK(specs_i.count(c), "unknown container %1%", c);
    for (int i = range.lo; i <= range.hi; i++) {
        BUG_CHECK(
            specs_i.at(c).live[i],
            "container %1%'s %2%th bit is not claimed live, but was set by an unallocated source",
            c, i);
    }
    LOG5("Solver add source unallocated range on container " << c << ": " << range);
    src_unallocated_bits[c].setrange(range.lo, range.size());
}

void ActionSolverBase::set_container_spec(ContainerID id, int size, bitvec live) {
    BUG_CHECK(size <= 32, "container larger than 32-bit is not supported");
    BUG_CHECK(!id.startsWith("$"),
              "invalid ContainerID: %1%, starting with $ is reserved for internal names.", id);
    specs_i[id] = ContainerSpec{size, live};
    if (!src_unallocated_bits.count(id)) {
        src_unallocated_bits[id] = bitvec();
    }
}

std::optional<Error> ActionSolverBase::validate_input() const {
    std::stringstream err_msg;
    for (const auto &dest_assigns : dest_assigns_i) {
        bitvec assigned;
        for (const auto &offset_assigns : dest_assigns.second) {
            for (const auto &assign : offset_assigns.second) {
                const auto &dst = assign.dst;
                // no bit is assigned more than once.
                for (int i = dst.range.lo; i <= dst.range.hi; i++) {
                    if (assigned[i]) {
                        err_msg << "container " << dst.container << "'s " << i << "th bit is "
                                << "assigned multiple times";
                        return Error(ErrorCode::invalid_input, err_msg.str());
                    }
                }
                assigned.setrange(dst.range.lo, dst.range.size());
            }
        }
    }
    return std::nullopt;
}

std::optional<Error> ActionMoveSolver::dest_meet_expectation(
    const ContainerID dest, const std::vector<Assign> &src1, const std::vector<Assign> &src2,
    const symbolic_bitvec::BitVec &bv_dest, const symbolic_bitvec::BitVec &bv_src1,
    const symbolic_bitvec::BitVec &bv_src2) const {
    std::stringstream ss;
    // For live bits, they must be either
    // (1) unchanged.
    // (2) set by a correct bit from one of the source.
    // (3) have unallocated source.
    // checking (2) here:
    bitvec set_bits;
    for (const auto &assigns : {std::make_pair(src1, bv_src1), std::make_pair(src2, bv_src2)}) {
        const auto &source_bv = assigns.second;
        for (const auto &assign : assigns.first) {
            const int dst_lo = assign.dst.range.lo;
            const int src_lo = assign.src.range.lo;
            const int sz = assign.dst.range.size();
            set_bits.setrange(dst_lo, sz);
            if (!bv_dest.slice(dst_lo, sz).eq(source_bv.slice(src_lo, sz))) {
                ss << "solver unsat because " << assign << " cannot be satisfied";
                return Error(ErrorCode::unsat, ss.str());
            }
        }
    }

    // mark all set by unallocated source bits are set_bits.
    set_bits |= src_unallocated_bits.at(dest);

    // Any bit that is live (occupied by some non-mutex fields) and not set,
    // it cannot be changed. The only possible case that it will not be changed is
    // when src2 and dest are the same container, for both deposit-field or byte-rotate-merge.
    // Because in this solver, we always use src2 as background if needed.
    bool require_src2_be_dest = false;
    const auto &live_bits = specs_i.at(dest).live;
    for (int i = 0; i < specs_i.at(dest).size; i++) {
        // bits occupied by other field not involved in this action.
        // It's safe to assume that those bits are from s2, as long as
        // we called this function with both (s1, s2) and (s2, s1).
        if (live_bits[i] && !set_bits[i]) {
            require_src2_be_dest = true;
            if (!bv_dest.get(i)->eq(bv_src2.get(i))) {
                ss << "solver unsat because dest[" << i << "] is overwritten unexpectedly";
                return Error(ErrorCode::unsat, ss.str());
            }
        }
    }
    if (require_src2_be_dest && !src2.empty() && dest != src2.front().src.container) {
        std::stringstream ss;
        ss << "destination " << dest << " will be corrupted because src2 "
           << src2.front().src.container << " is not equal to dest";
        return Error(ErrorCode::deposit_src2_must_be_dest, ss.str());
    }
    return std::nullopt;
}

Result ActionMoveSolver::run_deposit_field_symbolic_bitvec_solver(
    const ContainerID dest, const std::vector<Assign> &src1,
    const std::vector<Assign> &src2) const {
    LOG5("deposit-field.src1 assigns: " << src1);
    LOG5("deposit-field.src2 assigns: " << src2);
    const int width = specs_i.at(dest).size;
    // pre-compute rot
    const int n_left_rotate = left_rotate_offset(src1.front(), width);
    // pre-compute mask
    int mask_l = width;
    int mask_h = -1;
    for (const auto &assign : src1) {
        mask_l = std::min(mask_l, assign.dst.range.lo);
        mask_h = std::max(mask_h, assign.dst.range.hi);
    }
    bitvec mask;
    mask.setrange(mask_l, mask_h - mask_l + 1);

    // build solver
    using namespace solver::symbolic_bitvec;
    BvContext ctx;
    const auto bv_src1 = ctx.new_bv(width);
    const auto bv_src2 = ctx.new_bv(width);
    const auto bv_mask = ctx.new_bv_const(width, mask);
    const auto bv_dest = (((bv_src1 << n_left_rotate) & bv_mask) | (bv_src2 & (~bv_mask)));
    LOG3("deposit-field mask.l = " << mask_l);
    LOG3("deposit-field mask.h = " << mask_h);
    LOG3("deposit-field rot = " << n_left_rotate);
    auto err = dest_meet_expectation(dest, src1, src2, bv_dest, bv_src1, bv_src2);
    if (err) {
        LOG3("failed to use deposit-field because " << err->msg);
        return Result(*err);
    }
    const auto &src2_container = src2.empty() ? dest : src2.front().src.container;
    return Result(
        new DepositField(dest, src1.front().src.container, n_left_rotate, mask, src2_container));
}

// run deposit-field instruction symbolic bitvec solver.
Result ActionMoveSolver::run_deposit_field_solver(const ContainerID dest,
                                                  const std::vector<Assign> &src1,
                                                  const std::vector<Assign> &src2) const {
    return run_deposit_field_symbolic_bitvec_solver(dest, src1, src2);
}

Result ActionMoveSolver::try_deposit_field(const ContainerID dest,
                                           const RotateClassifiedAssigns &assigns) const {
    std::stringstream err_msg;
    if (assigns.size() > 2 || (assigns.size() == 2 && !assigns.count(0))) {
        err_msg << "container " << dest
                << " has too many unaligned or non-rotationally aligned sources";
        return Result(Error(ErrorCode::too_many_unaligned_sources, err_msg.str()));
    }
    // all possible cases and possible way to synthesize instruction is listed:
    if (assigns.size() == 1) {
        // case-1 only one offset.
        const auto source_classified = classify_by_sources(assigns.begin()->second);
        if (assigns.count(0)) {
            // case-1.1 align
            if (source_classified.n_sources() == 1) {
                // case-1.1.1 all assignments were from the same container(or ad),
                // need to verify whether it's possible for deposit-field because of
                // mask range limitation
                return run_deposit_field_solver(dest, source_classified.get_src(1), {});
            } else if (source_classified.n_sources() == 2) {
                // case-1.1.2 assignments were from 2 containers(or ad),
                // might be possible by splitting into two operand for deposit-field.
                auto rst = run_deposit_field_solver(dest, source_classified.get_src(1),
                                                    source_classified.get_src(2));
                if (rst.ok() || !source_classified.ad_or_const.empty()) {
                    return rst;
                } else {
                    // a special case that when two sources are aligned and both are container
                    // sources, we can try swap src1 and src2 to see if we can get correct
                    // mask.
                    return run_deposit_field_solver(dest, source_classified.get_src(2),
                                                    source_classified.get_src(1));
                }
            } else {
                // case-1.1.3 more than 2 sources, not possible
                err_msg << "container " << dest << " has too many different container sources: "
                        << source_classified.n_sources();
                return Result(Error(ErrorCode::too_many_container_sources, err_msg.str()));
            }
        } else {
            // case-1.2 not aligned
            if (source_classified.n_sources() == 1) {
                // case-1.2.1 only one source, not always possible because of the mask
                // need to run solver to check.
                return run_deposit_field_solver(dest, source_classified.get_src(1), {});
            } else {
                // case-1.2.2 more than 1 unaligned sources, no possible
                err_msg << "container " << dest << " has too many unaligned container sources: "
                        << source_classified.n_sources();
                return Result(Error(ErrorCode::too_many_unaligned_sources, err_msg.str()));
            }
        }
    } else if (assigns.size() == 2) {
        // case-2
        if (!assigns.count(0)) {
            // case-2.1 more than 1 unaligned sources, no possible
            BUG("impossible to reach this point, should have been caught by above checks.");
        } else {
            // case-2.2 1 aligned, 1 rotationally aligned.
            // The aligned source must be src2, and cannot be ad_or_const source
            // The rotationally aligned source must be src1.
            const auto src2 = classify_by_sources(assigns.begin()->second);
            const auto src1 = classify_by_sources(std::next(assigns.begin())->second);
            for (const auto &src : {src1, src2}) {
                if (src.n_sources() > 1) {
                    err_msg << "container " << dest
                            << " has too many different container sources for one same offset: "
                            << src1.n_sources();
                    return Result(Error(ErrorCode::too_many_container_sources, err_msg.str()));
                }
            }
            if (!src2.ad_or_const.empty()) {
                err_msg << "container " << dest << " cannot have ad_or_const source as src2";
                return Result(Error(ErrorCode::invalid_for_deposit_field, err_msg.str()));
            }
            return run_deposit_field_solver(dest, src1.get_src(1), src2.get_src(1));
        }
    }
    BUG("impossible to reach this point, should have been caught by above checks.");
}

Result ActionMoveSolver::run_byte_rotate_merge_symbolic_bitvec_solver(
    const ContainerID dest, const std::vector<Assign> &src1,
    const std::vector<Assign> &src2) const {
    LOG5("byte-rotate-merge.src1 assigns: " << src1);
    LOG5("byte-rotate-merge.src2 assigns: " << src2);

    // generate mask2 from src1, that always exists.
    const int width = specs_i.at(dest).size;
    const bitvec mask1 = byte_rotate_merge_src1_mask(src1);
    const int src1_shift = left_rotate_offset(src1.front(), width);
    const int src2_shift = src2.empty() ? 0 : left_rotate_offset(src2.front(), width);
    using namespace solver::symbolic_bitvec;
    BvContext ctx;
    const auto bv_src1 = ctx.new_bv(width);
    const auto bv_src2 = ctx.new_bv(width);
    const auto bv_mask1 = ctx.new_bv_const(width, mask1);
    const auto bv_dest =
        (((bv_src1 << src1_shift) & bv_mask1) | ((bv_src2 << src2_shift) & (~bv_mask1)));

    LOG3("mask bv: " << mask1);
    LOG3("byte-rotate-merge.src1_left_shift: " << src1_shift);
    LOG3("byte-rotate-merge.src2_left_shift: " << src2_shift);
    LOG3("byte-rotate-merge.src1_mask: " << bv_mask1.to_cstring());
    auto err = dest_meet_expectation(dest, src1, src2, bv_dest, bv_src1, bv_src2);
    if (err) {
        return Result(*err);
    }
    const auto &src2_container = src2.empty() ? dest : src2.front().src.container;
    return Result(new ByteRotateMerge(dest, src1.front().src.container, src1_shift, src2_container,
                                      src2_shift, mask1));
}

Result ActionMoveSolver::run_byte_rotate_merge_solver(const ContainerID dest,
                                                      const std::vector<Assign> &src1,
                                                      const std::vector<Assign> &src2) const {
    return run_byte_rotate_merge_symbolic_bitvec_solver(dest, src1, src2);
}

// try to solve this assignment using byte_rotate_merge.
// classify assignments by
// (1) shift offset.
// (2) sources.
// There can only be up to 2 shifts, and per each shift, only one source, and ad_or_const
// data must all have the same shifts in src1. (no need to check this, because we
// assume it's always possible).
Result ActionMoveSolver::try_byte_rotate_merge(const ContainerID dest,
                                               const RotateClassifiedAssigns &assigns) const {
    std::stringstream ss;
    // more than two different shift offsets.
    if (assigns.size() > 2) {
        ss << "too many different-offset byte shifts required: " << assigns.size();
        return Result(Error(ErrorCode::too_many_unaligned_sources, ss.str()));
    }

    // non-byte-aligned shifts not possible for byte_rotate_merge.
    for (const auto &offset_assigns : assigns) {
        const int &offset = offset_assigns.first;
        if (offset % 8 != 0) {
            ss << "dest " << dest << " has non-byte-shiftable source offset: " << offset;
            return Result(Error(ErrorCode::non_rot_aligned_and_non_byte_shiftable, ss.str()));
        }
    }

    // run a solver for different cases.
    if (assigns.size() == 1) {
        // (1) only one shift offset, can have up to 2 different sources.
        SourceClassifiedAssigns sources = classify_by_sources(assigns.begin()->second);
        if (sources.n_sources() == 1) {
            // if only one source, put it in src1, src2 is empty and will be destination
            return run_byte_rotate_merge_solver(dest, assigns.begin()->second, {});
        } else if (sources.n_sources() == 2) {
            // two sources with one offset, we can split them into two set for this
            // instruction.
            return run_byte_rotate_merge_solver(dest, sources.get_src(1), sources.get_src(2));
        } else {
            ss << "too many container sources for dest " << dest;
            return Result(Error(ErrorCode::too_many_container_sources, ss.str()));
        }
    } else {
        // when there's two sets of different-offset assignments,
        // each can only have one container source.
        for (const auto &offset_assign : assigns) {
            SourceClassifiedAssigns sources = classify_by_sources(offset_assign.second);
            if (sources.n_sources() > 1) {
                ss << "too many container sources for dest " << dest;
                return Result(Error(ErrorCode::too_many_container_sources, ss.str()));
            }
        }
        // It's guaranteed that src1 will be ad_or_const assignments because their offset
        // will always be zero and @p assigns are sorted in map<int, T>.
        return run_byte_rotate_merge_solver(dest, assigns.begin()->second,
                                            std::next(assigns.begin())->second);
    }
}

/// try to solve this assignment using bitmasked-set
Result ActionMoveSolver::try_bitmasked_set(const ContainerID dest,
                                           const RotateClassifiedAssigns &assigns) const {
    std::stringstream err_msg;
    if (assigns.size() > 1 || !assigns.count(0)) {
        err_msg << "container " << dest << " has unaligned aligned sources";
        return Result(Error(ErrorCode::too_many_unaligned_sources, err_msg.str()));
    }
    const auto source_classified = classify_by_sources(assigns.begin()->second);
    if (source_classified.n_sources() == 1) {
        bitvec mask;
        for (const auto &assign : source_classified.get_src(1)) {
            mask.setrange(assign.dst.range.lo, assign.dst.range.size());
        }
        const auto &src1 = source_classified.get_src(1).begin()->src.container;
        return Result(new BitmaskedSet(dest, src1, dest, mask));
    } else if (source_classified.n_sources() == 2) {
        const auto *src1 = &source_classified.get_src(1);
        const auto *src2 = &source_classified.get_src(2);
        // always use src2 as background, i.e., prefer src2 to be the same container as dest.
        if (!src1->front().src.is_ad_or_const && src1->front().src.container == dest) {
            std::swap(src1, src2);
        }
        // all bits in destination that will be set.
        bitvec set_bits;
        for (const auto &assign : assigns.at(0)) {
            set_bits.setrange(assign.dst.range.lo, assign.dst.range.size());
        }
        const auto &live_bits = specs_i.at(dest).live;
        for (int i = 0; i < specs_i.at(dest).size; i++) {
            if (live_bits[i] && !set_bits[i]) {
                // if there are live bits that are not set by assignments in this action,
                // then destination needs to be the same container as src2.
                if (src2->front().src.container != dest) {
                    err_msg << "invalid bitmasked-set, because dest[" << i
                            << "] is overwritten unexpectedly";
                    return Result(Error(ErrorCode::unsat, err_msg.str()));
                }
            }
        }
        // mask computed from src1
        bitvec mask;
        for (const auto &assign : *src1) {
            mask.setrange(assign.dst.range.lo, assign.dst.range.size());
        }
        return Result(
            new BitmaskedSet(dest, src1->front().src.container, src2->front().src.container, mask));
    } else {
        err_msg << "destination " << dest
                << " has too many sources: " << source_classified.n_sources();
        return Result(Error(ErrorCode::too_many_container_sources, err_msg.str()));
    }
}

const RotateClassifiedAssigns *ActionMoveSolver::apply_unallocated_src_optimization(
    const ContainerID dest, const RotateClassifiedAssigns &offset_assigns) {
    // optimization requirement: must have unallocated phv source and ad_or_const source.
    if (src_unallocated_bits.at(dest).empty() || !offset_assigns.count(0)) {
        return &offset_assigns;
    }
    const SourceClassifiedAssigns aligned_assigns = classify_by_sources(offset_assigns.at(0));
    if (aligned_assigns.ad_or_const.empty()) {
        return &offset_assigns;
    }
    // decide offset and container.
    int offset = 0;
    std::optional<ContainerID> c = std::nullopt;
    if (!aligned_assigns.containers.empty()) {
        offset = 0;
        c = aligned_assigns.containers.begin()->first;
    } else if (offset_assigns.size() > 1) {
        const auto next = std::next(offset_assigns.begin());
        offset = next->first;
        c = next->second.front().src.container;
    }
    const int dest_sz = specs_i.at(dest).size;
    if (!c.has_value()) {
        c = ContainerID("$unallocated");
        specs_i[*c] = ContainerSpec{dest_sz, bitvec()};
    }
    auto *rst = new RotateClassifiedAssigns(offset_assigns);
    for (const int idx : src_unallocated_bits.at(dest)) {
        const auto dst_bit = make_container_operand(dest, StartLen(idx, 1));
        const auto src_bit =
            make_container_operand(*c, StartLen(right_rotate(idx, offset, dest_sz), 1));
        (*rst)[offset].push_back({dst_bit, src_bit});
    }
    src_unallocated_bits.at(dest).clear();
    LOG3("unallocated_src_optimization applied on " << dest);
    return rst;
}

Result ActionMoveSolver::solve() {
    if (auto err = validate_input()) {
        return Result(*err);
    }
    std::vector<const Instruction *> instructions;
    for (const auto &kv : dest_assigns_i) {
        ErrorCode code = ErrorCode::unsat;
        std::stringstream ss;
        ContainerID dest = kv.first;
        const auto &offset_assigns = *apply_unallocated_src_optimization(dest, kv.second);
        if (offset_assigns.empty()) {
            continue;
        }
        Result rst;
        // we did not try container set here because it can be considered as a special
        // case of deposit-field.
        // try deposit-field, most common.
        rst = try_deposit_field(dest, offset_assigns);
        if (!rst.ok()) {
            ss << "1. cannot be synthesized by deposit-field: " << rst.err->msg << ";";
            // prefer to return deposit-field error code because it's more common.
            code = rst.err->code;
        } else {
            instructions.push_back(rst.instructions.front());
            LOG5("synthesized with deposit-field: " << rst.instructions.front()->to_cstring());
            continue;
        }
        // then byte rotate merge for rare cases.
        rst = try_byte_rotate_merge(dest, offset_assigns);
        if (!rst.ok()) {
            ss << " 2. cannot be synthesized by byte-rotate-merge: " << rst.err->msg << ";";
        } else {
            instructions.push_back(rst.instructions.front());
            LOG5("synthesized with byte-rotate-merge: " << rst.instructions.front()->to_cstring());
            continue;
        }
        // bitmasked-set as the least preffered choice because it uses action data bus.
        if (enable_bitmasked_set_i) {
            rst = try_bitmasked_set(dest, offset_assigns);
            if (!rst.ok()) {
                ss << " 3. cannot be synthesized by bitmasked-set: " << rst.err->msg << ";";
            } else {
                LOG5("synthesized with bitmasked-set: " << rst.instructions.front()->to_cstring());
                continue;
            }
        }
        return Result(Error(code, ss.str()));
    }
    return Result(instructions);
}

/// solve checks that either
Result ActionMochaSolver::solve() {
    if (auto err = check_whole_container_set_with_none_source_allocated()) {
        return Result(*err);
    }
    std::vector<const Instruction *> instructions;
    for (const auto &dest_assigns : dest_assigns_i) {
        auto rst = try_container_set(dest_assigns.first, dest_assigns.second);
        if (rst.ok()) {
            LOG5("synthesized with container-set: " << rst.instructions.front()->to_cstring());
            instructions.push_back(rst.instructions.front());
        } else {
            return rst;
        }
    }
    return Result(instructions);
}

/// (1) sources are of one container: all aligned.
/// Also, we need to ensure that other allocated bits in the container will not be corrupted,
/// and since set instruction on mocha/dark are whole-container-set, all other bits that
/// are not set in this action need to be not live.
Result ActionDarkSolver::solve() {
    if (auto err = check_whole_container_set_with_none_source_allocated()) {
        return Result(*err);
    }
    std::stringstream ss;
    std::vector<const Instruction *> instructions;
    for (const auto &dest_assigns : dest_assigns_i) {
        const auto &dest = dest_assigns.first;
        const auto &offset_assigns = dest_assigns.second;
        auto rst = try_container_set(dest, offset_assigns);
        if (rst.ok()) {
            const auto &source_classified = classify_by_sources(offset_assigns.at(0));
            if (!source_classified.ad_or_const.empty()) {
                ss << "dark container destination " << dest
                   << " has ad/const source: " << offset_assigns.at(0);
                return Result(Error(ErrorCode::dark_container_ad_or_const_source, ss.str()));
            }
            LOG5("synthesized with container-set: " << rst.instructions.front()->to_cstring());
            instructions.push_back(rst.instructions.front());
        } else {
            return rst;
        }
    }
    return Result(instructions);
}

std::ostream &operator<<(std::ostream &s, const Operand &c) {
    if (c.is_ad_or_const) {
        s << "ad_or_const";
    } else {
        s << c.container << " " << c.range;
    }
    return s;
}

std::ostream &operator<<(std::ostream &s, const Assign &c) {
    s << c.dst << " = " << c.src;
    return s;
}

std::ostream &operator<<(std::ostream &s, const std::vector<Assign> &c) {
    s << "{\n";
    for (const auto &v : c) {
        s << "\t" << v << "\n";
    }
    s << "}";
    return s;
}

}  // namespace solver
