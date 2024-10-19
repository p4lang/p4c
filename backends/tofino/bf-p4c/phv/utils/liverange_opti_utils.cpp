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

#include "bf-p4c/phv/utils/liverange_opti_utils.h"

#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

/// @returns true if any of the dominator units in @doms is a parser node.
bool hasParserUse(const PHV::UnitSet &doms) {
    for (const auto *u : doms)
        if (u->is<IR::BFN::Parser>() || u->is<IR::BFN::ParserState>() ||
            u->is<IR::BFN::GhostParser>())
            return true;
    return false;
}

/// @returns all pairs (x, y) where x is an unit in @f_units that can reach the unit y in @g_units.
ordered_set<std::pair<const IR::BFN::Unit *, const IR::BFN::Unit *>> canFUnitsReachGUnits(
    const PHV::UnitSet &f_units, const PHV::UnitSet &g_units,
    const ordered_map<gress_t, FlowGraph> &flowGraph) {
    ordered_set<std::pair<const IR::BFN::Unit *, const IR::BFN::Unit *>> rv;
    std::optional<gress_t> gress = std::nullopt;
    for (const auto *u1 : f_units) {
        bool deparser1 = u1->is<IR::BFN::Deparser>();
        bool table1 = u1->is<IR::MAU::Table>();
        if (!gress) gress = u1->thread();
        if (hasParserUse({u1})) {
            LOG4("\t\t\tParser defuse " << DBPrint::Brief << u1 << " can reach all g units.");
            for (const auto *u2 : g_units) rv.insert(std::make_pair(u1, u2));
            continue;
        }
        const auto *t1 = table1 ? u1->to<IR::MAU::Table>() : nullptr;
        BUG_CHECK(flowGraph.count(*gress), "Flow graph not found for %1%", *gress);
        const FlowGraph &fg = flowGraph.at(*gress);
        for (const auto *u2 : g_units) {
            // Units of different gresses cannot reach each other.
            if (gress)
                if (u2->thread() != *gress) return rv;
            bool deparser2 = u2->is<IR::BFN::Deparser>();
            // If f was used in a deparser and g was not in the deparser, then f cannot reach g.
            if (deparser1) {
                if (deparser2) {
                    LOG4("\t\t\tBoth units are deparser. Can reach.");
                    rv.insert(std::make_pair(u1, u2));
                } else {
                    LOG4("\t\t\t" << DBPrint::Brief << u1 << " cannot reach " << DBPrint::Brief
                                  << u2);
                }
                continue;
            }
            // Deparser/table use for f_unit cannot reach parser use for g_unit.
            if (table1 && hasParserUse({u2})) {
                LOG4("\t\t\t" << DBPrint::Brief << u1 << " cannot reach " << DBPrint::Brief << u2);
                continue;
            }
            // Deparser use for g can be reached by every unit's use in f.
            if (deparser2) {
                rv.insert(std::make_pair(u1, u2));
                LOG4("\t\t\t" << DBPrint::Brief << u1 << " can reach " << DBPrint::Brief << u2);
                continue;
            }

            if (!u2->is<IR::MAU::Table>())
                BUG("Non-parser, non-deparser, non-table defuse unit found.");
            const auto *t2 = u2->to<IR::MAU::Table>();
            // t2 cannot be nullptr due it being a bug if it is
            // Check if t1 exists
            if (t1 != nullptr) {
                if (fg.can_reach(t1, t2)) {
                    LOG4("\t\t\t" << t1->name << " can reach " << t2->name);
                    rv.insert(std::make_pair(u1, u2));
                } else {
                    LOG4("\t\t\t" << t1->name << " cannot reach " << t2->name);
                }
                // t1 does not exist (u1 is not a table)
            } else {
                LOG4("\t\t\t" << DBPrint::Brief << u1 << "is not a table and cannot reach "
                              << t2->name);
            }
        }
    }
    return rv;
}

