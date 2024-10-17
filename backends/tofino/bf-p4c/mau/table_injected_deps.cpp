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

#include <algorithm>
#include <sstream>
#include "ir/ir.h"
#include "lib/log.h"
#include "lib/ltbitmatrix.h"

#include "table_injected_deps.h"
#include "bf-p4c/common/field_defuse.h"

bool InjectControlDependencies::preorder(const IR::MAU::TableSeq *seq) {
    const Context *ctxt = getContext();
    if (ctxt && dynamic_cast<const IR::MAU::Table *>(ctxt->node)) {
        const IR::MAU::Table* parent;
        parent = dynamic_cast<const IR::MAU::Table *>(ctxt->node);
        auto seq_size = seq->size();
        auto seq_front = seq->front();
        auto seq_back = seq->back();

        LOG5("  ICD Parent:" << parent->name);

        for (auto child : seq->tables) {
            LOG5("    ICD Child:" << child->name);
            if (child->is_always_run_action()) {
                LOG5("\t\tSkiping ARA table " << child->name);
                continue;
            }

            if ((parent->is_detached_attached_tbl) || (child->is_detached_attached_tbl)) {
                LOG5("\t\tSkiping detached attached tables " << parent->name << child->name);
                continue;
            }

            auto edge_label = DependencyGraph::NONE;
            auto ctrl_annot = ""_cs;
            // Find control type relationship between parent & child
            for (auto options : parent->next) {
                LOG5("      ICD seq:" << options.first);
                if (options.second->size()  != seq_size  ||
                    options.second->front() != seq_front ||
                    options.second->back()  != seq_back)
                    continue;

                for (auto dst : options.second->tables) {
                    LOG5("        ICD dst:" << dst->name);
                    if (dst == child) {
                        ctrl_annot = options.first;
                        if (edge_label != DependencyGraph::NONE)
                            LOG3("\t\tICD Multiple deps to DST " << child->name << " from SRC " <<
                                 parent->name << ". Prev dep = " << edge_label << " Curr dep = " <<
                                 ctrl_annot);
                        edge_label = DependencyGraph::get_control_edge_type(ctrl_annot);
                        break;
                    }
                }
            }
            BUG_CHECK(edge_label != DependencyGraph::NONE,
                    "Cannot resolve Injected Control Edge Type");

            auto edge_pair = dg.add_edge(parent, child, edge_label);
            if (!edge_pair.first) continue;
            LOG3("Injecting CONTROL edge between " << parent->name << " --> " << child->name);

            dg.ctrl_annotations[*edge_pair.first] = ctrl_annot;  // Save annotation
        }
    }

    return true;
}

//     apply {
//         switch (t1.apply().action_run) {
//             default : {
//                 if (hdr.data.f2 == hdr.data.f1) {
//                     t2.apply();
//                     switch (t3.apply().action_run) {
//                         noop : {
//                             t4.apply();
//                         }
//                     }
//                 }
//             }
//             a1 : {
//                 t4.apply();
//             }
//         }
//     }

// In this example, InjectControlDependencies walks up the IR starting at t4
// and inserts a control dependence within t4's parent table sequence [t2, t3]
// between t2 and t4's parent, t3. The walk up the IR stop's at t1, t4's dominator.


/**
 * In Tofino, extra ordering is occasionally required due to the requirements of each table
 * having a single next table.  Examine the following program:
 *
 *     apply {
 *         switch (t1.apply().action_run) {
 *             a1 : {
 *                 t2.apply();
 *                 switch (t3.apply().action_run) {
 *                     noop : { t4.apply(); }
 *                 }
 *             }
 *             default : { t4.apply(); }
 *         }
 *     }
 *     t5.apply();
 *
 * In this example, for Tofino only, the table t4 must have the same default next table out of
 * every single execution.  This means on all branches under t1, t4 must be the last table that is
 * applied.  Now examine the sequence under branch a1, where both t2 and t3 can be applied.
 * If t2 and t3 have no dependencies, (and their children have no dependencies), they can be
 * reordered.  However, next table dictates that when a table is placed, all of it's control
 * dependent tables must be placed as well.  Means that if t3 is to be placed before t2, t4 would
 * also be placed.  This, however, would break the constraint that t4 has a single next table.
 *
 * Thus, in order to map this constraint to table placement, an ordering dependency is placed
 * between t2 and t3 to correctly propagate next table data.
 *
 * This pass now works on the new IR rules for Tables and TableSeqs.  Tests verifying this pass
 * are contained in the table_dependency gtest under PredicationEdges Tests
 */
