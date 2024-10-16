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

#ifndef BF_P4C_PHV_V2_COPACKER_H_
#define BF_P4C_PHV_V2_COPACKER_H_

#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {
namespace v2 {

/// SrcPackVec represents a way of how to pack and allocate a set of source field slices
/// to a source container.
struct SrcPackVec {
    /// pack to this container.
    std::optional<PHV::Container> container = std::nullopt;

    /// field slices and their starting indexes.
    FieldSliceAllocStartMap fs_starts = {};

    /// true if it is okay to left shift (increase starting index) for fields in fs_starts.
    /// disabled because we have not found a good algorithm to generate shiftable candidates.
    // bool left_shiftable = false;

    /// @returns true the hint is empty.
    bool empty() const { return fs_starts.empty(); }

    /// @returns the number of field slices that needs to be packed in this hint.
    size_t size() const { return fs_starts.size(); }
};
std::ostream& operator<<(std::ostream&, const SrcPackVec&);
std::ostream& operator<<(std::ostream&, const SrcPackVec*);

/// a list of SrcPackVec that must either all be satisfied or none. For each action,
/// the packing in destination may create 1 or 2 hints, e.g., bitwise-op would
/// require two sources to aligned and packed in two container.
struct CoPackHint {
    /// 1 or 2 source packing vector.
    std::vector<SrcPackVec*> pack_sources;
    /// true if this packing is the only option and required.
    bool required = false;
    /// reason of this copack constraint.
    cstring reason = ""_cs;
};
std::ostream& operator<<(std::ostream&, const CoPackHint&);
std::ostream& operator<<(std::ostream&, const CoPackHint*);

/// Every action that writes to the destination may have a set of mutex packing hints.
/// The value is a list of hints that are mutually exclusive,
/// only one of them needs to be satisfied.
using ActionSourceCoPackMap = ordered_map<const IR::MAU::Action*, std::vector<const CoPackHint*>>;
std::ostream& operator<<(std::ostream&, const ActionSourceCoPackMap&);
std::ostream& operator<<(std::ostream&, const ActionSourceCoPackMap*);

/// Result type that either has some copack hints, or an error.
struct CoPackResult {
    ActionSourceCoPackMap action_hints;
    const AllocError* err = nullptr;
    bool ok() const { return err == nullptr; }
};

/// CoPacker is the method that compute and generates copack constraints.
class CoPacker {
    const ActionSourceTracker& sources_i;
    const SuperCluster* sc_i;
    const ScAllocAlignment* alignment_i;
    ordered_map<FieldSlice, const SuperCluster::SliceList*> fs_to_sl;

    /// map an alloc slice to its sources.
    using DestSrcsMap = ordered_map<AllocSlice, safe_vector<SourceOp>>;
    using DestSrcsKv = std::pair<AllocSlice, safe_vector<SourceOp>>;

    /// CoPackHintOrErr is either an error or one copack hint.
    struct CoPackHintOrErr {
        const AllocError* err = nullptr;
        const CoPackHint* hint = nullptr;
        bool ok() const { return err == nullptr; }
        explicit CoPackHintOrErr(const AllocError* err): err(err) {}
        explicit CoPackHintOrErr(const CoPackHint* c): hint(c) {}
    };

    /// @returns the container starting index if already decided.
    std::optional<int> get_decided_start_index(const FieldSlice& fs) const;

    /// @returns false if new_fs cannot be packed with @p existing slices.
    bool ok_to_pack(const FieldSliceAllocStartMap& existing,
                    const FieldSlice& new_fs,
                    const int new_fs_start_index,
                    const Container& c) const;

    /// @returns true if we should skip creating source packing the unallocated @p fs.
    bool skip_source_packing(const FieldSlice& fs) const;

    /// @returns a copack hint introduced by @p writes to @p c.
    /// NOTE: the returned copack constraints will not be marked as required,
    /// Premise:
    /// we require source operands (values of @p writes), the safe_vector<SourceOp>, to have
    /// the same order as in p4. e.g.,
    /// if we have a = b & c, then for AllocSlice a, the first SourceOp needs to be b and
    /// the second must be c, and it should apply for field slices as well.
    /// Values returned from ActionSourceTracker can meet this requirement.
    /// We use this order to group sources into two copack groups, and since this order is
    /// specified by P4 code, user can tweak them if needed.
    const CoPackHint* gen_bitwise_copack(const Allocation& allocated_tx,
                                         const IR::MAU::Action* action,
                                         const DestSrcsMap& writes,
                                         const Container& c) const;

    /// @returns a copack hint introduced by @p writes in @p action, or an error if we know
    /// that we cannot pack sources together by checking ok_to_pack.
    /// NOTE: the returned copack constraints are *required*.
    CoPackHintOrErr gen_whole_container_set_copack(const Allocation& allocated_tx,
                                                   const IR::MAU::Action* action,
                                                   const DestSrcsMap& writes,
                                                   const Container& c) const;

    /// @returns a copack hint introduced by move-based instructions, i.e., @p writes, to @p c,
    /// in action @p action. This function only support deposit-field and bit-masked set.
    // The algorithm is
    /// (1) figure out facts from @p allocated_tx, if exists, including:
    ///     + aligned source container
    ///     + shiftable source container
    ///     + number of bits to shift for shiftable source container.
    /// (2) eagerly pack field to the aligned source, if ok_to_pack. If not, pack field to
    ///     the shiftable source container.
    CoPackHintOrErr gen_move_copack(const Allocation& allocated_tx,
                                    const IR::MAU::Action* action,
                                    const DestSrcsMap& writes,
                                    const Container& c) const;

 public:
    CoPacker(const ActionSourceTracker& sources,
             const SuperCluster* sc,
             const ScAllocAlignment* alignment);

    /// @returns CoPackResult introduced by allocated slices @p slices in container @p c.
    /// Premise:
    ///   1. @p alloc must be the allocation that *has* @p slices allocated already.
    ///   2. @p slices must be field slices of member variable sc_i.
    CoPackResult gen_copack_hints(const Allocation& allocated_tx,
                                  const std::vector<AllocSlice>& slices,
                                  const Container& c) const;
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_COPACKER_H_ */