/// Trim the set of dominators by removing nodes that are dominated by other dominator nodes
/// already in the set.
void getTrimmedDominators(PHV::UnitSet &candidates, const BuildDominatorTree &domTree) {
    // By definition of dominators, all candidates are tables.
    std::unordered_set<const IR::BFN::Unit *> dominatedNodes;
    for (const auto *u1 : candidates) {
        if (hasParserUse({u1})) continue;
        bool table1 = u1->is<IR::MAU::Table>();
        const auto *t1 = table1 ? u1->to<IR::MAU::Table>() : nullptr;
        for (const auto *u2 : candidates) {
            if (u1 == u2) continue;
            LOG5("\t\t\tDoes " << DBPrint::Brief << u1 << " dominate " << DBPrint::Brief << u2
                               << " ?");
            if (hasParserUse({u2})) continue;
            bool table2 = u2->is<IR::MAU::Table>();
            const auto *t2 = table2 ? u2->to<IR::MAU::Table>() : nullptr;
            // If u1 dominates u2, only consider u1. So, mark u2 for deletion.
            if (domTree.strictlyDominates(t1, t2)) {
                dominatedNodes.insert(u2);
                LOG5("\t\t\t   YES!");
            } else {
                LOG5("\t\t\t   NO!");
            }
        }
    }
    for (const auto *u : dominatedNodes) {
        candidates.erase(u);
    }
}

/// Update flowgraph with ARA edges
ordered_map<gress_t, FlowGraph> update_flowgraph(const PHV::UnitSet &g_units,
                                                 const PHV::UnitSet &f_units,
                                                 const ordered_map<gress_t, FlowGraph> &flgraphs,
                                                 const PHV::Transaction &transact,
                                                 bool &canUseAra) {
    LOG5("   update_flowgraph() with canUseAra: " << canUseAra);

    ordered_map<gress_t, FlowGraph> new_flowgraphs(flgraphs);
    // Set of table pairs that will be connected through new control flow edges
    ordered_set<std::pair<const IR::BFN::Unit *, const IR::BFN::Unit *>> new_edges;
    gress_t grs = INGRESS;

    // Collect unit pairs for new control flow edges
    for (auto *g_u : g_units) {
        if (!g_u->is<IR::MAU::Table>()) continue;

        for (auto *f_u : f_units) {
            if (!f_u->is<IR::MAU::Table>()) continue;
            // If fields used in the same unit init should not be done with ARA
            if (f_u == g_u) {
                canUseAra = false;
                LOG5("\t  turn-off canUseAra");
                break;
            }

            BUG_CHECK(g_u->thread() == f_u->thread(),
                      "Attempting to overlay fields of different gress");

            grs = f_u->thread();
            new_edges.insert(std::make_pair(g_u, f_u));
        }

        if (!canUseAra) {
            new_edges.clear();
            break;
        }
    }

    // Update Transaction's ARA control-flow edges
    for (const auto &u_pair : new_edges) {
        const IR::MAU::Table *g_t = u_pair.first->to<IR::MAU::Table>();
        const IR::MAU::Table *f_t = u_pair.second->to<IR::MAU::Table>();

        // Adding new ARA flow edges
        // *TODO*  Identify dominated tables to skip addition of control flow edges
        transact.addARAedge(grs, g_t, f_t);
        LOG5("\tAdding ARA edge to transaction ( " << grs << "): " << g_t->name << " --> "
                                                   << f_t->name);
    }

    // Update FlowGraph with ARA edges in transaction
    for (const auto &map_entry : transact.getARAedges()) {
        FlowGraph &fg = new_flowgraphs[map_entry.first];

        for (const auto &src2dsts : map_entry.second) {
            auto *src_tbl = src2dsts.first;

            for (auto *dst_tbl : src2dsts.second) {
                auto edge_descriptor = fg.add_edge(src_tbl, dst_tbl, "always_run"_cs);
                LOG5("   Added edge " << edge_descriptor.first << " : " << edge_descriptor.second
                                      << "(gress: " << map_entry.first << ")");
            }
        }
    }

    return new_flowgraphs;
}
