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

#ifndef BF_P4C_PHV_DUMP_TABLE_FLOW_GRAPH_H_
#define BF_P4C_PHV_DUMP_TABLE_FLOW_GRAPH_H_

#include "bf-p4c/mau/table_flow_graph.h"

struct DumpTableFlowGraph : public Visitor {
    const PhvInfo &phv;

    explicit DumpTableFlowGraph(const PhvInfo &p) : phv(p) {}

    struct CollectPhvReadsAndWrites : public Inspector {
        const PhvInfo &phv;
        explicit CollectPhvReadsAndWrites(const PhvInfo &p) : phv(p) {}

        ordered_set<const PHV::Field *> reads, writes;

        // An MAU::Table object may have TableSeq on the $default path,
        // make sure these tables are not visited.
        bool preorder(const IR::MAU::TableSeq *) override { return false; }
        bool preorder(const IR::MAU::Table *tbl) override {
            return !tbl->is_a_gateway_table_only();
        }

        bool preorder(const IR::MAU::Instruction *inst) override {
            for (unsigned i = 0; i < inst->operands.size(); i++) {
                auto f = phv.field(inst->operands[i]);
                if (f) {
                    if (i)
                        reads.insert(f);
                    else
                        writes.insert(f);
                }
            }

            return false;
        }
    };

    struct PhvDetails : public FlowGraph::DumpTableDetails {
        const PhvInfo &phv;
        explicit PhvDetails(const PhvInfo &p) : phv(p) {}

        void dump(std::ostream &out, const IR::MAU::Table *tbl) const override {
            out << FlowGraph::viz_node_name(tbl) << " [ shape=record, style=\"filled\","
                << " fillcolor=cornsilk, label=\""
                << (tbl->is_a_gateway_table_only() ? tbl->gateway_cond : tbl->name) << "\\l\\l"
                << std::endl;

            if (!tbl->match_key.empty()) {
                out << "M:\\l" << std::endl;
                for (auto m : tbl->match_key) {
                    auto f = phv.field(m->expr);
                    // When match on an error type is used the PHV does not exist
                    BUG_CHECK(
                        f,
                        "PHV details for %s not found. Possibly trying to match on an error type?",
                        m->expr);
                    out << " " << stripThreadPrefix(f->name) << "\\l" << std::endl;
                }
                out << "\\l";
            }

            CollectPhvReadsAndWrites fields(phv);
            tbl->apply(fields);

            if (!fields.reads.empty()) {
                out << "R:\\l" << std::endl;
                for (auto r : fields.reads)
                    out << " " << stripThreadPrefix(r->name) << "\\l" << std::endl;
                out << "\\l";
            }

            if (!fields.writes.empty()) {
                out << "W:\\l" << std::endl;
                for (auto w : fields.writes)
                    out << " " << stripThreadPrefix(w->name) << "\\l" << std::endl;
            }

            out << " \"];" << std::endl;
        }
    };

    std::ofstream *open_file(gress_t gress, int pipe_id, cstring directory = "graphs"_cs) {
        auto outdir = BFNContext::get().getOutputDirectory(directory, pipe_id);
        if (!outdir) return nullptr;

        static int fid = 0;
        auto filepath = outdir + "/" + std::to_string(fid++) + "_" + "table_flow_graph" + "_" +
                        ::toString(gress) + ".dot";

        return new std::ofstream(filepath);
    }

    const IR::Node *apply_visitor(const IR::Node *n, const char *) override { return n; }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        auto rv = Visitor::init_apply(root);

        ordered_map<gress_t, FlowGraph> graphs;
        FindFlowGraphs ffg(graphs);
        root->apply(ffg);

        for (auto &kv : graphs) {
            if (kv.second.emptyFlowGraph) continue;

            auto fs = open_file(kv.first, root->to<IR::BFN::Pipe>()->canon_id());
            kv.second.dump_viz(*fs, new PhvDetails(phv));
        }
        return rv;
    }
};

#endif /* BF_P4C_PHV_DUMP_TABLE_FLOW_GRAPH_H_ */
