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

#ifndef BF_P4C_PHV_OPTIMIZE_PHV_H_
#define BF_P4C_PHV_OPTIMIZE_PHV_H_

#include <optional>

#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"

struct TransactData {
    // Mapping table of Transaction -> SuperCluster
    std::unordered_map<PHV::Transaction *, PHV::SuperCluster *> tr_to_sc_i;
    // Mapping table of Transaction -> Unique Transaction ID
    std::unordered_map<PHV::Transaction *, int> tr_to_tr_id_i;
    // Ordered Mapping table of Unique Transaction ID -> Transaction
    std::map<int, PHV::Transaction *> tr_id_to_tr_i;
    // Ordered Mapping table of Container -> Set of Unique Transaction ID
    std::map<PHV::Container, std::set<int>> container_to_tr_ids_i;
    // Next Transaction ID to use. Removed Transaction ID are never re-used
    int next_tr_id = 0;

    // Mapping table of Unique Transaction ID -> Diff for logging purpose
    std::map<int, const cstring> tr_id_to_diff;
};

/**
 * Optimization Strategy called when PHV Allocation fail to place all the SuperClusters
 *
 * The Brute Force Strategy add a Transaction for each SuperClusters being assigned. This
 * Transaction list can be used to replay back the PHV Allocation found by the greedy algorithm.
 * It can also be used as a baseline to try replacing some of these transactions with others and
 * ultimately increase the whole solution score. The solution is optimized until we are either able
 * to fit all of the unassigned SuperClusters or it went over the entire transaction list for
 * MAX_OPT_PASS times.
 *
 * This is an overview of what this optimization strategy is doing:
 *
 * 1 - Take the first Transaction (a) in the list
 * 2 - Create a Transaction list (b) that excludes this Transaction (a) and all of the Transaction
 *     that was dependant (c) on that one as well
 * 3 - Play back this Transaction list (b)
 * 4 - Try to re-assign the removed Transaction (a) and all of the dependant one (c)
 * 5 - If that succeed, Try to assign the unassigned SuperClusters
 * 6 - Take the next Transaction from the list and go back to (1)
 */
class BruteForceOptimizationStrategy {
    // Limit the number of dependant transactions when trying to replace a transaction.
    static constexpr int MAX_PRUNED_TRANSACTIONS = 16;
    // Limit the number of passes we spend on trying to optimize the solution. One pass is equal
    // to the number of transaction eligible to be replaced.
    static constexpr int MAX_OPT_PASS = 2;
    // Number of passes that try to optimize the solution without any dark spilling. This is to
    // help Table Allocation fitting.
    static constexpr int MAX_OPT_PASS_WO_DARK = 1;
    // Only apply this optimization process if the solution have at least 32 Transactions. This is
    // to avoid trying to optimize the pounding rounds except if that one is huge and have
    // potential to be swapped differently.
    static constexpr int MIN_TR_SIZE = 32;
    // Largest SuperCluster to optimize. If at least one SuperCluster is larger than that, cancel
    // the optimization process since we don't have much chance to find a solution.
    static constexpr size_t MAX_SC_BIT = 64;
    // Maximum number of bits to place through the optimization process.
    static constexpr size_t MAX_TOTAL_SC_BIT = 256;

    // Required to call the BruteForceAllocationStrategy allocation services.
    BruteForceAllocationStrategy &alloc_strategy_i;
    const std::list<PHV::ContainerGroup *> &container_groups_i;
    const ScoreContext &score_ctx_i;

    // Transaction Data that is continuously updated to increase the solution score.
    TransactData *data;

    // Build a sequence of Transaction In/Out if tr_id is removed.
    bool buildSeqWithout(int tr_id, std::list<int> &tr_in, std::list<int> &tr_out);

    // Play a sequence on top of a particular Transaction.
    BruteForceOptimizationStrategy playSeq(std::list<int> &tr_in, PHV::Transaction &partial_alloc);

 public:
    explicit BruteForceOptimizationStrategy(
        BruteForceAllocationStrategy &alloc_strategy_i,
        const std::list<PHV::ContainerGroup *> &container_groups, const ScoreContext &score_ctx)
        : alloc_strategy_i(alloc_strategy_i),
          container_groups_i(container_groups),
          score_ctx_i(score_ctx) {
        data = new TransactData();
    }

    // Insert a Transaction/SuperCluster to the optimization strategy.
    void addTransaction(PHV::Transaction &transaction, PHV::SuperCluster &sc, int tr_id = -1);

    // Start the optimization process.
    std::list<PHV::SuperCluster *> optimize(std::list<PHV::SuperCluster *> &unallocated_sc,
                                            PHV::Transaction &rst);

    // Print the Container to Transaction Dependencies.
    void printContDependency();
};

#endif /* BF_P4C_PHV_OPTIMIZE_PHV_H_ */
