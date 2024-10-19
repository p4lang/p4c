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

#ifndef BF_P4C_PHV_V2_ALLOCATOR_BASE_H_
#define BF_P4C_PHV_V2_ALLOCATOR_BASE_H_

#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/allocator_metrics.h"
#include "bf-p4c/phv/v2/copacker.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {
namespace v2 {

/// ContScopeAllocResult extends the AllocResult with additional properties for
/// allocator to pick the best one.
/// 1. additional packing hints.
/// 2. allocated container.
/// 3. is this allocation result packing with other existing field slices.
struct ContScopeAllocResult : public AllocResult {
    ActionSourceCoPackMap action_hints;
    Container c;
    bool is_packing = false;
    ContScopeAllocResult() : AllocResult(new AllocError(ErrorCode::NOT_ENOUGH_SPACE, ""_cs)) {}
    explicit ContScopeAllocResult(const AllocError *err) : AllocResult(err) {}
    ContScopeAllocResult(const Transaction &tx, const ActionSourceCoPackMap &hints, Container c,
                         bool is_packing)
        : AllocResult(tx), action_hints(hints), c(c), is_packing(is_packing) {}
    bool cont_is_whole_container_set_only() const {
        BUG_CHECK(ok(), "cannot query this status on error");
        return c.type().kind() == Kind::mocha || c.type().kind() == Kind::dark;
    }
};

/// SomeContScopeAllocResult will store multiple ContScoreAllocResult(s) of the highest score under
/// different standards. They will be saved into the results vector and the index represents:
/// 0: highest score overall.
/// 1: highest score of not packing with any other field slices.
/// 2: highest score of not packing with any other field slices and not introducing
///    whole_container_set_only constraint. (i.e., not in mocha/dark container).
struct SomeContScopeAllocResult {
    const AllocError *err = nullptr;
    std::vector<const TxScore *> scores;
    std::vector<ContScopeAllocResult> results;
    explicit SomeContScopeAllocResult(const AllocError *default_err) : err(default_err) {}
    explicit SomeContScopeAllocResult(ContScopeAllocResult &rst, const TxScore *score)
        : scores{score}, results{rst} {}
    /// update the result list.
    void collect(const ContScopeAllocResult &rst, const TxScore *score);
    bool ok() const { return !results.empty(); }
    ContScopeAllocResult &front() { return results.front(); }
    size_t size() const { return results.size(); }
};

/// AllocatorBase contains all reusable functions for PHV allocation, mostly 3 categories:
/// (1) constraint checking functions that their names usually start with is_.
/// (2) helper functions starts with make_* or compute_*.
/// (3) allocation functions: const-qualified functions that returns an AllocResult.
class AllocatorBase {
 protected:
    const PhvKit &kit_i;

    /////////////////////////////////////////////////////////////////////////
    /// CONSTRAINT CHECKING Functions common to trivial and greedy allocation
    /////////////////////////////////////////////////////////////////////////
 protected:
    /// @returns error when @p sl cannot fit the container type constraint of @p c.
    const AllocError *is_container_type_ok(const AllocSlice &sl, const Container &c) const;

    /// @returns error when @p sl does not have the same gress assignment as @p c.
    const AllocError *is_container_gress_ok(const Allocation &alloc, const AllocSlice &sl,
                                            const Container &c) const;

    /// @returns error when there is the parser write mode of @p sl is different from other
    /// slices allocated in containers of the same parser group.
    const AllocError *is_container_write_mode_ok(const Allocation &alloc, const AllocSlice &sl,
                                                 const Container &c) const;

    /// @returns error when trying to pack a field with solitary constraint with other fields.
    const AllocError *is_container_solitary_ok(const Allocation &alloc, const AllocSlice &candidate,
                                               const Container &c) const;

    /// @returns error if violating max container bytes constraints on some field.
    const AllocError *is_container_bytes_ok(const Allocation &alloc,
                                            const std::vector<AllocSlice> &candidates,
                                            const Container &c) const;

    /// @returns error if violating any of the above is_container_* constraint.
    const AllocError *check_container_scope_constraints(const Allocation &alloc,
                                                        const std::vector<AllocSlice> &candidates,
                                                        const Container &c) const;