void PredicationBasedControlEdges::postorder(const IR::MAU::Table *tbl) {
    name_to_table[tbl->externalName()] = tbl;
    auto dom = ctrl_paths.find_dominator(tbl);
    if (dom == tbl)
        return;
    auto paths = ctrl_paths.table_pathways.at(tbl);

    std::set<const IR::MAU::Table *> ignore_tables;
    bool first_seq = true;
    /**
     * In the previous example, the table t4, due to next table propagation, required table
     * t2 and t3 to have an order.  But some examples don't require an order.  Examine the
     * following example:
     *
     *     switch (t1.apply().action_run) {
     *         a1 : { t2.apply(); t3.apply(); t4.apply(); }
     *         a2 : { t3.apply(); t4.apply(); }
     *     }
     *     t5.apply();
     *
     * Now if there were no dependencies between these tables, it would seem that table t4
     * would have to appear last for next table propagation to table t5.  However, t3 always
     * precedes t4 in every single application, and thus could in theory be reordered.
     *
     * Thus in the analysis for t4, the ignore tables will determine that t3 always precedes it
     * in all applications, and thus is safe not to add a dependency.
     */
    for (auto path : paths) {
        auto seq = path[1]->to<IR::MAU::TableSeq>();
        std::set<const IR::MAU::Table *> local_set;
        for (auto seq_table : seq->tables) {
            if (seq_table == tbl) break;
            local_set.insert(seq_table);
        }
        if (first_seq) {
            ignore_tables.insert(local_set.begin(), local_set.end());
            first_seq = false;
        } else {
            std::set<const IR::MAU::Table *> intersection;
            std::set_intersection(ignore_tables.begin(), ignore_tables.end(),
                                  local_set.begin(), local_set.end(),
                                  std::inserter(intersection, intersection.begin()));
            ignore_tables = intersection;
        }
    }

    // Walk up the table pathways for any table that precedes this table, and add a dependency
    // between these tables and the current table, as long as its not in the ignore_tables set
    for (auto path : paths) {
        auto it = path.begin();
        const IR::MAU::Table *local_check = (*it)->to<IR::MAU::Table>();
        BUG_CHECK(local_check == tbl, "Table Pathways not correct");
        while (true) {
            it++;
            auto higher_seq = (*it)->to<IR::MAU::TableSeq>();
            BUG_CHECK(it != path.end() && higher_seq != nullptr, "Table Pathways not correct");
            for (auto seq_table : higher_seq->tables) {
                if (local_check == seq_table) break;
                if (ignore_tables.count(seq_table)) continue;
                edges_to_add[seq_table].insert(local_check);
            }

            it++;
            auto higher_tbl = (*it)->to<IR::MAU::Table>();
            BUG_CHECK(it != path.end() && higher_tbl != nullptr, "Table Pathways not correct");
            if (higher_tbl == dom) break;
            local_check = higher_tbl;
        }
    }
}

void PredicationBasedControlEdges::end_apply() {
    if (dg == nullptr) return;
    for (auto &kv : edges_to_add) {
        for (auto dst : kv.second) {
            dg->add_edge(kv.first, dst, DependencyGraph::ANTI_NEXT_TABLE_CONTROL);
        }
    }
}

