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

#include "bf-p4c/phv/optimize_phv.h"

/**
 * Insert a Transaction/SuperCluster to the optimization strategy.
 *
 * @param[in] transaction Transaction to keep track
 * @param[in] sc SuperCluster binded to this Transaction
 * @param[in] tr_id Optional Transaction ID. When unset, the Transaction ID will be the next
 *                  integer value. Removed Transaction ID are not re-used.
 */
void BruteForceOptimizationStrategy::addTransaction(PHV::Transaction &transaction,
                                                    PHV::SuperCluster &sc, int tr_id) {
    if (tr_id != -1) data->next_tr_id = tr_id;

    data->tr_to_sc_i.insert(std::make_pair(&transaction, &sc));
    data->tr_to_tr_id_i.insert(std::make_pair(&transaction, data->next_tr_id));
    data->tr_id_to_tr_i.insert(std::make_pair(data->next_tr_id, &transaction));
    data->tr_id_to_diff.insert(std::make_pair(data->next_tr_id, transaction.getTransactionDiff()));

    LOG4("Adding Transaction " << data->next_tr_id << ":" << sc);
    LOG4(data->tr_id_to_diff.at(data->next_tr_id));
    const PHV::Transaction *parent =
        dynamic_cast<const PHV::Transaction *>(transaction.getParent());
    BUG_CHECK(parent, "Unable to find the parent of this Transaction");
    for (auto kv : transaction.getTransactionStatus()) {
        bool new_slice = false;
        const auto *parent_status = parent->getStatus(kv.first);
        BUG_CHECK(parent_status,
                  "Trying to get allocation status for container %1% not in Allocation",
                  cstring::to_cstring(kv.first));

        // Sanity check
        for (const PHV::AllocSlice &slice : kv.second.slices) {
            if (!parent_status->slices.count(slice)) {
                new_slice = true;
                break;
            }
        }
        if (!new_slice) {
            for (const PHV::AllocSlice &slice : parent_status->slices) {
                if (!kv.second.slices.count(slice)) {
                    new_slice = true;
                    break;
                }
            }
        }
        bool gress_assign = !(parent_status->gress == kv.second.gress &&
                              parent_status->parserGroupGress == kv.second.parserGroupGress &&
                              parent_status->deparserGroupGress == kv.second.deparserGroupGress);
        if (!new_slice && !gress_assign)
            BUG("Transaction contains parent cache information, new_slice=%1%, gress_assign=%2%",
                new_slice, gress_assign);

        std::set<int> ids;
        auto ret = data->container_to_tr_ids_i.insert(std::make_pair(kv.first, ids));
        ret.first->second.insert(data->next_tr_id);
        LOG4("    Adding Container dependency on " << kv.first);
    }
    data->next_tr_id++;
}

/**
 * Build a sequence of Transaction In/Out if @p tr_id is removed.
 *
 * @param[in] tr_id Transaction ID to remove from the list.
 * @param[out] tr_in Transaction ID list that have no dependency on @p tr_id
 * @param[out] tr_out Transaction ID list that have dependency on @p tr_id
 *
 * @return true if the returned sequence is valid. false otherwise.
 */
bool BruteForceOptimizationStrategy::buildSeqWithout(int tr_id, std::list<int> &tr_in,
                                                     std::list<int> &tr_out) {
    std::set<int> pruned_tr_ids;
    std::list<int> to_prune_tr_ids;
    std::stringstream tr_in_ss;
    std::stringstream tr_out_ss;

    pruned_tr_ids.insert(tr_id);
    to_prune_tr_ids.push_back(tr_id);

    LOG4("Building sequence without Transaction " << tr_id);
    while (!to_prune_tr_ids.empty()) {
        int &tr_to_prune = to_prune_tr_ids.front();
        for (auto kv : data->container_to_tr_ids_i) {
            auto it = kv.second.find(tr_to_prune);
            while (it != kv.second.end()) {
                auto ret = pruned_tr_ids.insert(*it);
                if (ret.second == true) {
                    LOG4("    Adding dependant Transaction " << *it);
                    to_prune_tr_ids.push_back(*it);
                }
                it++;
            }
        }
        to_prune_tr_ids.pop_front();

        // Avoid pruning too many transactions which can lead to performance issue
        if (pruned_tr_ids.size() > BruteForceOptimizationStrategy::MAX_PRUNED_TRANSACTIONS) {
            LOG4("    Too many Transaction to prune " << pruned_tr_ids.size() << "... Abort");
            return false;
        }
    }
    tr_in.clear();
    tr_out.clear();

    for (auto kv : data->tr_id_to_tr_i) {
        auto it = pruned_tr_ids.find(kv.first);
        if (it != pruned_tr_ids.end()) {
            tr_out_ss << kv.first << " ";
            tr_out.push_back(kv.first);
        } else {
            tr_in_ss << kv.first << " ";
            tr_in.push_back(kv.first);
        }
    }
    LOG5("    Transaction In: " << tr_in_ss);
    LOG5("    Transaction Out: " << tr_out_ss);
    return true;
}

