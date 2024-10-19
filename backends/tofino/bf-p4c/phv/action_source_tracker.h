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

#ifndef BF_P4C_PHV_ACTION_SOURCE_TRACKER_H_
#define BF_P4C_PHV_ACTION_SOURCE_TRACKER_H_

#include <optional>

#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

namespace PHV {

/// SourceOp represents a source operand.
struct SourceOp {
    enum class OpType { move, bitwise, whole_container };
    OpType t = OpType::move;
    bool ad_or_const = false;
    std::optional<PHV::FieldSlice> phv_src = std::nullopt;

    /// @returns a slice of SourceOp that when ad_or_const is true
    /// a clone will be returned. Otherwise, return a a slice of
    /// phv_src of (start, len);
    SourceOp slice(int start, int len) const;
};

std::ostream &operator<<(std::ostream &out, const SourceOp &src);

/// A collection of source operands of a field slice, classified by actions.
using ActionClassifiedSources = ordered_map<const IR::MAU::Action *, safe_vector<SourceOp>>;

/// ActionSourceTracker collects all source-to-destination for all field slices.
class ActionSourceTracker : public Inspector {
    const PhvInfo &phv;
    const ReductionOrInfo &red_info;

    /// sources[f][range1][act] = {src_op1, src_op2...} means that
    /// f[range1] is written in act, sourcing from src_op1, src_op2....
    /// \note it is not efficient for query to save intersecting ranges of one field, like
    /// sources[f] = {
    ///  0..5: xxx
    ///  2..7: yyy
    ///  3..4: zzz
    /// }, because query will have to iterate, slice and merge all sources.
    /// We will split them into disjoint ranges in the end_apply().
    ordered_map<const PHV::Field *, ordered_map<le_bitrange, ActionClassifiedSources>> sources;

 private:
    void clear() { sources.clear(); };

    /// add source operands in @p act into sources.
    void add_sources(const IR::MAU::Action *act,
                     const ActionAnalysis::FieldActionsMap &instructions);

    /// Clears any state accumulated in prior invocations of this pass.
    profile_t init_apply(const IR::Node *root) override {
        clear();
        return Inspector::init_apply(root);
    }

    /// preorder on actions to collect all source information.
    bool preorder(const IR::MAU::Action *act) override;

    /// fine-slice @a sources and print log sources for all fields.
    void end_apply() override;

 public:
    explicit ActionSourceTracker(const PhvInfo &phv, const ReductionOrInfo &ri)
        : phv(phv), red_info(ri) {}

    /// @returns action classified sources of @p fs for all actions.
    /// @p fs must be fine-sliced: every bit of @p fs must write by the same set of instructions.
    ActionClassifiedSources get_sources(const PHV::FieldSlice &fs) const;

    friend std::ostream &operator<<(std::ostream &out, const ActionSourceTracker &tracker);
};

std::ostream &operator<<(std::ostream &out, const ActionClassifiedSources &sources);
std::ostream &operator<<(std::ostream &out, const ActionSourceTracker &tracker);

}  // namespace PHV

#endif /* BF_P4C_PHV_ACTION_SOURCE_TRACKER_H_ */
