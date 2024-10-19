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

#ifndef BF_P4C_PHV_V2_GREEDY_TX_SCORE_H_
#define BF_P4C_PHV_V2_GREEDY_TX_SCORE_H_

#include "bf-p4c/phv/v2/kind_size_indexed_map.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/v2/tx_score.h"

namespace PHV {
namespace v2 {

/// a pair of container and the index of the byte, counting from 0.
using ContainerByte = std::pair<Container, int>;
/// map tables to ixbar bytes of container.
using TableIxbarContBytesMap = ordered_map<const IR::MAU::Table *, ordered_set<ContainerByte>>;
/// map tables to ixbar bytes of container.
using StageIxbarContBytesMap = ordered_map<int, ordered_set<ContainerByte>>;

/// Vision stores
/// (1) available bits in the PHV v.s. unallocated candidates.
/// (2) TODO: table layout status.
struct Vision {
    /// ContGress represents the hardware gress constraint of a container.
    enum class ContGress {
        ingress,
        egress,
        unassigned,
    };

    /// Map super clusters to required number of containers grouped by kind and size, based
    /// on their baseline allocations.
    ordered_map<const SuperCluster *, KindSizeIndexedMap> sc_cont_required;
    /// The estimated number of containers that will be required for unallocated
    /// super clusters, by gress and size.
    ordered_map<gress_t, KindSizeIndexedMap> cont_required;
    /// Available empty containers. Container without gress assignments will
    /// be counted twice as in ingress and in egress.
    ordered_map<ContGress, KindSizeIndexedMap> cont_available;

    // TODO: maybe a good idea to add learning quanta and pov bits limit here?
    // ordered_map<gress_t, int> pov_bits_used;
    // ordered_map<gress_t, int> pov_bits_unallocated;

    // The direction of dataflow for table matching is
    // phv-bytes ======> ixbar (groups)  =======> search bus ========> SRAM/TCAM table.
    //             (1)                     (2)
    // (1) Constraints are:
    //     + 32-bit container bytes must be mapped to same mod-4 bytes.
    //     + 16-bit container bytes must be mapped to same mod-2 bytes.
    //     + 8-bit container byte can be mapped to any bytes.
    // (2) Constraints are:
    //     + search bus read from one of 8 (exact) or 12 (ternary) ixbar group.
    //     + For ternary, a group is of 5 + 1 (4bit of mid-byte) bytes group.
    //     + For sram, a group has 16 bytes.

    /// maps table to allocated match key positions.
    TableIxbarContBytesMap table_ixbar_cont_bytes;
    /// maps stages to allocated sram match key positions. We do not need to take table mutually
    /// exclusiveness into account because table ixbar cannot be reused, it is a static config.
    StageIxbarContBytesMap stage_sram_ixbar_cont_bytes;
    /// maps stages to allocated tcam match key positions. We do not need to take table mutually
    /// exclusiveness into account because table ixbar cannot be reused, it is a static config.
    StageIxbarContBytesMap stage_tcam_ixbar_cont_bytes;

    /// supply vs demand in terms of bits.
    int bits_demand(const PHV::Kind &k) const;
    int bits_supply(const PHV::Kind &k) const;
    bool has_more_than_enough(const PHV::Kind &k) const {
        return bits_supply(k) >= bits_demand(k);
    };
};

std::ostream &operator<<(std::ostream &, const ContainerByte &);
std::ostream &operator<<(std::ostream &, const Vision &);

class GreedyTxScoreMaker : public TxScoreMaker {
 private:
    const PhvKit &kit_i;

    Vision vision_i;
    std::set<const Field *> table_key_with_ranges;

    friend class GreedyTxScore;
    friend std::ostream &operator<<(std::ostream &, const Vision &);

 public:
    GreedyTxScoreMaker(const PhvKit &utils, const std::list<ContainerGroup *> &container_groups,
                       const std::list<SuperCluster *> &sorted_clusters,
                       const ordered_map<const SuperCluster *, KindSizeIndexedMap> &baseline);

    TxScore *make(const Transaction &tx) const override;

    /// update @a vision_i.
    void record_commit(const Transaction &tx, const SuperCluster *presliced_sc);

    /// When deallocating @p sc from @p curr_alloc, by removing @p slices, some containers
    /// will become free again. We will update vision_i based on them and return them as
    /// a new baseline for this super cluster.
    KindSizeIndexedMap record_deallocation(const SuperCluster *sc,
                                           const ConcreteAllocation &curr_alloc,
                                           const ordered_set<AllocSlice> &slices);

    cstring status() const;