/**
 * Play a sequence on top of a particular Transaction.
 *
 * @param[in] tr_in Transaction ID list to play
 * @param[in] partial_alloc Re-play the Transaction on top of @p partial_alloc
 *
 * @return BruteForceOptimizationStrategy object containing the re-played sequence.
 */
BruteForceOptimizationStrategy BruteForceOptimizationStrategy::playSeq(
    std::list<int> &tr_in, PHV::Transaction &partial_alloc) {
    BruteForceOptimizationStrategy rv = BruteForceOptimizationStrategy(
        this->alloc_strategy_i, this->container_groups_i, this->score_ctx_i);

    LOG4("Re-Playing the given Transaction ID list");
    for (int &tr_id : tr_in) {
        // Find the Transaction and the SuperCluster from the ID
        PHV::Transaction *tr = data->tr_id_to_tr_i.at(tr_id);
        PHV::SuperCluster *sc = data->tr_to_sc_i.at(tr);
        // Cloning is needed to update the parent and also to make sure the added transaction is
        // not cleaned by the commit.
        PHV::Transaction *clone = tr->clone(partial_alloc);
        PHV::Transaction *upd_parent = tr->clone(partial_alloc);
        rv.addTransaction(*upd_parent, *sc, tr_id);
        partial_alloc.commit(*clone);
    }

    return rv;
}

/**
 * Start the optimization process.
 *
 * @param[inout] unallocated_sc Unallocated SuperClusters list to place. The list is updated
 *                              during the process and ideally is returned empty on success.
 * @param[inout] rst Base Transaction to commit the best result on.
 *
 * @return Assigned SuperClusters list from the @p unallocated_sc list.
 */
