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

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_PARSER_GRAPH_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_PARSER_GRAPH_H_

#include <boost/graph/adjacency_list.hpp>

#include "backends/graphs/controls.h"
#include "backends/graphs/graph_visitor.h"
#include "backends/graphs/parsers.h"

/**
 * @ingroup parde
 * @brief Extends p4c's parser graph with various algorithms
 */
class P4ParserGraphs : public graphs::ParserGraphs {
    void postorder(const IR::P4Parser *parser) override {
        if (dumpDot) {
            // Create parser dot, that is saved into parserGraphsArray
            graphs::ParserGraphs::postorder(parser);
            // Create empty control graphs
            std::vector<graphs::Graphs::Graph *> emptyControl;
            // And call graph visitor that actually outputs the graphs from the arrays
            std::filesystem::path filePath = "";
            std::filesystem::path empty_path = "";
            graphs::Graph_visitor gvs(empty_path, true, false, false, filePath);
            gvs.process(emptyControl, parserGraphsArray);
            // Clear the array so it won't stay there if another parser is outputted
            parserGraphsArray.clear();
        }
    }

    void end_apply() override {
        for (auto &kv : transitions) {
            for (auto *t : kv.second) {
                auto *src = t->sourceState;
                auto *dst = t->destState;

                preds[dst->name].insert(src->name);
                succs[src->name].insert(dst->name);
            }
        }
    }

    bool is_descendant_impl(cstring a, cstring b, std::set<cstring> &visited) const {
        if (!a || !b || a == b) return false;

        auto inserted = visited.insert(b);
        if (inserted.second == false)  // visited
            return false;

        if (!succs.count(b)) return false;

        if (succs.at(b).count(a)) return true;

        for (auto succ : succs.at(b))
            if (is_descendant_impl(a, succ, visited)) return true;

        return false;
    }

 public:
    P4ParserGraphs(P4::ReferenceMap *refMap, bool dumpDot)
        : graphs::ParserGraphs(refMap, ""), dumpDot(dumpDot) {}

    /// Is "a" a descendant of "b"?
    bool is_descendant(cstring a, cstring b) const {
        std::set<cstring> visited;
        return is_descendant_impl(a, b, visited);
    }

    /// Is "a" an ancestor of "b"?
    bool is_ancestor(cstring a, cstring b) const { return is_descendant(b, a); }

    const IR::ParserState *get_state(const IR::P4Parser *parser, cstring state) const {
        for (auto s : states.at(parser)) {
            if (s->name == state) return s;
        }
        return nullptr;
    }

    /// a kludge due to base class's lack of state -> parser reverse map
    const IR::P4Parser *get_parser(const IR::ParserState *state) const {
        for (auto &kv : states) {
            for (auto s : kv.second)
                if (s->name == state->name) return kv.first;
        }

        return nullptr;
    }

    ordered_set<const IR::ParserState *> get_all_descendants(const IR::ParserState *state) const {
        ordered_set<const IR::ParserState *> rv;

        auto parser = get_parser(state);

        for (auto s : states.at(parser)) {
            if (is_descendant(s->name, state->name)) rv.insert(s);
        }

        return rv;
    }

    ordered_set<const IR::ParserState *> get_all_ancestors(const IR::ParserState *state) const {
        ordered_set<const IR::ParserState *> rv;

        auto parser = get_parser(state);

        for (auto s : states.at(parser)) {
            if (is_descendant(state->name, s->name)) rv.insert(s);
        }

        return rv;
    }

    boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> create_boost_graph(
        const IR::P4Parser *parser, std::map<int, cstring> &id_to_state) const;

    std::set<std::set<cstring>> compute_strongly_connected_components(
        const IR::P4Parser *parser) const;

    std::set<std::set<cstring>> compute_loops(const IR::P4Parser *parser) const;

    bool has_loops(const IR::P4Parser *parser) const {
        auto loops = compute_loops(parser);
        return !loops.empty();
    }

    std::vector<cstring> topological_sort(const IR::P4Parser *parser) const;

    std::map<cstring, ordered_set<cstring>> preds, succs;

    bool dumpDot;
};

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_PARSER_GRAPH_H_ */
