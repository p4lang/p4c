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

#include "bf-p4c/phv/v2/copacker.h"

#include <optional>

#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/v2/utils_v2.h"
#include "lib/safe_vector.h"

namespace PHV {
namespace v2 {

namespace {

/// @returns the AllocSlice of @p fs in @p tx.
std::optional<AllocSlice> find_allocated(const Allocation &tx, const FieldSlice &fs) {
    for (const auto &sl : tx.getStatus(fs.field())) {
        if (sl.field_slice() == fs.range()) {
            return sl;
        } else {
            BUG_CHECK(!sl.field_slice().overlaps(fs.range()), "not fine sliced: %1%", fs);
        }
    }
    return std::nullopt;
}

int n_wrap_around_right_shift_bits(const int from, const int to, const int size) {
    const int offset = from - to;
    BUG_CHECK(offset >= 0 || offset + size > 0, "wrap around more than 1 round is not allowed");
    return offset >= 0 ? offset : offset + size;
}

}  // namespace

std::ostream &operator<<(std::ostream &out, const SrcPackVec &v) {
    if (v.container) {
        out << *v.container << ":";
    }
    out << v.fs_starts;
    return out;
}

std::ostream &operator<<(std::ostream &out, const SrcPackVec *v) {
    if (!v)
        out << "null";
    else
        out << *v;
    return out;
}

std::ostream &operator<<(std::ostream &out, const CoPackHint &v) {
    std::string sep = "";
    for (const auto *src : v.pack_sources) {
        out << sep << src;
        sep = ", ";
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const CoPackHint *v) {
    if (!v)
        out << "null";
    else
        out << *v;
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionSourceCoPackMap &v) {
    std::string sep = "";
    for (const auto &action_hints : v) {
        out << sep << action_hints.first->externalName() << ":{";
        for (const auto *copack : action_hints.second) {
            out << copack;
        }
        out << "}";
        sep = "\n";
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionSourceCoPackMap *v) {
    if (!v)
        out << "null";
    else
        out << *v;
    return out;
}

CoPacker::CoPacker(const ActionSourceTracker &sources, const SuperCluster *sc,
                   const ScAllocAlignment *alignment)
    : sources_i(sources), sc_i(sc), alignment_i(alignment) {
    for (const auto *sl : sc->slice_lists()) {
        for (const auto &fs : *sl) {
            fs_to_sl[fs] = sl;
        }
    }
}

std::optional<int> CoPacker::get_decided_start_index(const FieldSlice &fs) const {
    const auto *aligned = &sc_i->aligned_cluster(fs);
    if (alignment_i->cluster_starts.count(aligned)) {
        return alignment_i->cluster_starts.at(aligned);
    }
    return std::nullopt;
}

bool CoPacker::skip_source_packing(const FieldSlice &fs) const { return fs_to_sl.count(fs); }

/// @returns false if new_fs cannot be packed with @p existing slices.
bool CoPacker::ok_to_pack(const FieldSliceAllocStartMap &existing, const FieldSlice &new_fs,
                          const int new_fs_start_index, const Container &c) const {
    // container size check.
    if (new_fs_start_index + new_fs.size() > (int)c.size() || new_fs_start_index < 0) {
        return false;
    }
    // bit-in-byte alignment check.
    if (new_fs.alignment()) {
        const int alignment = new_fs.alignment()->align;
        if (new_fs_start_index % 8 != alignment) {
            return false;
        }
    }
    // container alignment check.
    if (auto decided_start_idx = get_decided_start_index(new_fs)) {
        if (*decided_start_idx != new_fs_start_index) {
            return false;
        }
    }
    // solitary check.
    if (!existing.empty()) {
        bool has_solitary = false;
        ordered_set<const PHV::Field *> all_fields{new_fs.field()};
        has_solitary |= new_fs.field()->is_solitary();
        /// TODO: do not need to filter out padding fields because they can
        /// never be referenced in actions.
        for (const auto &fs_index : existing) {
            const auto *f = fs_index.first.field();
            all_fields.insert(f);
            has_solitary |= f->is_solitary();
        }
        if (has_solitary && all_fields.size() > 1) {
            return false;
        }
    }
    return true;
}

const CoPackHint *CoPacker::gen_bitwise_copack(const Allocation &allocated_tx,
                                               const IR::MAU::Action *action,
                                               const DestSrcsMap &writes,
                                               const Container &c) const {
    static const size_t left_index = 0;
    static const size_t right_index = 1;
    // group sources by their index in @p writes.
    CoPackHint *rst = new CoPackHint();
    rst->pack_sources = {new SrcPackVec(), new SrcPackVec()};
    rst->reason = "bitwise-op"_cs;
    LOG5("gen_bitwise_copack: " << action);
    for (const auto &dest_sources : writes) {
        const auto &dest = dest_sources.first;
        const auto &sources = dest_sources.second;
        // TODO: copack do not support conditionally-set for now,
        // which is treated as bitwise with 3 sources.
        if (sources.size() == 3) {
            continue;
        }
        BUG_CHECK(sources.size() <= 2, "more than 2 sources bit-wise op in %1%", action);
        for (const auto i : {left_index, right_index}) {
            if (i >= sources.size()) break;
            const auto &src = sources[i];
            if (!src.phv_src) continue;
            const auto &src_fs = *src.phv_src;
            const auto src_alloc = find_allocated(allocated_tx, src_fs);
            if (src_alloc) {
                // assign container to corresponding source if allocated.
                if (rst->pack_sources[i]->container &&
                    rst->pack_sources[i]->container != src_alloc->container()) {
                    LOG3("container conflict found for src "
                         << i << ": " << *rst->pack_sources[i]->container << "  and "
                         << src_alloc->container());
                    // if we cannot group sources by the order of source operands in p4,
                    // we do not generate any packing hints for bitwise operations.
                    return nullptr;
                }
                rst->pack_sources[i]->container = src_alloc->container();
            } else {
                if (skip_source_packing(src_fs)) continue;
                const auto decided_start_idx = get_decided_start_index(src_fs);
                // for bitwise, the allocator should always try aligned sources.
                BUG_CHECK(!decided_start_idx || *decided_start_idx == dest.container_slice().lo,
                          "unaligned bitwise allocation, destination: %1%, source idx: %2%",
                          dest.container_slice().lo, *decided_start_idx);
                // add packing constraints.
                if (ok_to_pack(rst->pack_sources[i]->fs_starts, src_fs, dest.container_slice().lo,
                               c)) {
                    rst->pack_sources[i]->fs_starts[src_fs] = dest.container_slice().lo;
                } else {
                    LOG3("Failed to suggest packing for bitwise-op: cannot pack "
                         << src_fs << " with " << rst->pack_sources[i]->fs_starts);
                    return nullptr;
                }
            }
        }
    }

    // For bitwise operations, it is okay and helpful to return packing that contains only
    // one fieldslice. Because it makes allocator try source with alignments first.
    // remove empty packing requirement.
    std::vector<SrcPackVec *> filtered;
    for (const auto i : {left_index, right_index}) {
        if (!rst->pack_sources[i]->empty()) {
            filtered.push_back(rst->pack_sources[i]);
        }
    }
    rst->pack_sources = filtered;
    if (rst->pack_sources.empty()) {
        return nullptr;
    } else {
        LOG5("CoPack Hint: " << rst);
        return rst;
    }
}

CoPacker::CoPackHintOrErr CoPacker::gen_whole_container_set_copack(const Allocation &allocated_tx,
                                                                   const IR::MAU::Action *action,
                                                                   const DestSrcsMap &writes,
                                                                   const Container &c) const {
    LOG5("gen_whole_container_set_copack: " << action);
    auto *copack = new SrcPackVec();
    for (const auto &dest_src : writes) {
        const auto &dest = dest_src.first;
        BUG_CHECK(dest_src.second.size() == 1, "whole container set must have 1 source: %1%",
                  dest_src.second.size());
        const auto &src_op = dest_src.second.front();
        if (!src_op.phv_src) continue;
        const auto &src_fs = *src_op.phv_src;
        std::optional<AllocSlice> src_alloc = find_allocated(allocated_tx, src_fs);
        if (!src_alloc) {
            if (skip_source_packing(src_fs)) continue;
            auto *err = new AllocError(ErrorCode::INVALID_ALLOC_FOUND_BY_COPACKER);
            const auto decided_start_idx = get_decided_start_index(src_fs);
            if (decided_start_idx && *decided_start_idx != dest.container_slice().lo) {
                *err << "action " << action->externalName() << " requires " << *src_op.phv_src
                     << " to be allocated to " << dest.container_slice().lo
                     << ", but it's impossible, because the slice must be allocated to "
                     << *decided_start_idx;
                return CoPackHintOrErr(err);
            }
            if (!ok_to_pack(copack->fs_starts, *src_op.phv_src, dest.container_slice().lo, c)) {
                *err << "action " << action->externalName() << " requires " << *src_op.phv_src
                     << " to be packed with " << copack->fs_starts << ", but it is impossible.";
                return CoPackHintOrErr(err);
            }
            copack->fs_starts[*src_op.phv_src] = dest.container_slice().lo;
        } else {
            BUG_CHECK(!copack->container || *copack->container == src_alloc->container(),
                      "Two source container for mocha/dark destination is impossible, "
                      "should have been caught in can_pack_v2()");
            copack->container = src_alloc->container();
        }
    }
    if (copack->empty()) return CoPackHintOrErr((CoPackHint *)nullptr);
    // For whole-container-set, it is okay and helpful to return packing that contains only
    // one fieldslice. Because it makes allocator try a source with fixed alignment first.
    auto *rst = new CoPackHint();
    rst->required = true;
    rst->reason = "whole-container-set"_cs;
    rst->pack_sources = {copack};
    return CoPackHintOrErr(rst);
}

CoPacker::CoPackHintOrErr CoPacker::gen_move_copack(const Allocation &allocated_tx,
                                                    const IR::MAU::Action *action,
                                                    const DestSrcsMap &writes,
                                                    const Container &c) const {
    for (const auto &dest_sources : writes) {
        const auto &sources = dest_sources.second;
        BUG_CHECK(sources.size() == 1, "more than 1 source for move-based op in %1%", action);
    }
    LOG5("gen move copack for " << action);
    // NOTE: we do not generate hints that leverage byte-rotate-merge: it is too complicated.
    // So, the returned hints are not *required* in any case. TODO: we can make some
    // of them required if byte-rotate-merge is not possible.
    // Algorithm: use deposit-field and bitmasked set to synthesize the move.
    // If there were unaligned allocated source slices, save the shift offset and container.
    // For aligned sources, save them to aligned sources. When there is already a different
    // container for aligned source, save it to shiftable source.
    // If there is any action has action or const source, save all allocated and unallocated
    // fields to the aligned source.
    const bool has_adc = std::any_of(writes.begin(), writes.end(), [](const DestSrcsKv &kv) {
        return kv.second.front().ad_or_const;
    });
    SrcPackVec *const aligned_src_pack = new SrcPackVec();
    SrcPackVec *const shiftable_src_pack = has_adc ? nullptr : new SrcPackVec();
    std::optional<int> shiftable_src_shift_bits = std::nullopt;
    // Go through all allocated sources to set container and the number of bits to shift.
    for (const auto &dest_sources : writes) {
        const auto &dest = dest_sources.first;
        const auto &src = dest_sources.second.front();
        if (!src.phv_src) continue;
        const auto &src_fs = *src.phv_src;
        int src_start_idx;
        std::optional<Container> src_container = std::nullopt;
        if (const auto src_alloc = find_allocated(allocated_tx, src_fs)) {
            src_container = src_alloc->container();
            src_start_idx = src_alloc->container_slice().lo;
        } else {
            /// when the source has not been allocated yet, we can still check if its alignment
            /// has been decided or not. If decided, we can use this value to update shiftable bits.
            if (auto start_idx = get_decided_start_index(src_fs)) {
                src_start_idx = *start_idx;
            } else {
                continue;
            }
        }
        LOG5("Found move (dest, src) pair: "
             << dest << " = " << src_fs.shortString() << "@"
             << (src_container != std::nullopt ? cstring::to_cstring(*src_container) : "*") << "["
             << src_start_idx << ":" << src_start_idx + src_fs.size() - 1 << "]");
        // compute number of bits that source will be right shifted, wrap-around considered.
        const int this_src_right_shift_bits =
            n_wrap_around_right_shift_bits(src_start_idx, dest.container_slice().lo, c.size());
        // unaligned source allocated.
        if (this_src_right_shift_bits != 0) {
            // skip if they may only be possible through byte-rotate-merge.
            // case.1: has action data and unaligned.
            if (has_adc) {
                LOG3("Skip, src needs to be shifted but there are action_data/const sources");
                return CoPackHintOrErr((CoPackHint *)nullptr);
            }
            // case.2: two different shift offsets.
            if (shiftable_src_shift_bits &&
                *shiftable_src_shift_bits != this_src_right_shift_bits) {
                LOG3("Skip, different number of bits to shift for allocated sources, seen: "
                     << *shiftable_src_shift_bits << ", this: " << this_src_right_shift_bits);
                return CoPackHintOrErr((CoPackHint *)nullptr);
            }
            // case.3: two unaligned sources from diffrent container.
            if (shiftable_src_pack->container && src_container &&
                *shiftable_src_pack->container != *src_container) {
                LOG3("Skip, different container found for allocated and aligned sources, seen: "
                     << *shiftable_src_pack->container << " this: " << *src_container);
                return CoPackHintOrErr((CoPackHint *)nullptr);
            }
            // set number of bits to shift for shiftable source.
            shiftable_src_shift_bits = this_src_right_shift_bits;
            if (src_container) {
                LOG5("set shiftabe source pack: " << *src_container);
                // set source container of shiftable source.
                shiftable_src_pack->container = *src_container;
            }
        } else {
            // when this is an aligned and allocated src, it is tricky: it can either be in the
            // aligned source container, or shiftable source container with 0-bit offset.
            if (aligned_src_pack->container && src_container) {
                if (*aligned_src_pack->container != *src_container) {
                    BUG_CHECK(shiftable_src_pack,
                              "action data with two different container sources");
                    BUG_CHECK(!shiftable_src_pack->container ||
                                  shiftable_src_pack->container == src_container,
                              "3 different containers found for allocated sources.");
                    // case: this is an aligned source from C1, we have seen another source
                    // field in C2, and in C1, there is at least one other field that shift
                    // differently than this source:
                    // C1: {f1: shift *shiftable_src_shift_bits(not zero)}, {this field: shift 0};
                    // C2: all aligned sources.
                    BUG_CHECK(!shiftable_src_shift_bits || *shiftable_src_shift_bits == 0,
                              "3 different (container, shift) pairs are not possible even for "
                              "byte-rotate-merge.");
                    shiftable_src_pack->container = *src_container;
                    shiftable_src_shift_bits = 0;
                }
            } else {
                // assign this container to be the container of aligned source.
                if (src_container) {
                    aligned_src_pack->container = *src_container;
                }
            }
        }
    }
    if (LOGGING(5)) {
        if (aligned_src_pack->container) {
            LOG5("aligned_src_pack container: " << *aligned_src_pack->container);
        }
        if (shiftable_src_pack && shiftable_src_pack->container) {
            LOG5("shiftable_src_pack container: " << *shiftable_src_pack->container);
        }
        if (shiftable_src_pack && shiftable_src_shift_bits) {
            LOG5("shiftable_src_shift_bits: " << *shiftable_src_shift_bits);
        }
    }

    // We have learned everthing we need about allocated slices, and now we will try to
    // pack unallocated slices. For now, we will just generate the simplest one:
    // try to pack all fields to the aligned source, unless we found packing violation.
    // TODO: could we generate errors when allocation failed?
    // auto* err = new AllocError(ErrorCode::INVALID_ALLOC_FOUND_BY_COPACKER);
    for (const auto &dest_sources : writes) {
        const auto &dest = dest_sources.first;
        const auto &src = dest_sources.second.front();
        if (!src.phv_src) continue;
        const auto &src_fs = *src.phv_src;
        if (find_allocated(allocated_tx, src_fs)) continue;
        if (skip_source_packing(src_fs)) continue;
        LOG5("Try to pack: " << src_fs);

        /// check whether this unallocated slice has decided start index.
        if (auto start_idx = get_decided_start_index(src_fs)) {
            if (start_idx == dest.container_slice().lo &&
                ok_to_pack(aligned_src_pack->fs_starts, src_fs, dest.container_slice().lo, c)) {
                LOG5("packed to aligned source at " << dest.container_slice().lo);
                // aligned start index, try to pack to aligned source first.
                aligned_src_pack->fs_starts[src_fs] = dest.container_slice().lo;
            } else {
                // not aligned or cannot be packed to aligned source, try shiftable source.
                const int decided_shift =
                    n_wrap_around_right_shift_bits(*start_idx, dest.container_slice().lo, c.size());
                if (shiftable_src_pack &&
                    (!shiftable_src_shift_bits || *shiftable_src_shift_bits == decided_shift)) {
                    if (ok_to_pack(shiftable_src_pack->fs_starts, src_fs, *start_idx, c)) {
                        LOG5("packed to shiftable source at " << *start_idx);
                        shiftable_src_pack->fs_starts[src_fs] = *start_idx;
                        shiftable_src_shift_bits = decided_shift;
                    }
                }
            }
        } else {
            // fs does not have decided starting position, so we try to pack it in the aligned
            // source or in the shiftable source with the same amount of shift we decided.
            if (ok_to_pack(aligned_src_pack->fs_starts, src_fs, dest.container_slice().lo, c)) {
                LOG5("packed to aligned source at " << dest.container_slice().lo);
                aligned_src_pack->fs_starts[src_fs] = dest.container_slice().lo;
            } else if (shiftable_src_pack) {
                const int decided_shift =
                    (shiftable_src_shift_bits ? *shiftable_src_shift_bits : 0);
                const int shifted_start = dest.container_slice().lo + decided_shift;
                if (ok_to_pack(shiftable_src_pack->fs_starts, src_fs, shifted_start, c)) {
                    LOG5("packed to shiftable source at " << shifted_start);
                    shiftable_src_pack->fs_starts[src_fs] = shifted_start;
                    shiftable_src_shift_bits = decided_shift;
                }
            }
        }
    }
    auto *rst = new CoPackHint();
    rst->reason = "move-based"_cs;
    // empty and only one field slice source packing are ignored.
    if (!aligned_src_pack->empty() && (shiftable_src_pack && !shiftable_src_pack->empty())) {
        // if each has at least one field slice, then return packing.
        rst->pack_sources.push_back(aligned_src_pack);
        rst->pack_sources.push_back(shiftable_src_pack);
    } else {
        // otherwise, one of the source must have at least 2 field slices.
        // Packing with only 1 field slice in the source is not useful.
        if (aligned_src_pack->size() > 1) {
            rst->pack_sources.push_back(aligned_src_pack);
        }
        if (shiftable_src_pack && shiftable_src_pack->size() > 1) {
            rst->pack_sources.push_back(shiftable_src_pack);
        }
    }
    if (rst->pack_sources.empty()) {
        return CoPackHintOrErr((CoPackHint *)nullptr);
    }
    return CoPackHintOrErr(rst);
}

CoPackResult CoPacker::gen_copack_hints(const Allocation &allocated_tx,
                                        const std::vector<AllocSlice> &slices,
                                        const Container &c) const {
    // These are the actions that write to container *c*.
    // For every action, we will check if sources of the alloc slices
    // can be allocated to some container without violating constraints.
    ordered_map<const IR::MAU::Action *, DestSrcsMap> action_writes;
    ordered_map<const IR::MAU::Action *, SourceOp::OpType> action_type;
    for (const auto &alloc_sl : allocated_tx.liverange_overlapped_slices(c, slices)) {
        const FieldSlice fs(alloc_sl.field(), alloc_sl.field_slice());
        for (const auto &action_src : sources_i.get_sources(fs)) {
            const auto *act = action_src.first;
            const auto &src_vec = action_src.second;
            for (const auto &src : src_vec) {
                BUG_CHECK(!action_type.count(act) || action_type.at(act) == src.t,
                          "mixed op in action %1% for slice %2%", act, alloc_sl);
                action_type[act] = src.t;
                action_writes[act][alloc_sl].push_back(src);
            }
        }
    }

    const bool mocha_or_dark =
        c.type().kind() == PHV::Kind::mocha || c.type().kind() == PHV::Kind::dark;
    CoPackResult copack_rst;
    for (const auto &action_type : action_type) {
        const auto *action = action_type.first;
        switch (action_type.second) {
            case SourceOp::OpType::bitwise: {
                const auto *bitwise_hint =
                    gen_bitwise_copack(allocated_tx, action, action_writes.at(action), c);
                if (bitwise_hint) {
                    copack_rst.action_hints[action] = {bitwise_hint};
                }
                break;
            }
            case SourceOp::OpType::move: {
                if (mocha_or_dark) {
                    const auto whole_set_hint = gen_whole_container_set_copack(
                        allocated_tx, action, action_writes.at(action), c);
                    if (!whole_set_hint.ok()) {
                        copack_rst.err = whole_set_hint.err;
                        return copack_rst;
                    }
                    if (whole_set_hint.hint) {
                        copack_rst.action_hints[action] = {whole_set_hint.hint};
                    }
                } else {
                    const auto move_hint =
                        gen_move_copack(allocated_tx, action, action_writes.at(action), c);
                    if (!move_hint.ok()) {
                        copack_rst.err = move_hint.err;
                        return copack_rst;
                    }
                    if (move_hint.hint) {
                        copack_rst.action_hints[action] = {move_hint.hint};
                    }
                }
                break;
            }
            case SourceOp::OpType::whole_container: {
                // not supported yet.
                break;
            }
            default:
                BUG("unknown action type: %1%", int(action_type.second));
                break;
        }
    }
    return copack_rst;
}

}  // namespace v2
}  // namespace PHV
