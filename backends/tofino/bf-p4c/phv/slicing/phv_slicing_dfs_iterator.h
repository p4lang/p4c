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

#ifndef BF_P4C_PHV_SLICING_PHV_SLICING_DFS_ITERATOR_H_
#define BF_P4C_PHV_SLICING_PHV_SLICING_DFS_ITERATOR_H_

#include <utility>

#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/parde/check_parser_multi_write.h"
#include "bf-p4c/phv/slicing/phv_slicing_iterator.h"
#include "bf-p4c/phv/slicing/phv_slicing_split.h"
#include "bf-p4c/phv/slicing/types.h"
#include "bf-p4c/phv/utils/utils.h"
#include "lib/ordered_set.h"

namespace PHV {
namespace Slicing {

/// locate a slice list.
using SliceListLoc = std::pair<SuperCluster *, SuperCluster::SliceList *>;

/// constraints introduced on fieldslices of container sizes after splitting a slice list.
struct AfterSplitConstraint {
    // all possible container sizes, used for initialization.
    static const bitvec all_container_sizes;

    // ConstraintType abstracts this constraint.
    enum class ConstraintType {
        EXACT = 0,  // must be placed in container of the size.
        MIN = 1,    // must be placed in container at least the size.
        NONE = 2,   // no new constraint.
    };

    // default constructor
    AfterSplitConstraint() : sizes(all_container_sizes) {}
    // construct from set.
    explicit AfterSplitConstraint(const bitvec &sizes);
    // construct based on type.
    explicit AfterSplitConstraint(ConstraintType t, int v = 0);

    // available container sizes for this constraint.
    bitvec sizes;

    // return the type of this constraint.
    ConstraintType type() const;

    // return the smallest available container size;
    int min() const { return *sizes.begin(); }

    // returns the intersection of two AfterSplitConstraint.
    // e.g. MIN(8)   ^ EXACT(32) = EXACT(32)
    //      MIN(8)   ^ MIN(16)   = MIN(16)
    //      EXACT(8) ^ EXACT(16) = std::nullopt
    std::optional<AfterSplitConstraint> intersect(const AfterSplitConstraint &other) const;

    // return true if field with this AfterSplitConstraint can
    // be placed in a container of size @p n bits.
    bool ok(const int n) const;
};

std::ostream &operator<<(std::ostream &out, const AfterSplitConstraint &c);

/// map FieldSlices to AfterSplit constraints introduced by the slicing decisions.
using SplitDecision = ordered_map<FieldSlice, AfterSplitConstraint>;

/// SplitChoice for a slice list.
enum class SplitChoice {
    B = 8,
    H = 16,
    W = 32,
};

/// DfsItrContext implements the Slicing Iterator using DFS.
/// Because caller won't be using DfsItrContext directly(protected by pImpl),
/// to make white-box testing possible, all functions are public.
class DfsItrContext : public IteratorInterface {
 public:
    // input
    const PhvInfo &phv_i;
    const SuperCluster *sc_i;
    const PHVContainerSizeLayout pa_i;
    const ActionPackingValidatorInterface &action_packing_validator_i;
    const ParserPackingValidatorInterface &parser_packing_validator_i;
    const PackConflictChecker has_pack_conflict_i;
    const IsReferencedChecker is_used_i;
    IteratorConfig config_i;
    CheckWriteModeConsistency check_write_mode_consistency_i;

    // if a pa_container_size asks a field to be allocated to containers larger than it's
    // size, it's recorded here and will be used during pruning. Note that for one field,
    // there can only one upcasting piece, which is the tailing part.
    ordered_map<const PHV::Field *, std::pair<le_bitrange, int>> pa_container_size_upcastings_i;

    //// DFS states
    // split_decisions collects slicing decisions already made and
    // map fieldslice -> the after split constraints of the fieldslice.
    // The trick here is what we use a value, FieldSlice, representing that,
    // if a slicelist contains any fieldslice in this set, then the split decision
    // for that slicelist has already been made.
    assoc::hash_map<FieldSlice, AfterSplitConstraint> split_decisions_i;

    // slicelist_on_stack stores all slice list that were split during DFS in a stack-style.
    // and the number of choices left for the slice list to try different slicing.
    std::vector<const SuperCluster::SliceList *> slicelist_on_stack_i;

    // done_i is a set of super clusters that
    // (1) all slice slices are already split, or cannot be split further.
    // (2) the super cluster is well-formed, i.e. SuperCluster::is_well_formed() = true.
    ordered_set<SuperCluster *> done_i;