    /// @returns a vector of slice list that covers all field slices of @p sc.
    /// SliceLists are sorted in the order of suggested allocation by
    /// (1) slice list with fixed alignment.
    std::vector<const SuperCluster::SliceList *> *make_alloc_order(const ScoreContext &ctx,
                                                                   const SuperCluster *sc,
                                                                   const PHV::Size width) const;

    /// @returns AllocError if can_pack verification failed.
    /// NOTE: when @p c is an empty normal container, a special error with
    /// code ACTION_CANNOT_BE_SYNTHESIZED will be returned.
    /// It indicates that even if we are allocating candidates to an empty container
    /// (assume all other container-scope constraint checks have passed), we still cannot pack
    /// or allocate candidates, which usually means that we need to slice the original super
    /// cluster, which has the slice list of @p candidates, differently, to avoid some packings.
    const AllocError *verify_can_pack(const ScoreContext &ctx, const Allocation &alloc,
                                      const SuperCluster *sc,
                                      const std::vector<AllocSlice> &candidates, const Container &c,
                                      ActionSourceCoPackMap &co_pack_hints) const;

    /// Generate pseudo AllocSlices for field slices that have not been allocated, but their
    /// allocation can be speculated upfront: when there is only one valid starting position.
    /// @returns a transaction that contains the pseudo AllocSlices.
    /// we can infer that field slices will be allocated to a container with corresponding
    /// starting positions. This will allow can_pack function to check constraints from
    /// action reading(writing) side, even if destination(source) has not been allocated yet.
    Transaction make_speculated_alloc(const ScoreContext &ctx, const Allocation &alloc,
                                      const SuperCluster *sc,
                                      const std::vector<AllocSlice> &candidates,
                                      const Container &candidates_cont) const;

    /// @returns a vector a starting postions for @p sl in @p width-sized containers.
    /// The starting positions might be sorted for more efficient searching, see implementation.
    std::vector<int> make_start_positions(const ScoreContext &ctx,
                                          const SuperCluster::SliceList *sl,
                                          const PHV::Size width) const;

    /// @returns a set of container sizes that are okay for @p sc.
    std::set<PHV::Size> compute_valid_container_sizes(const SuperCluster *sc) const;

    /// Try to allocate fieldslices with starting positions defined in @p fs_starts to container @p
    /// c. Various container-level constraints will be checked. When @p skip_mau_checks are true,
    /// mau-related checks will be skipped, e.g., verify_can_pack. It should only be true when
    /// allocator is trying to allocate stride clusters.
    /// Premises without BUG_CHECK:
    ///   (1) @p fs_starts contains all field slices that needs to be allocated to @p c. They
    ///       should never exceed the width of the container,
    ///       i.e., max_i(start_i + size_i) < c.width().
    ///   (2) allocating to @p c will not violate pa_container_size pragma specified on
    ///       field slices in @p fs_starts:
    ///   (3) if there were wide_arithmetic slices, if lo(hi), @p c must have even(odd) index. Also
    ///       caller needs to allocate them to adjacent even-odd pair containers.
    ///   (4) if deparsed/exact_container, total number of bits must be equal to the width of @p c.
    ///   (5) field slices of an aligned cluster, including slices that have already been
    ///       allocated in @p alloc, and slices to be allcoated in @p fs_starts,
    ///       must have the same starting index in container. This function, and all the functions
    ///       this function will invoke, will not check this constraint. Failed to respect this
    ///       rule will get an action analysis error in later passes.
    /// @returns error in AllocResult if
    ///   (1) not enough space:
    ///      <1> non-mutex bits occupied or has non-mutex solitary field.
    ///      <2> uninitialized read + extracted.
    ///   (2) cannot pack into container.
    ///   (3) when container is mocha/dark/tphv, not all field slices are be valid
    ///       to be allocated to the container type of @p c.
    ///   (4) violate pa_container_type.
    ///   (5) container gress match
    ///   (6) parserGroupGress match, all containers must have same write mode.
    ///   (7) deparser group must be the same.
    ///   (8) solitary fields are not packed with other fields except for paddings fields.
    ///   (9) fields in @p fs_starts will not violate parser extraction constraints.
    ///   (5) field max container bytes constraints.
    /// NOTE: alloc slices of ignore_alloc field slices will not be generated.
    /// Possible ErrorCode:
    /// (1) NOT_ENOUGH_SPACE
    /// (2) ACTION_CANNOT_BE_SYNTHESIZED
    /// (3) *all kinds of container scope static error codes, e.g. gress, container type mismatch..
    ContScopeAllocResult try_slices_to_container(const ScoreContext &ctx, const Allocation &alloc,
                                                 const FieldSliceAllocStartMap &fs_starts,
                                                 const Container &c,
                                                 AllocatorMetrics &alloc_metrics,
                                                 const bool skip_mau_checks = false) const;