std::list<PHV::SuperCluster *> BruteForceOptimizationStrategy::optimize(
    std::list<PHV::SuperCluster *> &unallocated_sc, PHV::Transaction &rst) {
    std::optional<PHV::Transaction> best_alloc = std::nullopt;
    std::optional<PHV::Transaction> best_partial = std::nullopt;
    std::list<PHV::SuperCluster *> allocated_sc;
    std::stringstream opt_history;
    int max_opt_pass;
    int pass = 0;

    opt_history << "Beginning of the PHV Optimization process with " << unallocated_sc.size()
                << " unallocated super clusters\n";
    LOG4("Beginning of the PHV Optimization process with " << unallocated_sc.size()
                                                           << " unallocated super clusters");
    int num_sc = 1;
    for (auto *usc : unallocated_sc) {
        LOG2("  " << num_sc << ".  " << *usc);
        opt_history << "  " << num_sc << ".  " << *usc;
        ++num_sc;
    }

    // Do not optimize relatively small Transaction pool (typically pounder round)
    if (data->tr_id_to_tr_i.size() < BruteForceOptimizationStrategy::MIN_TR_SIZE) {
        LOG4("Cancelling the PHV Optimization process since transaction size = "
             << data->tr_id_to_tr_i.size());
        return allocated_sc;
    }

    // Validate some minimum criteria to try the optimization process.
    size_t largest_sc_bits = 0;
    size_t total_sc_bits = 0;
    for (PHV::SuperCluster *sc : unallocated_sc) {
        total_sc_bits += sc->aggregate_size();
        if (sc->aggregate_size() > largest_sc_bits) largest_sc_bits = sc->aggregate_size();
    }
    LOG4("Total unallocated super clusters bits=" << total_sc_bits);
    LOG4("Largest unallocated super clusters bits=" << largest_sc_bits);

    if (largest_sc_bits > BruteForceOptimizationStrategy::MAX_SC_BIT ||
        total_sc_bits > BruteForceOptimizationStrategy::MAX_TOTAL_SC_BIT) {
        LOG4("Cancelling the PHV Optimization process since the unallocated superclusters "
             << "represent too many bits");
        return allocated_sc;
    }

    if (Device::currentDevice() != Device::TOFINO)
        max_opt_pass = BruteForceOptimizationStrategy::MAX_OPT_PASS;
    else
        max_opt_pass = BruteForceOptimizationStrategy::MAX_OPT_PASS_WO_DARK;

    // Save the initial value of the dark spilling. Ultimately we might have to change the way we
    // iterate between darkSpillARA Enable -> Disable. Dark Spilling cause Table Allocation fitting
    // issue and it would be nice to be able to enable/disable it with some kind of level such that
    // for example only Dark Spilling that don't affect Table Allocation fitting can be enabled at
    // first.
    bool initial_ara = PhvInfo::darkSpillARA;
    while (pass < max_opt_pass) {
        // One pass is defined using the first and the last Transaction ID at this time.
        int stop_id = data->tr_id_to_tr_i.rbegin()->first;
        int next_id = data->tr_id_to_tr_i.begin()->first;
        LOG4("Pass " << pass << ", First Id: " << next_id << ", Stop Id: " << stop_id);
        opt_history << "Pass " << pass << ", First Id: " << next_id << ", Stop Id: " << stop_id
                    << "\n";

        // This always disable dark spilling for n passes and re-enable it after. This overwrite
        // the actual PhvInfo::darkSpillARA and can introduce dark spilling even on cases where it
        // was explicitely disabled. This is expected but at the end of the day it is better to
        // give it a try than to just fail on PHV Allocation.
        if (pass < BruteForceOptimizationStrategy::MAX_OPT_PASS_WO_DARK)
            PhvInfo::darkSpillARA = false;
        else
            PhvInfo::darkSpillARA = true;

        while (next_id < stop_id) {
            std::list<int> tr_in;
            std::list<int> tr_out;
            if (buildSeqWithout(next_id, tr_in, tr_out)) {
                LOG4("Try a sequence without Transaction id: " << next_id);
                opt_history << "Try a sequence without Transaction id: " << next_id << "\n";
                bool all_tr_replaced = true;
                bool any_tr_updated = false;
                PHV::Transaction partial_alloc = rst.makeTransaction();
                BruteForceOptimizationStrategy partial_opt = playSeq(tr_in, partial_alloc);

                for (int tr_id : tr_out) {
                    PHV::Transaction *tr = data->tr_id_to_tr_i.at(tr_id);
                    PHV::SuperCluster *sc = data->tr_to_sc_i.at(tr);
                    LOG4("Try replacing Transaction id: " << tr_id);
                    opt_history << "Try replacing Transaction id: " << tr_id << "\n";
                    best_alloc = alloc_strategy_i.tryVariousSlicing(
                        partial_alloc, sc, container_groups_i, score_ctx_i, opt_history);
                    if (best_alloc) {
                        LOG4("    Success!");
                        PHV::Transaction *clone = (*best_alloc).clone(partial_alloc);
                        partial_opt.addTransaction(*clone, *sc);
                        partial_alloc.commit(*best_alloc);

                        int best_tr_id = partial_opt.data->next_tr_id - 1;
                        const cstring &original = data->tr_id_to_diff.at(tr_id);
                        const cstring &best = partial_opt.data->tr_id_to_diff.at(best_tr_id);
                        if (original != best) {
                            any_tr_updated = true;
                            opt_history << "Found difference between the original and "
                                        << "the replacement diff:\n";
                            opt_history << "Original:\n";
                            opt_history << original << "\n";
                            opt_history << "Replacement:\n";
                            opt_history << best << "\n";
                        } else {
                            opt_history << "Found NO difference between the original and "
                                        << "the replacement diff\n";
                        }
                    } else {
                        // Only accept a solution that minimally replace all of the removed
                        // Transaction. The idea is to always improve the solution or at least
                        // have an equivalent solution. This can be improved in the future to
                        // accept such solution to help getting out of local minima.
                        LOG4("    Failed... Abort!");
                        all_tr_replaced = false;
                        break;
                    }
                }
                // Compare score before and after?
                if (all_tr_replaced) {
                    if (any_tr_updated) {
                        LOG4("Try placing unallocated super clusters");
                        opt_history << "Try placing unallocated super clusters\n";
                        bool unallocated_update = false;
                        for (PHV::SuperCluster *sc : unallocated_sc) {
                            LOG4("Try allocating super cluster " << *sc);
                            opt_history << "Try allocating super cluster " << *sc;
                            best_alloc = alloc_strategy_i.tryVariousSlicing(
                                partial_alloc, sc, container_groups_i, score_ctx_i, opt_history);
                            if (best_alloc) {
                                // Great! We were able to successfully insert one of the
                                // unallocated SuperCluster.
                                LOG4("    Success!");
                                PHV::Transaction *clone = (*best_alloc).clone(partial_alloc);
                                partial_opt.addTransaction(*clone, *sc);
                                partial_alloc.commit(*best_alloc);
                                allocated_sc.push_back(sc);
                                unallocated_update = true;
                            }
                        }
                        if (unallocated_update) {
                            LOG4("Update unallocated super cluster list");
                            for (PHV::SuperCluster *sc : allocated_sc) {
                                unallocated_sc.remove(sc);
                            }
                        }
                    }
                    // Always update the solution even if the new solution is somehow equivalent
                    // to the previous one.
                    data = partial_opt.data;
                    best_partial = partial_alloc;
                }
            }
            // Stop criteria
            if (unallocated_sc.empty()) break;

            // Move to the next Transaction ID. Take note that replaced Transaction ID are replaced
            // with higher value to replay the sequence in the proper order. This can lead to
            // hole in the Transaction ID list which is why we have to get the next Transaction ID
            // that follow the one being processed.
            next_id = data->tr_id_to_tr_i.upper_bound(next_id)->first;
        }
        // Stop criteria
        if (unallocated_sc.empty()) break;

        int unallocated_slices = 0;
        int unallocated_bits = 0;
        LOG1("Pass " << pass << " finished with " << unallocated_sc.size() << " superclusters:");
        opt_history << "Pass " << pass << " finished with " << unallocated_sc.size()
                    << " superclusters:\n";
        int num = 1;
        for (auto *usc : unallocated_sc) {
            LOG1("  " << num << ".  " << *usc);
            opt_history << "  " << num << ".  " << *usc;
            ++num;
            usc->forall_fieldslices([&](const PHV::FieldSlice &fs) {
                unallocated_slices++;
                unallocated_bits += fs.size();
            });
        }
        LOG1(" Unallocated slices: " << unallocated_slices);
        LOG1(" Unallocated bits:   " << unallocated_bits);
        opt_history << " Unallocated slices: " << unallocated_slices;
        opt_history << " Unallocated bits:   " << unallocated_bits;

        pass++;
    }

    LOG4("Exit Optimization phase with " << unallocated_sc.size()
                                         << " unallocated super clusters remaining");
    opt_history << "Exit Optimization phase with " << unallocated_sc.size()
                << " unallocated super clusters remaining\n";
    if (LOGGING(1)) {
        // Use a separate file for the optimization_history.
        auto filename = Logging::PassManager::getNewLogFileName("phv_optimization_history_"_cs);
        int pipe_id = alloc_strategy_i.getPipeId();
        auto logfile = new Logging::FileLog(pipe_id, filename, Logging::Mode::AUTO);
        LOG1(opt_history.str());
        Logging::FileLog::close(logfile);
    }

    if (best_partial && !allocated_sc.empty()) rst.commit(*best_partial);

    PhvInfo::darkSpillARA = initial_ara;

    return allocated_sc;
}

/**
 * Print the Container to Transaction Dependencies.
 */
void BruteForceOptimizationStrategy::printContDependency() {
    for (auto kv : data->container_to_tr_ids_i) {
        LOG4("Container " << kv.first << " have dependency with transaction ID:");
        std::stringstream tr_id_ss;
        for (int tr_id : kv.second) {
            tr_id_ss << tr_id << " ";
        }
        LOG4(tr_id_ss);
    }
}