    // super clusters left to be sliced.
    ordered_set<SuperCluster *> to_be_split_i;

    // true if has itr generated before. one context can only generate one iterator.
    bool has_itr_i = false;

    // tracking dfs depth.
    int dfs_depth_i = 0;

    // a step counter records how many steps the search has tried.
    int n_steps_i = 0;

    // last solution was found at n_steps_since_last_solution before.
    int n_steps_since_last_solution = 0;

    // Set of rejected SplitChoice options from previous slice-lists
    std::set<SplitChoice> reject_sizes;

    // if not nullptr, backtrack to the stack that to_invalidate is not on stack,
    // i.e. not a part of the DFS path.
    const SuperCluster::SliceList *to_invalidate = nullptr;

    // When an action cannot be synthesized, either we can try to split the destination more,
    // or we try to split sources differently so that maybe then src and dest slices happen to
    // be aligned magically or two sources are packed together (very rare, but not impossible).
    // Then problem is that, we do not know which one to try first. So we always invalidate the
    // most recent choice in DFS. However,consider this:
    // dfs-1: incorrect decision on a destination field.
    // ....
    // dfs-99: finally made a decision on the source fields.
    // If we always revert what we have done in dsf-99, and retry, it will take 2^98 times to
    // make it backtrack to dfs-1, to correct the wrong.
    // So we set the counter here that as long as the number of times that a slice list is called
    // to be invalidated, but ignored, exceeds the max allowance, we will try to backtrack to that
    // slice list. Counter is cleared when we backtracked to the list.
    static constexpr int to_invalidate_max_ignore = 8;
    ordered_map<const SuperCluster::SliceList *, int> to_invalidate_sl_counter;

 public:
    DfsItrContext(const PhvInfo &phv, const MapFieldToParserStates &field_to_states,
                  const CollectParserInfo &parser_info, const SuperCluster *sc,
                  const PHVContainerSizeLayout &pa,
                  const ActionPackingValidatorInterface &action_packing_validator,
                  const ParserPackingValidatorInterface &parser_packing_validator,
                  const PackConflictChecker &pack_conflict, const IsReferencedChecker is_used)
        : phv_i(phv),
          sc_i(sc),
          pa_i(pa),
          action_packing_validator_i(action_packing_validator),
          parser_packing_validator_i(parser_packing_validator),
          has_pack_conflict_i(pack_conflict),
          is_used_i(is_used),
          config_i(false, false, true, true, false, (1 << 25), (1 << 19)),
          check_write_mode_consistency_i(phv, field_to_states, parser_info) {}

    /// iterate will pass valid slicing results to cb. Stop when cb returns false.
    void iterate(const IterateCb &cb) override;

    /// invalidate is the feedback mechanism for allocation algorithm to
    /// ask iterator not to produce slicing result contains @p sl. Caller can
    /// This DFS iterator will respect the list of top-most stack frame,
    /// i.e., the most recent decision made by DFS.
    void invalidate(const SuperCluster::SliceList *sl) override;

    /// set configs.
    void set_config(const IteratorConfig &cfg) override { config_i = cfg; }

    /// dfs search valid slicing. @p unchecked are superclusters that needs to be checked
    /// for pruning.
    bool dfs(const IterateCb &yield, const ordered_set<SuperCluster *> &unchecked);

    /// split_by_pa_container_size will split @p sc by @p pa container size.
    std::optional<std::list<SuperCluster *>> split_by_pa_container_size(
        const SuperCluster *sc, const PHVContainerSizeLayout &pa);

    /// split_by_adjacent_no_pack will split @p sc at byte boundary if two adjacent fields
    /// cannot be packed into one container.
    std::optional<std::list<SuperCluster *>> split_by_adjacent_no_pack(SuperCluster *sc) const;

    /// split_by_deparsed_bottom_bits will split at the beginning of deparsed_bottom_bits field.
    std::optional<std::list<SuperCluster *>> split_by_deparsed_bottom_bits(SuperCluster *sc) const;

    /// split_by_adjacent_deparsed_and_non_deparsed will split @p sc between deparsed and
    /// non-deparsed field.
    std::optional<std::list<SuperCluster *>> split_by_adjacent_deparsed_and_non_deparsed(
        SuperCluster *sc) const;

    /// split_by_valid_container_range will split based on valid container range constraint
    /// that a field cannot be packed fields after it, when its valid container range
    /// is equal to the size of the field.
    std::optional<std::list<SuperCluster *>> split_by_valid_container_range(SuperCluster *sc) const;