/**
 * The purpose of this function is to add the extra control dependencies for metadata
 * initialization for Tofino.  Metadata initialization works in the following way:
 *
 * Metadata is assumed to always be initialized to 0.  Two pieces of metadata that are live in
 * the same packet can be overlaid if and only if the metadata have mutually exclusive live
 * ranges.  Before any read of a metadata, the metadata must be set to 0, which generally
 * comes from the parser.  In the overlay cases, however, the metadata itself in the later live
 * range must be written to 0 explicitly.
 *
 * This analysis to guarantee these possibilities is all handled by PHV allocation.  However,
 * the allocation implicitly adds dependencies that cannot be directly tracked by the table
 * dependency graph.  The rule is that all tables that use the metadata in the first live range
 * must appear before any tables using metadata in the second live range.
 * This by definition is a control dependence between two tables.
 *
 * In Tofino, specifically, putting a control dependence in the current analysis won't necessarily
 * work.  The table placement algorithm only works by placing all tables that require next table
 * propagation through them.  Data dependencies between any next table propagation of these
 * two tables are tracked through the TableSeqDeps (a separate analysis than PHV).  However,
 * control dependencies, like this metadata overlay, are not possible to track.
 *
 * The only way to truly track these, while still being able to use the correct analysis of
 * the dependency analysis is to mark two tables in the same TableSeq as Control Dependent.
 * Specifically the two tables marked as CONTROL Dependent are the two tables in the same
 * TableSeq object that require the two actual control dependent tables to follow due to next
 * table propagation limits in Tofino.
 *
 * This constraint is much looser than JBay, and perhaps can be replaced by getting rid of
 * TableSeqDeps, and just using an analysis on the DependencyGraph to pull this information,
 * rather than calculating it directly.
 */
void InjectMetadataControlDependencies::end_apply() {
    if (tables_placed)
        return;

    for (auto kv_pair : phv.getMetadataDeps()) {
        cstring first_table = kv_pair.first;
        BUG_CHECK(name_to_table.count(first_table), "Table %s has a metadata dependency, but "
                  "doesn't appear in the TableGraph?", first_table);
        for (cstring second_table : kv_pair.second) {
            BUG_CHECK(name_to_table.count(second_table), "Table %s has a metadata dependency, but "
                      "doesn't appear in the TableGraph?", second_table);
            auto inject_points = ctrl_paths.get_inject_points(name_to_table.at(first_table),
                                                             name_to_table.at(second_table));
            BUG_CHECK(fg.can_reach(name_to_table.at(first_table), name_to_table.at(second_table)),
                "Metadata initialization analysis incorrect.  Live ranges between %s and %s "
                "overlap", first_table, second_table);
            auto edge_pair = dg.add_edge(name_to_table.at(first_table),
                    name_to_table.at(second_table), DependencyGraph::ANTI_NEXT_TABLE_METADATA);
            if (!edge_pair.first) continue;
            auto tpair = std::make_pair(first_table, second_table);
            auto mdDepFields = phv.getMetadataDepFields();
            if (mdDepFields.count(tpair)) {
                auto mdDepField = mdDepFields.at(tpair);
                dg.data_annotations_metadata.emplace(*edge_pair.first, mdDepField);
            }
            LOG5("  Injecting ANTI dep between " << first_table << " and " << second_table
                 << " due to metadata initializaation");
            for (auto inject_point : inject_points) {
                auto inj1 = inject_point.first->to<IR::MAU::Table>();
                auto inj2 = inject_point.second->to<IR::MAU::Table>();
                LOG3("  Metadata inject points " << inj1->name << " " << inj2->name
                     << " from tables " << first_table << " " << second_table);
                BUG_CHECK(fg.can_reach(inj1, inj2), "Metadata initialization analysis incorrect.  "
                     "Cannot inject dependency between %s and %s", inj1, inj2);
                // Instead of adding injection points at the control point, just going to
                // rely on the metadata check in table placement, as this could eventually be
                // replaced, along with TableSeqDeps, with a function call
                // dg.add_edge(inject_point.first, inject_point.second, DependencyGraph::CONTROL);
            }
        }
    }
}