    /// try to find a valid container in @p group for @p fs_starts by calling
    /// try_slices_to_container on every container of @p group. The returned result
    /// will be the container that has the highest score based on ctx.score()->make().
    /// Premises without BUG_CHECK:
    ///   (1) AllocSlices that will be generated based on @p fs_starts will not exceed
    ///       the size of width of @p group.
    ///   (2) @p ctx score has been initialized.
    /// Possible ErrorCode:
    /// (1) NOT_ENOUGH_SPACE
    /// (2) ACTION_CANNOT_BE_SYNTHESIZED
    /// Pruning container group to improve speedup
    /// - Skip containers of the same type (kind and size) if an empty container cannot be allocated
    /// - Skip containers of the same equivalence class which have been tried before
    /// NOTE: Above pruning improves phv compilation times by 10x in some cases
    SomeContScopeAllocResult try_slices_to_container_group(const ScoreContext &ctx,
                                                           const Allocation &alloc,
                                                           const FieldSliceAllocStartMap &fs_starts,
                                                           const ContainerGroup &group,
                                                           AllocatorMetrics &alloc_metrics) const;

    /// A helper function that will call try_slices_to_container if @p c is specified (not
    /// std::nullopt). Otherwise, it will call try_slices_to_container_group with @p group.
    SomeContScopeAllocResult try_slices_adapter(const ScoreContext &ctx, const Allocation &alloc,
                                                const FieldSliceAllocStartMap &fs_starts,
                                                const ContainerGroup &group,
                                                std::optional<Container> c,
                                                AllocatorMetrics &alloc_metrics) const;

    /// Try to allocate by @p action_hints to @p group. This function will add allocated field
    /// slices to @p allocated.
    std::optional<Transaction> try_hints(const ScoreContext &ctx, const Allocation &alloc,
                                         const ContainerGroup &group,
                                         const ActionSourceCoPackMap &action_hints_map,
                                         ordered_set<PHV::FieldSlice> &allocated,
                                         ScAllocAlignment &hint_enforced_alignments,
                                         AllocatorMetrics &alloc_metrics) const;

    /// try to allocate a pair of wide_arith slice lists (@p lo, @p hi) to an even-odd
    /// pair of containers in @p group.
    AllocResult try_wide_arith_slices_to_container_group(
        const ScoreContext &ctx, const Allocation &alloc, const ScAllocAlignment &alignment,
        const SuperCluster::SliceList *lo, const SuperCluster::SliceList *hi,
        const ContainerGroup &group, AllocatorMetrics &alloc_metrics) const;

    /// try to allocate @p sc with @p alignment to @p group.
    /// premise:
    /// (1) alloc_alignment must have been populated in @p ctx.
    AllocResult try_super_cluster_to_container_group(const ScoreContext &ctx,
                                                     const Allocation &alloc,
                                                     const SuperCluster *sc,
                                                     const ContainerGroup &group,
                                                     AllocatorMetrics &alloc_metrics) const;