    /// split_by_long_fieldslices will split fieldslices that its length is greater or equal to
    /// 64 bits, using 32-bit container if possible.
    std::optional<std::list<SuperCluster *>> split_by_long_fieldslices(SuperCluster *sc) const;

    /// split_by_parser_write_mode will split based on incompatible parser write modes
    std::optional<std::list<SuperCluster *>> split_by_parser_write_mode(SuperCluster *sc);

    /// return possible SplitChoice on @p target.
    /// When minimal_packing_mode is false, results are sorted with a set of heuristics
    /// that choices with more packing opportunities (generally larger container-sized chunks),
    /// ,split at field boundaries, and better chances to split between ref/unref fields,
    /// are placed at lower indexes. See implementation for more details on heuristics.
    /// If minimal_packing_mode is true, then we will prefer to split with less packing
    /// of fieldslices.
    std::vector<SplitChoice> make_choices(const SliceListLoc &target) const;

    /// pruning strategies
    /// return true if found any unsatisfactory case. This function will return true
    /// if any of the following pruning strategies returns true.
    /// TODO: non-const because it may call invalidate();
    bool dfs_prune(const ordered_set<SuperCluster *> &unchecked);

    /// dfs_prune_unwell_formed: return true if
    /// (1) @p sc cannot be split further and is not well_formed.
    /// (2) a slicelist in @p sc that cannot be split further has pack conflicts.
    bool dfs_prune_unwell_formed(const SuperCluster *sc) const;

    bool dfs_prune_invalid_parser_packing(const SuperCluster *sc) const;

    /// return true if exists constraint unsat due to the limit of slice list size.
    bool dfs_prune_unsat_slicelist_max_size(const SplitDecision &constraints,
                                            const SuperCluster *sc) const;

    /// return true if constraints for a slice list cannot be *all* satisfied.
    /// Check for cases like:
    ///    32      xxx      16
    /// [fs1<8>, fs2<8>, fs3<16>]
    /// it's unsat because fs1 was decided to be allocated into 32-bit container
    /// while for fs3 it's 16-bit container. However, in the above layout
    /// it's impossible. We run a greedy algorithm looking for cases but may have
    /// false negatives.
    bool dfs_prune_unsat_slicelist_constraints(const SplitDecision &constraints,
                                               const SuperCluster *sc) const;

    /// return true if exists a metadata list that will join two
    /// exact_containers lists of different sizes. For example:;
    /// sl_1: [f1<16>, f2<8>, f3<8>[0:1], f3<8>[2:7]], total 32, exact.
    /// sl_2: [f2'<8>, f4<8>[0:3], f4<8>[4:7]], total 16, exact.
    /// sl_3: [md1<2>, pad<2>, md2<4>]
    /// rotational clusters:
    /// {f3[0:1], md1}, {f4<8>[0:3], md2}
    /// sl_3 will join sl_1 and sl_2 into one super cluster, and we can infer that
    /// this cluster is invalid because exact slice list sizes are not the same.
    bool dfs_prune_unsat_exact_list_size_mismatch(const SplitDecision &decided_sz,
                                                  const SuperCluster *sc) const;

    /// return true if there exists packing that make it impossible to
    /// to synthesize actions.
    /// TODO: non-const because it may call invalidate();
    bool dfs_prune_invalid_packing(const SuperCluster *sc);

    /// collect_aftersplit_constraints returns AfterSplitConstraints on the fieldslice
    /// of @p sc based on split_decisions_i and pa_container_size_upcastings_i.
    std::optional<SplitDecision> collect_aftersplit_constraints(const SuperCluster *sc) const;

    /// collect additional implicit container size constraint and save them to @p decided_sz,
    /// if it can be expressed. Otherwise, if will only check whether the implicit container
    /// size constraint can be satisfied.
    /// TODO: this function is created initially for the most common case of
    /// egress::eg_intr_md.egress_port that has
    /// (1) ^bit[0..15]; (2) no_split; (3) deparsed_bottom_bits
    /// because of these three constraints, this field can only be allocated to 16-bit container.
    /// There can be more special cases for these implicit (hard to be generalized).
    bool collect_implicit_container_sz_constraint(SplitDecision *decided_sz,
                                                  const SuperCluster *sc) const;

    /// dfs_pick_next return the next slice list to be split.
    /// There are some heuristics for returning the slicelist that has most constraints.
    std::optional<SliceListLoc> dfs_pick_next() const;