/**
 * Adds anti-dependency edges for tables with exiting actions.
 *
 * In Tofino2, `exit()` is desugared into an assignment to an `hasExited` variable, which gets
 * threaded through the program. This results in a read-write dependence between any table with an
 * exiting action and all subsequent tables in the control flow. Here, we mimic this dependence for
 * Tofino's benefit, except we use anti-dependency edges instead of flow dependency edges, and we
 * add edges in a way that maintains the correct chain-length metric for tables.
 *
 * Edges are added as follows. For each table T with an exiting action, start at T and walk up to
 * the top level of the pipe along all possible control-flow paths. For the table T' at each level
 * (including T itself), let S be the table sequence containing T', and add the following
 * anti-dependency edges:
 *   - to T' from the next-table leaves of every predecessor of T' in S.
 *   - from each next-table leaf of T' to every successor of T' in S.
 */
void InjectActionExitAntiDependencies::postorder(const IR::MAU::Table* table) {
    if (tables_placed) return;
    if (!table->has_exit_action()) return;

    std::set<const IR::MAU::Table*> processed;

    // Walk up to the top level of the pipe along each control-flow path to the table.
    for (auto path : ctrl_paths.table_pathways.at(table)) {
        const IR::MAU::Table* curTable = nullptr;
        for (auto parent : path) {
            if (auto t = parent->to<IR::MAU::Table>()) {
                curTable = t;
                continue;
            }

            auto tableSeq = parent->to<IR::MAU::TableSeq>();
            if (!tableSeq) continue;

            if (processed.count(curTable)) continue;

            CHECK_NULL(curTable);
            bool predecessor = true;  // Tracks whether "sibling" is a predecessor of "curTable".
            for (auto sibling : tableSeq->tables) {
                if (sibling == curTable) {
                    predecessor = false;
                    continue;
                }

                if (predecessor) {
                    // Add an anti-dependency to curTable from each next-table leaf of the
                    // predecessor.
                    for (auto leaf : cntp.next_table_leaves.at(sibling)) {
                        auto edge_pair = dg.add_edge(leaf, curTable, DependencyGraph::ANTI_EXIT);
                        if (!edge_pair.first) continue;
                        dg.data_annotations_exit.emplace(*edge_pair.first,
                                                            table->get_exit_actions());
                    }
                } else {
                    // Add an anti-dependency edge from each next-table leaf of curTable to the
                    // successor.
                    for (auto leaf : cntp.next_table_leaves.at(curTable)) {
                        auto edge_pair = dg.add_edge(leaf, sibling, DependencyGraph::ANTI_EXIT);
                        if (!edge_pair.first) continue;
                        dg.data_annotations_exit.emplace(*edge_pair.first,
                                                            table->get_exit_actions());
                    }
                }
            }
        }
    }
}

bool InjectControlExitDependencies::preorder(const IR::MAU::Table* table) {
    if (tables_placed) return false;
    tables_placed |= table->is_placed();
    if (!table->run_before_exit) return false;
    collect_run_before_exit_table(table);
    return true;
}

void InjectControlExitDependencies::postorder(const IR::MAU::Table* table) {
    if (tables_placed) return;
    if (!is_first_run_before_exit_table_in_gress(table)) return;
    inject_dependencies_from_gress_root_tables_to_first_rbe_table(table);
}

void InjectControlExitDependencies::collect_run_before_exit_table(
        const IR::MAU::Table* rbe_table) {
    auto it = run_before_exit_tables.find(rbe_table->gress);
    LOG3("  Adding "
         << rbe_table->gress
         << " "
         << rbe_table->name
         << " to run_before_exit_tables map");
    if (it != run_before_exit_tables.end())
        it->second.push_back(rbe_table);
    else
        run_before_exit_tables.emplace(rbe_table->gress,
                                       std::vector<const IR::MAU::Table *>{ rbe_table });
}