    // algorithms
 public:
    /// @returns an score of how imbalanced container bytes are. Imbalance is measured under
    /// a mod-4 system. For example,
    ///      ixbar bytes layout
    ///   0       1     2       3
    ///  H2@0   MH5@1  W6@2    W6@3
    ///  H9@0          MH5@1
    /// The score is 2 because there are 2 ixbar bytes that has more than minimal bytes (1
    /// in this case). See gtests.
    static int ixbar_imbalanced_alignment(const ordered_set<ContainerByte> &cont_bytes);
};

/// GreedyTxScore is the default allocation heuristics.
class GreedyTxScore : public TxScore {
 private:
    const GreedyTxScoreMaker *maker_i = nullptr;
    const Vision *vision_i;

    /// the most precious container bits: normal container and potentially mocha bits
    /// depending on current context.
    int used_L1_bits() const;

    /// mocha and t-phv bits.
    int used_L2_bits() const;

    friend class GreedyTxScoreMaker;

 private:
    /// The number of `newly used` (not used in parent but used in this tx) container bits,
    /// for each kind of container. This should also account for bits that cannot be
    /// used in future allocation, e.g., unused bits packed with solitary fields,
    /// or unused bits of mocha/dark container, should be considered as `used`, because we cannot
    /// pack other fields in those containers. This value unifies many metrics:
    /// (1) overlay_bits: less used bits.
    /// (2) wasted_bits: higher used bits.
    /// (3) clot_bits: less used bits.
    KindSizeIndexedMap used_bits;

    /// The number of containers that were not used in parent, but used in this tx.
    KindSizeIndexedMap used_containers;

    /// The number of containers that their gress was newly assigned in this tx.
    KindSizeIndexedMap n_set_gress;
    KindSizeIndexedMap n_set_parser_gress;
    KindSizeIndexedMap n_set_deparser_gress;
    KindSizeIndexedMap n_mismatched_deparser_gress;
    // TODO
    // {n_mismatched_deparser_gress, weighted_delta[n_mismatched_deparser_gress], false},

    /// For table key fields, how many new bytes will be used, by stages.
    /// This can potentially optimize cross-table key field packing.
    ordered_map<int, int> ixbar_bytes;

    /// the number of overflow created.
    int n_size_overflow = 0;

    /// It is not rare that two allocations have the same number of container size overflow.
    /// Consider this example:
    ///      Supply v.s. demand
    /// H:    30           35
    /// B:    31           95
    /// Assume the width the super cluster is 3. Either allocated to B, or H, the
    /// number of overflow is the same. But overflow problem on B-sized containers are
    /// more severe. This value is used to characterize this property.
    int n_max_overflowed = 0;

    /// For tofino1, as long as there are redundant TPHV bits, always prefer to allocate to tphv
    /// than to overlay on normal containers.
    int n_tphv_on_phv_bits = 0;

    /// the number of new tphv collection used.
    int n_inc_used_tphv_collection = 0;

    /// the delta of the number of bits that will be read by deparser when pov bits are allocated
    /// in a container. It can be 0 when packing with existing pov bits.
    int n_pov_deparser_read_bits = 0;

    /// the delta of the number of bytes that will be read by deparser
    /// when learning fields are allocated to containers.
    int n_deparser_read_learning_bytes = 0;

    /// The sum of the number of new bytes used by all tables in this tx.
    /// (1) unnecessary cross-byte allocation will use more bytes.
    /// (2) packing match key fields of the same table into the same byte will save bytes.
    int n_table_new_ixbar_bytes = 0;

    /// The sum of the number of new bytes used by all stages int this tx.
    /// (1) unnecessary cross-byte allocation will use more bytes.
    /// (2) packing match key fields of the same stage into the same byte will save bytes.
    int n_stage_new_ixbar_bytes = 0;

    // sum of the imbalanced alignment for all tables involved in this tx.
    int n_table_ixbar_imbalanced_alignments = 0;

    // sum of the imbalanced alignment of ixbar bytes,
    // for all stages that are (1) involved in this tx and (2) the remaining bytes of the stage
    // are less than threshold.
    int n_overloaded_stage_ixbar_imbalanced_alignments = 0;

    // For TCAM table match keys, amount of nibbles occupied, i.e. the width of the span
    // between first and last nibble occupied.
    int n_range_match_nibbles_occupied = 0;

 public:
    explicit GreedyTxScore(const Vision *vision) : vision_i(vision) {}

    /// @returns true if better
    bool better_than(const TxScore *other) const override;

    /// @returns string representation.
    std::string str() const override;

    /// @returns true if deparser gress mismatch present
    bool has_mismatch_gress() const;
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_GREEDY_TX_SCORE_H_ */