    /// internal type of callback.
    using DfsAllocCb = std::function<bool(const Transaction &)>;
    struct DfsState {
        ScAllocAlignment alignment;
        std::vector<const SuperCluster::SliceList *> allocated;
        std::vector<const SuperCluster::SliceList *> to_allocate;
        DfsState(const ScAllocAlignment &alignment,
                 const std::vector<const SuperCluster::SliceList *> &allocated,
                 const std::vector<const SuperCluster::SliceList *> &to_allocate)
            : alignment(alignment), allocated(allocated), to_allocate(to_allocate) {}
        bool done() const { return to_allocate.empty(); }
        const SuperCluster::SliceList *next_to_allocate() const { return to_allocate.front(); }
        DfsState next_state(
            const ScAllocAlignment &updated_alignment,
            const ordered_set<const SuperCluster::SliceList *> &just_allocated) const;
    };
    class DfsListsAllocator {
        const AllocatorBase &base;
        const int n_step_limit;
        DfsAllocCb yield;
        int n_steps = 0;
        bool caller_pruned = false;               // pruned by caller.
        const AllocError *deepest_err = nullptr;  // will not be nullptr unless succeeded.
        int deepest_depth = -1;
        bool pruned() const { return caller_pruned || n_steps > n_step_limit; }
        std::string depth_prefix(const int depth) const;
        std::optional<ScAllocAlignment> new_alignment_with_start(
            const ScoreContext &ctx, const SuperCluster::SliceList *target, const int sl_start,
            const PHV::Size &width, const ScAllocAlignment &alignment) const;
        bool allocate(const ScoreContext &ctx, const Transaction &tx, const DfsState &state,
                      const int depth, AllocatorMetrics &alloc_metrics);

     public:
        DfsListsAllocator(const AllocatorBase &base, int n_step_limit)
            : base(base), n_step_limit(n_step_limit) {}
        bool search_allocate(const ScoreContext &ctx, const Transaction &tx,
                             const ScAllocAlignment &alignment,
                             const std::vector<const SuperCluster::SliceList *> &allocated,
                             const std::vector<const SuperCluster::SliceList *> &to_allocate,
                             const DfsAllocCb &yield, AllocatorMetrics &alloc_metrics);
        const AllocError *get_deepest_err() const { return deepest_err; }
    };

    AllocResult alloc_stride(const ScoreContext &ctx, const Allocation &alloc,
                             const std::vector<FieldSlice> &stride,
                             const ContainerGroupsBySize &groups,
                             AllocatorMetrics &alloc_metrics) const;

 public:
    explicit AllocatorBase(const PhvKit &kit) : kit_i(kit) {
        kit_i.parser_packing_validator->set_trivial_pass(kit.settings.trivial_alloc);
    };
    virtual ~AllocatorBase(){};

    /////////////////////////////////////////////////////////////////
    /// ALLOCATION functions common to trivial and greedy allocation
    /////////////////////////////////////////////////////////////////

    /// Try to allocate @p sc, without further slicing, to @p groups.
    /// Premise:
    /// (1) DO NOT pass fully clot-allocated clusters to this function. If does, fields will
    ///     be double-allocated.
    /// (2) DO NOT pass deparser-zero-candidate cluster to this function, because they will not be
    ///     allocated to zero containers, Unless the correctness of their allocation does not
    ///     matter, e.g., in trivial allocator, as they are not referenced in MAU.
    AllocResult try_sliced_super_cluster(const ScoreContext &ctx, const Allocation &alloc,
                                         const SuperCluster *sc,
                                         const ContainerGroupsBySize &groups,
                                         AllocatorMetrics &alloc_metrics) const;

    /// This function will always successfully allocate fieldslices of @p sc to deparser zero
    /// optimization containers, which are B0 for ingress, B16 for egress. phv.addZeroContainer()
    /// will be called to add used deparser-zero containers.
    /// NOTE: if @p sc is fully-clot allocated, it will be ignored.
    /// premise:
    /// (1) @p sc must be deparser-zero optimization candidate.
    PHV::Transaction alloc_deparser_zero_cluster(const ScoreContext &ctx,
                                                 const PHV::Allocation &alloc,
                                                 const PHV::SuperCluster *sc, PhvInfo &phv) const;

    /// Try to allocate stride super cluster @p sc, without further slicing, to @p groups.
    /// Premise:
    /// (1) DO NOT pass non-strided super cluster to this function.
    AllocResult alloc_strided_super_clusters(const ScoreContext &ctx, const Allocation &alloc,
                                             const SuperCluster *sc,
                                             const ContainerGroupsBySize &groups,
                                             AllocatorMetrics &alloc_metrics,
                                             const int max_n_slicings = 64) const;
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_ALLOCATOR_BASE_H_ */