bool InjectControlExitDependencies::is_first_run_before_exit_table_in_gress(
        const IR::MAU::Table* rbe_table) {
    auto first = run_before_exit_tables[rbe_table->gress].begin();
    if (run_before_exit_tables[rbe_table->gress].empty())
        return false;
    else
        return rbe_table == *first;
}

void InjectControlExitDependencies::inject_dependencies_from_gress_root_tables_to_first_rbe_table(
        const IR::MAU::Table* first_rbe_table) {
    LOG3("  Injecting CONTROL_EXIT dependencies from all "
         << first_rbe_table->gress
         << " root tables to first run before exit table "
         << first_rbe_table->name);
    auto root_table_seq = get_gress_root_table_seq(first_rbe_table);
    for (auto source : root_table_seq->tables) {
        if (!source->run_before_exit)
            inject_control_exit_dependency(source, first_rbe_table);
    }
}

const IR::MAU::TableSeq* InjectControlExitDependencies::get_gress_root_table_seq(
        const IR::MAU::Table* table) {
    auto paths = ctrl_paths.table_pathways.at(table);
    auto root_table_seq = paths[0][1]->to<IR::MAU::TableSeq>();
    return root_table_seq;
}

void InjectControlExitDependencies::link_run_before_exit_tables() {
    for (auto kv : run_before_exit_tables) {
        LOG3("  Linking " << kv.first << " run before exit tables");
        auto first = kv.second.begin();
        auto last = kv.second.empty() ? kv.second.end() : std::prev(kv.second.end());
        for (auto it = first; it != last; ++it)
            inject_control_exit_dependency(*it, *std::next(it));
    }
}

void InjectControlExitDependencies::inject_control_exit_dependency(
        const IR::MAU::Table* source,
        const IR::MAU::Table* destination) {
    auto annotation = "exit"_cs;
    auto edge_pair = dg.add_edge(source, destination, DependencyGraph::CONTROL_EXIT);
    if (!edge_pair.first) return;
    LOG4("    Injecting CONTROL_EXIT dependency: "
         << source->name
         << " --> "
         << destination->name);
    dg.ctrl_annotations[*edge_pair.first] = annotation;
}

const IR::MAU::Table* InjectDarkAntiDependencies::getTable(UniqueId uid) {
    if (id_to_table.count(uid))
        return id_to_table.at(uid);

    // Check if we can find same-name UniqueId
    std::vector<UniqueId> same_name_tbls;

    for (auto entry : id_to_table) {
        if (entry.first.name == uid.name)
            same_name_tbls.push_back(entry.first);
    }

    BUG_CHECK(same_name_tbls.size() > 0, "Cannot find UniqueId %1% in id_to_table", uid);

    if (same_name_tbls.size() > 1) {
        LOG1("UniqueId " << uid << " shares name with " << same_name_tbls.size() << " tables!!!");
        for (auto tbl_id : same_name_tbls)
            LOG1("\t\t " << tbl_id);
    }

    return id_to_table.at(*(same_name_tbls.begin()));
}

bool InjectDarkAntiDependencies::preorder(const IR::MAU::Table *tbl) {
    placed |= tbl->is_placed();
    if (placed)
        return false;
    id_to_table[tbl->pp_unique_id()] = tbl;
    LOG7("\t Table: " << tbl->name << "  pp_id:" << tbl->pp_unique_id());
    return true;
}

