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

#include "bf-p4c/mau/determine_power_usage.h"

#include <algorithm>
#include <map>
#include <vector>

#include "bf-p4c/mau/mau_power.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/phv/phv.h"

namespace MauPower {

/**
 * Finalize data structures prior to power estimation.
 */
void DeterminePowerUsage::end_apply(const IR::Node *) {
    find_stage_dependencies();
    update_stage_dependencies_for_min_latency();
}

/** Use the dependency graph to determine
 * per-gress MAU stage dependencies based on where tables are located.
 */
void DeterminePowerUsage::find_stage_dependencies() {
    if (LOGGING(4)) {
        dep_graph_.print_dep_type_map(std::cout);
        dep_graph_.print_container_access(std::cout);
    }
    // Initialize stage_dep_to_previous_: stage encoding is ingress, egress, ghost.
    for (gress_t gress : Device::allGresses()) {
        for (int stage = 0; stage < Device::numStages(); ++stage) {
            if (force_match_dependency_) {
                mau_features_->set_dependency_for_gress_stage(gress, stage, DEP_MATCH);
                continue;
            }
            if (stage == 0) {
                // stage 0 considered match dependent
                mau_features_->set_dependency_for_gress_stage(gress, stage, DEP_MATCH);
            } else if (Device::currentDevice() == Device::TOFINO &&
                       stage == Device::numStages() / 2) {
                // Forced match dependency between stages 5 and 6 for Tofino.
                mau_features_->set_dependency_for_gress_stage(gress, stage, DEP_MATCH);
            } else {
                // Start all stages off as concurrent.
                // For Tofino2+, this will be converted to action prior to output.
                mau_features_->set_dependency_for_gress_stage(gress, stage, DEP_CONCURRENT);
            }
        }
    }

    if (force_match_dependency_)
        LOG4("User indicated that all MAU stage dependencies must be match.");

    // Use dependency graph to find final table placement stage dependencies.
    for (gress_t gress : Device::allGresses()) {
        for (int stage = 1; stage < Device::numStages(); ++stage) {
            mau_dep_t worst_dep = mau_features_->get_dependency_for_gress_stage(gress, stage);

            // Check dependencies from tables in current stage to tables in a previous stage.
            // (Until we see a match dependency.)
            int prev_stage = stage - 1;
            while (worst_dep != DEP_MATCH) {
                for (auto *t1 : mau_features_->stage_to_tables_[gress][stage]) {
                    if (worst_dep == DEP_MATCH) break;
                    if (t1->gress != gress) continue;
                    if (table_uses_mocha_container_.find(t1->unique_id()) !=
                        table_uses_mocha_container_.end()) {
                        if (table_uses_mocha_container_.at(t1->unique_id())) {
                            worst_dep = DEP_MATCH;
                            LOG4("  Table " << t1->externalName() << " (stage " << stage
                                            << ") uses mocha containers.");
                            LOG4("    So dependency to previous stage will be set to match.");
                            break;
                        }
                    }
                    // Force a match dependency on stage with separate_gateway annotation because we
                    // want to lower down the power usage even if predication would resolve it
                    // properly.
                    if (t1->getAnnotation("separate_gateway"_cs)) {
                        worst_dep = DEP_MATCH;
                        LOG4("  Table " << t1->externalName() << " (stage " << stage
                                        << ") uses separate_gateway annotation");
                        LOG4("    So dependency to previous stage will be set to match.");
                        break;
                    }
                    auto control_graph = graphs_->get_graph(t1->gress);
                    for (auto *t2 : mau_features_->stage_to_tables_[gress][prev_stage]) {
                        if (t1->gress == t2->gress &&
                            (control_graph->active_simultaneously(t2->unique_id(),
                                                                  t1->unique_id()) ||
                             t1->always_run == IR::MAU::AlwaysRun::TABLE ||
                             t2->always_run == IR::MAU::AlwaysRun::TABLE)) {
                            // Returns the dependency type from t1 to t2.
                            DependencyGraph::mau_dependencies_t mau_dep =
                                dep_graph_.find_mau_dependency(t1, t2);
                            LOG4("MAU DEP t1 = " << t1->externalName() << " and t2 = "
                                                 << t2->externalName() << " is " << mau_dep);
                            if (mau_dep == DependencyGraph::MAU_DEP_MATCH) {
                                worst_dep = DEP_MATCH;
                                LOG4("   match dep: " << t1->externalName() << " (stage " << stage
                                                      << ") to " << t2->externalName() << " (stage "
                                                      << prev_stage << ")");
                                break;  // kill loop, this is worst dependency
                            } else if (mau_dep == DependencyGraph::MAU_DEP_ACTION) {
                                // Keep looking for match dependency.
                                worst_dep = DEP_ACTION;
                                LOG4("   action dep: " << t1->externalName() << " (stage " << stage
                                                       << ") to " << t2->externalName()
                                                       << " (stage " << prev_stage << ")");
                            }
                        }
                    }
                }
                if (prev_stage == 0) break;
                if (mau_features_->get_dependency_for_gress_stage(gress, prev_stage) == DEP_MATCH)
                    break;  // can stop looking if found match dependent stage
                --prev_stage;
            }  // end while
            mau_features_->set_dependency_for_gress_stage(gress, stage, worst_dep);
            LOG4("Setting " << gress << " stage " << stage << " dependency to " << worst_dep);
        }
    }
}

void DeterminePowerUsage::postorder(const IR::BFN::Pipe *) {
    // Propagate shared memory accesses to tables that they were not attached to.
    add_unattached_memory_accesses();
}

bool DeterminePowerUsage::preorder(const IR::MAU::Meter *m) {
    // All 'normal' meters have to run at EOP time.
    bool runs_at_eop = m->color_output();
    bool is_lpf_or_wred = m->alu_output();
    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for Meter %1%", m);
    auto uniq_id = tbl->unique_id(m);
    mau_features_->meter_runs_at_eop_.emplace(uniq_id, runs_at_eop);
    mau_features_->meter_is_lpf_or_wred_.emplace(uniq_id, is_lpf_or_wred);
    return true;
}

bool DeterminePowerUsage::preorder(const IR::MAU::Counter *c) {
    bool runs_at_eop =
        c->type == IR::MAU::DataAggregation::BYTES || c->type == IR::MAU::DataAggregation::BOTH;
    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for Counter %1%", c);
    auto uniq_id = tbl->unique_id(c);
    mau_features_->counter_runs_at_eop_.emplace(uniq_id, runs_at_eop);
    return true;
}

bool DeterminePowerUsage::preorder(const IR::MAU::Selector *sel) {
    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for Selector %1%", sel);
    auto uniq_id = tbl->unique_id(sel);
    mau_features_->selector_group_size_.emplace(uniq_id, sel->max_pool_size);
    return true;
}

bool DeterminePowerUsage::uses_mocha_containers_in_ixbar(const IR::MAU::Table *t) const {
    if (Device::currentDevice() != Device::TOFINO) {
        if (t && t->resources) {
            PHV::FieldUse READ(PHV::FieldUse::READ);
            std::vector<IXBar::Use *> ixbar_uses{
                t->resources->match_ixbar.get(),      t->resources->gateway_ixbar.get(),
                t->resources->proxy_hash_ixbar.get(), t->resources->selector_ixbar.get(),
                t->resources->salu_ixbar.get(),       t->resources->meter_ixbar.get()};
            // Direct crossbar uses.
            for (auto *use : ixbar_uses) {
                if (!use) continue;
                for (auto byte : use->use) {
                    for (auto fi : byte.field_bytes) {
                        auto field = phv_.field(fi.field);
                        if (!field) continue;
                        le_bitrange range = StartLen(fi.lo, fi.hi - fi.lo + 1);
                        bool saw_mocha = false;
                        field->foreach_alloc(range, t, &READ, [&](const PHV::AllocSlice &slice) {
                            if (slice.field_slice().overlaps(fi.lo, fi.hi)) {
                                if (slice.container().is(PHV::Kind::mocha)) saw_mocha = true;
                            }
                        });
                        if (saw_mocha) return true;
                    }
                }
            }
            // Crossbar uses via hash distribution.
            for (auto &hash_dist : t->resources->hash_dists) {
                for (auto &hash_dist_use : hash_dist.ir_allocations) {
                    for (auto &byte : hash_dist_use.use->use) {
                        for (auto &fi : byte.field_bytes) {
                            auto field = phv_.field(fi.field);
                            if (!field) continue;
                            le_bitrange range = StartLen(fi.lo, fi.hi - fi.lo + 1);
                            bool saw_mocha = false;
                            field->foreach_alloc(
                                range, t, &READ, [&](const PHV::AllocSlice &slice) {
                                    if (slice.field_slice().overlaps(fi.lo, fi.hi)) {
                                        if (slice.container().is(PHV::Kind::mocha))
                                            saw_mocha = true;
                                    }
                                });
                            if (saw_mocha) return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void DeterminePowerUsage::postorder(const IR::MAU::Table *t) {
    if (!t->logical_id) return;
    table_uses_mocha_container_.emplace(t->unique_id(), uses_mocha_containers_in_ixbar(t));
    if (t->stage() >= Device::numStages()) {
        exceeds_stages_ = true;
        return;
    }
    mau_features_->stage_to_tables_[t->gress][t->stage()].push_back(t);
    mau_features_->table_to_stage_.emplace(t->unique_id(), t->stage());
    mau_features_->uid_to_table_.emplace(t->unique_id(), t);
    auto my_unique_id = t->unique_id();

    // Sum up memory accesses of each type of resource
    // Note that this is the sum of all attached tables of each specific type.
    // For example, if two meters are attached to a match table, the
    // meter_table power memory access will be the sum of both.
    PowerMemoryAccess match_table, action_data_table, stat_table;
    PowerMemoryAccess meter_table, stateful_table, selector_table;
    PowerMemoryAccess tind_table, idletime_table;

    // collect usage of match table and any attached tables
    // update some map that stores that info

    bool match_runs_at_eop = false;
    bool local_meter_runs_at_eop = false;
    bool have_meter_lpf_or_wred = false;

    for (auto &use : t->resources->memuse) {
        auto &mem = use.second;
        if (mem.type == Memories::Use::METER) {
            if (mau_features_->meter_runs_at_eop_.find(use.first) !=
                mau_features_->meter_runs_at_eop_.end()) {
                match_runs_at_eop |= mau_features_->meter_runs_at_eop_.at(use.first);
                local_meter_runs_at_eop |= mau_features_->meter_runs_at_eop_.at(use.first);
            }
            if (mau_features_->meter_is_lpf_or_wred_.find(use.first) !=
                mau_features_->meter_is_lpf_or_wred_.end()) {
                have_meter_lpf_or_wred = true;
            }
        } else if (mem.type == Memories::Use::COUNTER) {
            if (mau_features_->counter_runs_at_eop_.find(use.first) !=
                mau_features_->counter_runs_at_eop_.end()) {
                match_runs_at_eop |= mau_features_->counter_runs_at_eop_.at(use.first);
            }
        }
    }

    for (auto &use : t->resources->memuse) {
        auto &mem = use.second;
        int all_mems = 0;
        for (auto &r : mem.row) {
            all_mems += r.col.size();
        }

        if (mem.type == Memories::Use::TERNARY) {
            // All TCAMs are read for a lookup
            match_table.tcam_read += all_mems;
            mau_features_->has_tcam_[t->gress][t->stage()] = true;

        } else if (mem.type == Memories::Use::EXACT) {
            // One unit of depth in each way is accessed on a table read.
            // Total RAMs accessed will then be the sum of the widths of ways.
            int num_ways = 0;
            for (auto way : t->ways) {
                // LOG4("  exact match width = " << way.width);
                ++num_ways;
                match_table.ram_read += way.width;
            }
            mau_features_->has_exact_[t->gress][t->stage()] = true;
            // FIXME(mea): If this is a keyless table, compiler currently always
            // uses an exact match bus.  This would have to be changed
            // when that does.
        } else if (mem.type == Memories::Use::ATCAM) {
            // One unit of depth in each column is accessed on a table read.
            // Total RAMs accessed will then be the sum of the widths of columns.
            int col_ram_width = 0;
            int num_cols = 0;
            for (auto way : t->ways) {
                LOG4("  atcam match width = " << way.width);
                if (col_ram_width != 0) {
                    BUG_CHECK(way.width != col_ram_width, "ATCAM column widths do not match.");
                }
                col_ram_width = way.width;
            }

            for (auto mem_way : mem.ways) {
                ++num_cols;
            }

            match_table.ram_read += (col_ram_width * num_cols);
            mau_features_->has_exact_[t->gress][t->stage()] = true;

        } else if (mem.type == Memories::Use::TIND) {
            tind_table.ram_read += 1;

        } else if (mem.type == Memories::Use::ACTIONDATA) {
            int ram_width = (t->layout.action_data_bytes_in_table + 15) / 16;

            auto local_action_data_table = PowerMemoryAccess();
            local_action_data_table.ram_read += ram_width;
            action_data_table += local_action_data_table;
            attached_memory_usage_.emplace(use.first, local_action_data_table);

        } else if (mem.type == Memories::Use::METER) {
            auto local_meter_table = PowerMemoryAccess();
            local_meter_table.ram_read += 1;  // Meters are only one ram wide
            local_meter_table.ram_write += 1;
            local_meter_table.map_ram_read += all_mems;  // All map rams read for synth-2-port
            local_meter_table.map_ram_write += 1;        // But only one written
            // if have color map rams, add that to map_rams_read / map_rams_write
            // All 'normal' meters run at EOP.
            // LPF/WRED do not run at EOP.
            if (local_meter_runs_at_eop) {
                local_meter_table.deferred_ram_read += 1;
                local_meter_table.deferred_ram_write += 1;

                // Since normal meters (color-based) can only run at EOP on Tofino,
                // this condition can be used to increment the map_ram used for
                // meter color.
                local_meter_table.map_ram_read += 1;
                local_meter_table.map_ram_write += 1;
            }
            meter_table += local_meter_table;
            attached_memory_usage_.emplace(use.first, local_meter_table);
            if (have_meter_lpf_or_wred) {
                mau_features_->has_meter_lpf_or_wred_[t->gress][t->stage()] = true;
            }

        } else if (mem.type == Memories::Use::SELECTOR) {
            auto local_selector_table = PowerMemoryAccess();
            local_selector_table.ram_read += 1;  // Selectors are only one ram wide
            local_selector_table.ram_write += 1;
            local_selector_table.map_ram_read += all_mems;  // All map rams read for synth-2-port
            local_selector_table.map_ram_write += 1;        // But only one written

            selector_table += local_selector_table;
            attached_memory_usage_.emplace(use.first, local_selector_table);
            mau_features_->has_selector_[t->gress][t->stage()] = true;

            int sel_words = 1;
            if (mau_features_->selector_group_size_.find(use.first) !=
                mau_features_->selector_group_size_.end()) {
                int gsize = mau_features_->selector_group_size_.at(use.first) +
                            StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE - 1;
                sel_words = gsize / StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE;
            }
            mau_features_->max_selector_words_[t->gress][t->stage()] =
                std::max(mau_features_->max_selector_words_[t->gress][t->stage()], sel_words);

        } else if (mem.type == Memories::Use::STATEFUL) {
            auto *att = t->get_attached(use.first);
            if (att && att->use != IR::MAU::StatefulUse::NO_USE) {
                auto local_stateful_table = PowerMemoryAccess();
                local_stateful_table.ram_read += 1;  // Stateful are only one ram wide
                local_stateful_table.ram_write += 1;
                local_stateful_table.map_ram_read += all_mems;  // All maprams read for synth-2-port
                local_stateful_table.map_ram_write += 1;        // But only one written

                stateful_table += local_stateful_table;
                attached_memory_usage_.emplace(use.first, local_stateful_table);
            }
            mau_features_->has_stateful_[t->gress][t->stage()] = true;

        } else if (mem.type == Memories::Use::COUNTER) {
            auto local_stat_table = PowerMemoryAccess();
            local_stat_table.ram_read += 1;  // Counters are only one ram wide
            local_stat_table.ram_write += 1;
            local_stat_table.map_ram_read += all_mems;  // All map rams read for synth-2-port
            local_stat_table.map_ram_write += 1;        // But only one written

            // Even if counter is packet-based, if there is a meter
            // attached to the match table that runs at EOP, this counter
            // will be setup to run at EOP as well.
            // I no longer remember why this is the case.
            // Something to do with a hardware constraint?
            if (match_runs_at_eop) {
                local_stat_table.deferred_ram_read += 1;
                local_stat_table.deferred_ram_write += 1;
            }

            stat_table += local_stat_table;
            attached_memory_usage_.emplace(use.first, local_stat_table);
            mau_features_->has_stats_[t->gress][t->stage()] = true;

        } else if (mem.type == Memories::Use::IDLETIME) {
            idletime_table.ram_read += 1;  // only access and update one idletime
            idletime_table.ram_write += 1;
        }

        /*
         * Unattached memories means that this is a shared resource.
         * Tables that appear here do not appear in the current match
         * table's memuse.
         * This means we have to find how many resources are consumed
         * by the shared resource from the match table it has been
         * attached to.  The complication here is that the IR traversal
         * order does not guarantee that the table it is attached to is
         * visited first.
         * So, two options:
         * (1) traverse the IR twice.  On first pass, fill in what can.
         *     On second pass, fill in unattached.
         * (2) add helper pass to find unattached match table.
         * Option (2) was selected.
         */
        if (mem.unattached_tables.size() > 0) {
            match_tables_with_unattached_.push_back(t);
        }
    }

    // Sum all memory accesses.
    PowerMemoryAccess memory_access = match_table + action_data_table;
    memory_access += stat_table;
    memory_access += meter_table;
    memory_access += stateful_table;
    memory_access += selector_table;
    memory_access += tind_table;
    memory_access += idletime_table;

    double table_power = memory_access.compute_table_power(Device::numPipes());
    double normalized_wt = memory_access.compute_table_weight(table_power, Device::numPipes());

    LOG4("\nTable = " << t->externalName());
    LOG4("  power = " << table_power << " Watts.");
    LOG4("  normalized weight = " << normalized_wt << ".");
    LOG4("  logical_id = " << *t->global_id() << ".");
    LOG4("\n" << memory_access);
    table_memory_access_.emplace(my_unique_id, memory_access);
}

void DeterminePowerUsage::add_unattached_memory_accesses() {
    // Need to loop over tables again, and find
    for (auto t : match_tables_with_unattached_) {
        // LOG4("Table " << t->name << " has unattached.");
        auto my_unique_id = t->unique_id();
        auto tbl_memory_access = table_memory_access_.at(my_unique_id);
        // LOG4("Memory access before\n" << tbl_memory_access);

        for (auto &use : t->resources->memuse) {
            auto &mem = use.second;
            // For each unattached table, lookup the memory use
            // that is pinned to the attached table name.
            for (auto &unattached : mem.unattached_tables) {
                // auto &attached_table_unique_id = unattached.first;
                for (auto &match_attached_to_unique_id : unattached.second) {
                    // LOG4("Unattached attached_table_unique_id = " << attached_table_unique_id);
                    // LOG4("     match_attached_to_unique_id = " << match_attached_to_unique_id);
                    if (attached_memory_usage_.count(match_attached_to_unique_id)) {
                        auto &attached_mem_access =
                            attached_memory_usage_.at(match_attached_to_unique_id);
                        tbl_memory_access += attached_mem_access;

                        // LOG4("Found attached memory access\n" << attached_mem_access);
                    } else {
                        LOG1("WARNING: Unable to find shared resource memory usage for " << t->name
                                                                                         << ".");
                    }
                }
            }
        }
    }
}

void DeterminePowerUsage::update_stage_dependencies_for_min_latency() {
    auto &spec = Device::mauPowerSpec();
    if (!BackendOptions().disable_egress_latency_padding) {
        // Minimum latency required for egress pipeline on Tofino,
        // so convert stages to match-dependent if can.
        int egress_stage = 0;
        while (mau_features_->compute_pipe_latency(EGRESS) <
                   spec.get_min_required_egress_mau_latency() &&
               egress_stage < Device::numStages()) {
            mau_dep_t dep = mau_features_->get_dependency_for_gress_stage(EGRESS, egress_stage);
            if (dep != DEP_MATCH) {
                mau_features_->set_dependency_for_gress_stage(EGRESS, egress_stage, DEP_MATCH);
                LOG4("Converted egress stage " << egress_stage << " to match dependent.");
            }
            ++egress_stage;
        }
    }
    LOG4("Final pipeline latencies:");
    LOG4("  ingress: " << mau_features_->compute_pipe_latency(INGRESS));
    LOG4("  egress: " << mau_features_->compute_pipe_latency(EGRESS));
    if (Device::currentDevice() != Device::TOFINO)
        LOG4("  ghost: " << mau_features_->compute_pipe_latency(GHOST));
}

}  // end namespace MauPower