    /// propagate_8bit_exact_container_split propagates the 8bit split decision on @p target
    /// to other slice lists in @p sc, as long as there is one exact_containers field slices
    /// in rotational clusters of splitted slices of list.
    /// If we are splitting out an 8-bit slice list with
    /// exact_containers constraint, then we can infer that all other slices in the
    /// the same rotational cluster with slices that were just split out, need to
    /// be split by the bytes (counting from the beginning of lists) that contains them.
    void propagate_8bit_exact_container_split(SuperCluster *sc, SuperCluster::SliceList *target,
                                              SplitSchema *schema, SplitDecision *decisions) const;

    /// If we found that any field slice in the last byte of a slice list has a decided size,
    /// then we can split the *tail* out so that the packing is materialized as early as possible.
    bool propagate_tail_split(SuperCluster *sc, const SplitDecision &constraints,
                              const SplitDecision *decisions,
                              const SuperCluster::SliceList *just_split_target,
                              const int n_just_split_bits, SplitSchema *schema) const;

    /// make_split_meta will generate schema and decision to split out first @p first_n_bits
    /// of @p sl under @p sc.When a conflicting split decision is found, @returns std::nullopt.
    std::optional<std::pair<SplitSchema, SplitDecision>> make_split_meta(
        SuperCluster *sc, SuperCluster::SliceList *sl, int first_n_bits) const;

    /// return true if the slicelist needs to be further split.
    bool need_further_split(const SuperCluster::SliceList *sl) const;

    /// return true if there are pack_conflicts in @p sl.
    bool check_pack_conflict(const SuperCluster::SliceList *sl) const;

    /// get_well_formed_no_more_split returns super clusters that all the slice lists
    /// does not need_further_split, and the cluster is well_formed.
    std::vector<SuperCluster *> get_well_formed_no_more_split() const;

    // For some superclusters, slicing iterator is prone to generating duplicate slicing plans.
    // Duplicate slicing plans mean that although the slice lists have been sliced into a different
    // way, the slicing plan does not effectively change the result of PHV allocation. For example,
    // a supercluster may look like this:
    // slice lists:
    // [a[0-31]]
    // [b[0-31]]
    // rotational clusters:
    // [[a[0-31]], [b[0-31]]]
    //
    // one slicing plan can be:
    // slice lists:
    // [a[0-31]]
    // [b[0-31]]
    // rotational clusters:
    // [[a[0-31]], [b[0-31]]]
    // This slicing plan changes nothing.
    //
    // Another slicing plan can be:
    // slice lists:
    // [a[0-15], a[16-31]]
    // [b[0-15]]
    // [b[16-31]]
    // rotational clusters:
    // [[a[0-15]], [b[0-15]]]
    // [[a[16-31]], [b[16-31]]]
    // For the first slicing plan, a and b will be allocated to 32-bit container, since they are
    // in two slice lists that both are size of 32 bits.
    // For the second slicing plan, a will be allocated to a 32-bit container, since it is in a
    // 32-bit slice list. And b[0-15] shares the same rotational cluster with a[0-15], so b has to
    // be allocate to a 32-bit container. Therefore, this slicing does not effectively change the
    // PHV allocation result and should be pruned during DFS.
    // This function collects supercluster that has the aformentioned pattern. To minimize the
    // impact on PHV allocation, this function is very conservative. If a supercluster only have
    // 32-bit fieldslices that are all in individual slice lists, and all fieldslices are in the
    // same rotational cluster, this supercluster will be checked for the duplicate slicing plans.
    void need_to_check_duplicate();
    // check duplicate is -1: duplication check has not been performed.
    // check duplicate is 0: no need to check duplication.
    // check duplicate is 1: need to check duplication and the first slicing plan(first slicing plan
    // is always slice nothing and output the original supercluster) has not been yielded.
    // check duplicate is 2: need to check duplication and the first slicing plan has been yielded.
    int check_duplicate = -1;
    // We only want to
    const int duplicate_check_supercluster_size = 5;
    // If a supercluster with check_duplicate flag on still has at least one 32-bit slice list,
    // all fieldslices will be allocated to 32-bit containers and this is a duplicate slicing plan.
    bool check_duplicate_slicing_plan(const ordered_set<SuperCluster *>);
};

}  // namespace Slicing
}  // namespace PHV

#endif /* BF_P4C_PHV_SLICING_PHV_SLICING_DFS_ITERATOR_H_ */