void InjectDarkAntiDependencies::end_apply() {
    if (placed) return;

    LOG7("\t\t ARA Constraints have " << phv.getARAConstraints().size() << " gresses");
    int gress_idx = 1;

    for (auto & per_gress_map : Values(phv.getARAConstraints())) {
        LOG7("\t\t Gress " << gress_idx << " has " << per_gress_map.size() << " ara tables");
        int ara_idx = 0;

        for (auto pair : per_gress_map) {
            auto orig_ar_id = pair.first->pp_unique_id();
            // auto curr_ar_table = id_to_table.at(orig_ar_id);
            auto curr_ar_table = getTable(orig_ar_id);
            LOG7("\t\t ARA table " << ara_idx << " has " << pair.second.first.size() <<
                 " prior tables and " << pair.second.second.size() << " post tables");

            for (auto orig_id : pair.second.first) {
                auto curr_table = getTable(orig_id);

                auto edge_pair = dg.add_edge(curr_table, curr_ar_table,
                            DependencyGraph::ANTI_NEXT_TABLE_METADATA);
                if (!edge_pair.first) continue;
                LOG6("\t\tInjectDarkAntiDependence(1): " << curr_table->name << " --> " <<
                     curr_ar_table->name);
            }

            for (auto orig_id : pair.second.second) {
                auto curr_table = getTable(orig_id);
                auto edge_pair = dg.add_edge(curr_ar_table, curr_table,
                            DependencyGraph::ANTI_NEXT_TABLE_METADATA);
                if (!edge_pair.first) continue;
                LOG6("\t\tInjectDarkAntiDependence(2): " << curr_ar_table->name << " --> " <<
                     curr_table->name);
            }
            ara_idx++;
        }
        gress_idx++;
    }
}

void InjectDepForAltPhvAlloc::end_apply() {
    LOG3("Injecting dependencies for alt-phv-alloc overlays" << IndentCtl::indent);

    // Field Filtering Function
    // Field is an alias
    std::function<bool(const PHV::Field*)>
    fieldIsAlias = [&](const PHV::Field* f) {
        if (!f) return false;
        return (f->aliasSource != nullptr);
    };

    // Slice Filtering Function
    // Slice is read only & deparsed zero
    std::function<bool(const PHV::AllocSlice*)>
    sliceIsReadOnlyOrDeparseZero = [&](const PHV::AllocSlice* sl) {
        if (!sl) return false;
        return ((sl->getEarliestLiveness().second.isRead()
                && sl->getLatestLiveness().second.isRead())
               || sl->field()->is_deparser_zero_candidate());
    };
    auto container_to_slices = phv.getContainerToSlicesMap(&fieldIsAlias,
                                                            &sliceIsReadOnlyOrDeparseZero);

    for (const auto& kv : container_to_slices) {
        const auto& c = kv.first;
        const auto& slices = kv.second;

        // Skip container with single or no slice allocation
        if (slices.size() <= 1) continue;

        for (auto i = slices.begin(); i != (slices.end() - 1); i++) {
            LOG4("Injecting for container: " << c << " --> slice A: " << *i << IndentCtl::indent);
            for (auto j = i + 1; j != slices.end(); j++) {
                const auto& si = *i;
                const auto& sj = *j;
                LOG4("Start slice B: " << sj);

                // Filter slice pairs to those which are overlayed,
                // not mutex and have a disjoint live range

                // Skip fields which are not overlayed
                auto overlayed = si.container_slice().overlaps(sj.container_slice());
                if (!overlayed) {
                    LOG5("Fields are not overlayed");
                    continue;
                }

                // Skip fields mutex
                if (phv.field_mutex()(si.field()->id, sj.field()->id)) {
                    LOG5("Fields are mutually exlusive");
                    continue;
                }

                // Ideally we need a BUG_CHECK here to error out if there are
                // instances of mutex fields which are overlayed but do not have disjoint
                // ranges (i.e. they overlap). However there are cases where this is not true
                // when the tables are mutually exclusive, the fields can be mutex but overlap and
                // not cause an issue.
                //
                // Two non-mutex fields, f_a and f_b, are overlaid in B0.
                // f_a's live range: [-1w, 4r]
                // f_b's live range: [3w, 7r]
                // It's not a BUG, because when the table t_a that writes f_a are mutex
                // with table t_b that reads f_b, hence they will not cause read / write violations
                //
                //             stage 3         stage 4
                //    |---- t_a writes B0
                // ---|
                //    |--------------------- t_b reads B0
                //
                // BUG_CHECK(si.isLiveRangeDisjoint(sj),
                //     "Slices are overlayed but not disjoint or mutually exclusive %1% - %2%",
                //      si, sj);

                // Determine dependency direction (from / to) based on live range
                bool siBeforeSj = si.getEarliestLiveness() < sj.getEarliestLiveness();
                const PHV::AllocSlice& fromSlice = siBeforeSj ? si : sj;
                const PHV::AllocSlice& toSlice   = siBeforeSj ? sj : si;

                LOG6("From Slice : " << fromSlice << " --> To Slice : " << toSlice);
                std::map<int, const IR::MAU::Table*> id_from_tables;
                std::map<int, const IR::MAU::Table*> id_to_tables;
                for (const auto& from_locpair : defuse.getAllDefsAndUses(fromSlice.field())) {
                    const auto from_table = from_locpair.first->to<IR::MAU::Table>();
                    if (!from_table) continue;
                    id_from_tables[from_table->id] = from_table;
                    LOG6("Table Read / Write (From Slice): " << from_table->name);
                }
                for (const auto& to_locpair : defuse.getAllDefsAndUses(toSlice.field())) {
                    const auto to_table = to_locpair.first->to<IR::MAU::Table>();
                    if (!to_table) continue;
                    id_to_tables[to_table->id] = to_table;
                    LOG6("Table Read / Write (To Slice): " << to_table->name);
                }
                for (const auto& from_table : id_from_tables) {
                    for (const auto& to_table : id_to_tables) {
                        if (from_table.second == to_table.second)
                            continue;

                        auto mutex_tables = mutex(from_table.second, to_table.second);
                        if (mutex_tables) {
                            LOG5("Tables " << from_table.second->name << " and "
                                    << to_table.second->name << " are mutually exclusive");
                            continue;
                        }
                        LOG4("Adding edge between " << from_table.second->name
                                                   << " and " << to_table.second->name);
                        dg.add_edge(from_table.second, to_table.second,
                                    DependencyGraph::ANTI_NEXT_TABLE_CONTROL);
                    }
                }
            }
            LOG4_UNINDENT;
        }
    }

    LOG6("Table Dependency Graph (Alt Inject Deps)");
    LOG6(dg);
    LOG3_UNINDENT;
}

