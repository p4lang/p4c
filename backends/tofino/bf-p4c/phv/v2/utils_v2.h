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

#ifndef BF_P4C_PHV_V2_UTILS_V2_H_
#define BF_P4C_PHV_V2_UTILS_V2_H_

#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/tx_score.h"
#include "bf-p4c/phv/v2/types.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace PHV {
namespace v2 {

/// ErrorCode classifies all kinds of PHV allocation error.
/// NOTE: when adding a new error, remember to update to_str function.
enum class ErrorCode {
    /// For an allocation attempt, if we cannot find bits of container (space) to
    /// satisfy `static` (non-action-related) constraints of candidates,
    /// this error will be returned.
    NOT_ENOUGH_SPACE,
    /// We cannot synthesize an action based on current allocation. Reasons can be
    /// (1) proposed packing in the destination container are invalid.
    /// (2) source operands are not or cannot be allocated accordingly and so we cannot
    ///     find an instruction for the write.
    /// (3) intrinsic conflict of constraints.
    /// For AllocError of this type, it should include a vector of AllocSlices of the destination
    /// container where instruction could not be found. (if many, any one is okay).
    ACTION_CANNOT_BE_SYNTHESIZED,
    /// container type constraint not satisfied, e.g., try non-mocha field against mocha container.
    CONTAINER_TYPE_MISMATCH,
    /// container gress assignment mismatches field gress.
    CONTAINER_GRESS_MISMATCH,
    /// mixed parser write mode for a parser group.
    CONTAINER_PARSER_WRITE_MODE_MISMATCH,
    /// packing extracted with uninitialized is not allowed.
    CONTAINER_PARSER_PACKING_INVALID,
    /// violating max container bytes constraint on field.
    FIELD_MAX_CONTAINER_BYTES_EXCEEDED,
    /// when allocate the aligned cluster, no valid start position was found.
    ALIGNED_CLUSTER_NO_VALID_START,
    /// aligned cluster cannot be allocated to any container group.
    ALIGNED_CLUSTER_CANNOT_BE_ALLOCATED,
    /// super cluster does not any have suitable container group width.
    NO_VALID_CONTAINER_SIZE,
    /// no valid alignment was found for the candidate super cluster
    NO_VALID_SC_ALLOC_ALIGNMENT,
    /// failed to allocate wide arithmetic slice lists
    WIDE_ARITH_ALLOC_FAILED,
    /// no slicing has been found by slicing iterator.
    NO_SLICING_FOUND,
    /// when copacker has identified invalid allocation that sources can never be
    /// packed correctly.
    INVALID_ALLOC_FOUND_BY_COPACKER,
    /// some copack hints are required, for example, when you pack destination fields into
    /// a mocha container, all their sources needs to be packed into a container. If it can't
    /// be applied, then we will throw this error.
    CANNOT_APPLY_REQUIRED_COPACK_HINTS,
};

/// convert @p to string.
cstring to_str(const ErrorCode &e);

/// AllocError contains a code of the failure reason and detailed messages.
/// The additional cannot_allocate_sl may be specified if some allocation process
/// has found that allocation cannot proceed because a slice list cannot be allocated,
/// which is usually caused by field packing in the slice that violates action phv constraints.
struct AllocError {
    ErrorCode code;
    cstring msg;
    /// when allocating slices, and action cannot be synthesized, alloc slices in the
    /// destination container will be saved here.
    const std::vector<AllocSlice> *invalid_packing = nullptr;
    /// when failed to allocate a super cluster because action cannot be synthesized, if it is
    /// intrinsic conflict of constraints, then either the destination slice list needs to be
    /// sliced more, or source slice list needs to be sliced less (packed together). Both
    /// sources and destination slice lists will be saved here.
    const ordered_set<const SuperCluster::SliceList *> *reslice_required = nullptr;
    explicit AllocError(ErrorCode code) : code(code), msg(cstring::empty) {}
    AllocError(ErrorCode code, cstring msg) : code(code), msg(msg) {}
    std::string str() const;
};

template <class T>
AllocError &operator<<(AllocError &e, const T &v) {
    std::stringstream ss;
    ss << v;
    e.msg += ss.str();
    return e;
}

std::ostream &operator<<(std::ostream &out, const AllocError &e);

/// AllocResult is the most common return type of an allocation function. It is either an error
/// or a PHV::Transaction if succeeded.
struct AllocResult {
    const AllocError *err = nullptr;
    std::optional<PHV::Transaction> tx;
    explicit AllocResult(const AllocError *err) : err(err) {}
    explicit AllocResult(const Transaction &tx) : tx(tx) {}
    bool ok() const { return err == nullptr; }
    std::string err_str() const;
    std::string tx_str(cstring prefix = ""_cs) const;
    static std::string pretty_print_tx(const PHV::Transaction &tx, cstring prefix = cstring::empty);
    bool operator==(const AllocResult &other) const;
};

/// ScAllocAlignment is the alignment arrangement for a super cluster based on its alignment
/// constraints of slice lists and aligned clusters.
struct ScAllocAlignment {
    /// a cluster_alignment maps aligned cluster to start bit location in a container.
    ordered_map<const AlignedCluster *, int> cluster_starts;
    /// @returns merged alignment constraint if no conflict was found.
    std::optional<ScAllocAlignment> merge(const ScAllocAlignment &other) const;
    /// @returns pretty print string for the alignment of @p sc.
    cstring pretty_print(cstring prefix, const SuperCluster *sc) const;
    /// @returns true if it is okay to place field slices in @p aligned to @p start index of
    /// a container.
    bool ok(const AlignedCluster *aligned, int start) const {
        return !cluster_starts.count(aligned) || cluster_starts.at(aligned) == start;
    }
    /// @returns true if there is no alignment scheduled: cluster without slice lists.
    bool empty() const { return cluster_starts.empty(); }
};

/// Factory function to build up to @p max_n alignment for @p sc in @p width container group.
/// The process is a DFS on all combinations of different alignments of all slice lists within
/// the cluster @p sc. The DFS order can be tuned by providing a non-null @p sl_alloc_order.
/// NOTE: when @p sl_order is provided, it must have exactly all the slice list in @p sc and
/// no more.
std::vector<ScAllocAlignment> make_sc_alloc_alignment(
    const SuperCluster *sc, const PHV::Size width, const int max_n,
    const std::list<const SuperCluster::SliceList *> *sl_order = nullptr);

/// a collection of allocation configurations that balances speed and performance of allocation.
struct SearchConfig {
    /// The number of DFS steps that allocator can use for each super cluster. This variable
    /// sets the upper-bound of searching time. Ideally, each step can place one slice list to
    /// a container.
    int n_dfs_steps_sc_alloc = 256;