TableFindInjectedDependencies
        ::TableFindInjectedDependencies(const PhvInfo &p, DependencyGraph& d,
                                        FlowGraph& f, const BFN_Options *options,
                                        const TableSummary *summary)
        : phv(p), dg(d), fg(f), summary(summary) {
    auto mutex = new TablesMutuallyExclusive();
    auto defuse = new FieldDefUse(phv);
    addPasses({
        // new DominatorAnalysis(dg, dominators),
        // new InjectControlDependencies(dg, dominators),
        // After Table Placement for JBay, it is unsafe to run DefaultNext, which is run for
        // flow graph at the moment.  This is only used for a validation check for metadata
        // dependencies, nothing else.  Has never failed after table placement
        new InjectControlDependencies(dg),
        new FindFlowGraph(fg),
        &ctrl_paths,
        new PassIf(
            [options] {
                return !Device::hasLongBranches() || (options && options->disable_long_branch);
            },
            { new PredicationBasedControlEdges(&dg, ctrl_paths) }),
        new InjectMetadataControlDependencies(phv, dg, fg, ctrl_paths),
        &cntp,
        new InjectActionExitAntiDependencies(dg, cntp, ctrl_paths),
        new PassIf(
            [] {
                return Device::currentDevice() == Device::TOFINO;
            },
            { new InjectControlExitDependencies(dg, ctrl_paths) }),
        new InjectDarkAntiDependencies(phv, dg, ctrl_paths),
        // During Alt finalize table inject dependencies between overlayed fields
        new PassIf(
            [options, summary] {
                return (options && options->alt_phv_alloc && summary
                        && summary->getActualState() == State::ALT_FINALIZE_TABLE);
            },
            {
                mutex,
                defuse,
                new InjectDepForAltPhvAlloc(phv, dg, *defuse, *mutex)
            })
    });
}