    /// Allocator will find the best score out of this number of allocation for a super cluster.
    int n_best_of_sc_alloc = 1;

    /// Allocator will stop try allocating candiddates to other containers if candidates have been
    /// successfully allcoated to an empty normal container.
    bool stop_first_succ_empty_normal_container = false;

    /// For table match key fields, try byte-aligned container starts first.
    bool try_byte_aligned_starts_first_for_table_keys = false;
};

/// ScoreContext is the allocation context that is updated and passed down during allocation.
class ScoreContext {
    /// current super cluster that we are trying to allocate.
    const SuperCluster *sc_i = nullptr;

    /// current container group that we are trying to allocate.
    const ContainerGroup *cont_group_i = nullptr;

    /// allocation ordered of a super cluster.
    const std::vector<const SuperCluster::SliceList *> *alloc_order_i = {};

    /// decided allocation alignment under current context.
    const ScAllocAlignment *alloc_alignment_i = nullptr;

    /// factory for allocation score.
    const TxScoreMaker *score_i = nullptr;

    /// t is the log level.
    int t_i = 0;

    /// search time related parameters.
    const SearchConfig *search_config_i = new SearchConfig();

    /// tracer will try to log steps of every allocation.
    std::ostream *tracer = nullptr;

 private:
    static constexpr const char *tab_table[] = {
        "", " ", "  ", "   ", "    ", "     ", "      ", "       ", "        ",
    };
    static constexpr int max_log_level = sizeof(tab_table) / sizeof(tab_table[0]) - 1;

 public:
    ScoreContext() {}

    const SuperCluster *sc() const {
        BUG_CHECK(sc_i, "sc not added in ctx.");
        return sc_i;
    }
    ScoreContext with_sc(const SuperCluster *sc) const {
        auto cloned = *this;
        cloned.sc_i = sc;
        return cloned;
    }

    const ContainerGroup *cont_group() const {
        BUG_CHECK(cont_group_i, "container group not added in ctx.");
        return cont_group_i;
    }
    ScoreContext with_cont_group(const ContainerGroup *cont_group) const {
        auto cloned = *this;
        cloned.cont_group_i = cont_group;
        return cloned;
    }

    const std::vector<const SuperCluster::SliceList *> *alloc_order() const {
        BUG_CHECK(alloc_order_i, "sl alloc order not added in ctx.");
        return alloc_order_i;
    }
    ScoreContext with_alloc_order(const std::vector<const SuperCluster::SliceList *> *order) const {
        auto cloned = *this;
        cloned.alloc_order_i = order;
        return cloned;
    }

    const ScAllocAlignment *alloc_alignment() const {
        BUG_CHECK(alloc_alignment_i, "alloc alignment not added in ctx.");
        return alloc_alignment_i;
    }
    ScoreContext with_alloc_alignment(const ScAllocAlignment *alignment) const {
        auto cloned = *this;
        cloned.alloc_alignment_i = alignment;
        return cloned;
    }

    const TxScoreMaker *score() const {
        BUG_CHECK(score_i, "score not added in ctx.");
        return score_i;
    }
    ScoreContext with_score(const TxScoreMaker *score) const {
        auto cloned = *this;
        cloned.score_i = score;
        return cloned;
    }

    int t() const { return t_i; }
    std::string t_tabs() const { return tab_table[t_i]; }
    ScoreContext with_t(int t) const {
        if (t > max_log_level) t = max_log_level;
        auto cloned = *this;
        cloned.t_i = t;
        return cloned;
    }

    const SearchConfig *search_config() const { return search_config_i; }
    ScoreContext with_search_config(const SearchConfig *c) const {
        auto cloned = *this;
        cloned.search_config_i = c;
        return cloned;
    }
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_UTILS_V2_H_ */
