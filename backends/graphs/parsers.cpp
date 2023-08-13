/*
 * Copyright (c) 2017 VMware Inc. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parsers.h"

#include <iosfwd>
#include <string>
#include <utility>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/subgraph.hpp>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/null.h"

namespace graphs {

using Graph = ParserGraphs::Graph;

static cstring toString(const IR::Expression *expression) {
    std::stringstream ss;
    P4::ToP4 toP4(&ss, false);
    toP4.setListTerm("(", ")");
    expression->apply(toP4);
    return cstring(ss.str());
}

// we always have only one subgraph
Graph *ParserGraphs::CreateSubGraph(Graph &currentSubgraph, const cstring &name) {
    auto &newSubgraph = currentSubgraph.create_subgraph();
    boost::get_property(newSubgraph, boost::graph_name) = "cluster" + name;
    boost::get_property(newSubgraph, boost::graph_graph_attribute)["label"] = name;
    boost::get_property(newSubgraph, boost::graph_graph_attribute)["fontsize"] = "22pt";
    boost::get_property(newSubgraph, boost::graph_graph_attribute)["style"] = "bold";
    return &newSubgraph;
}

ParserGraphs::ParserGraphs(P4::ReferenceMap *refMap, const cstring &graphsDir)
    : refMap(refMap), graphsDir(graphsDir) {
    visitDagOnce = false;
}

void ParserGraphs::postorder(const IR::P4Parser *parser) {
    Graph *g_ = new Graph();
    g = CreateSubGraph(*g_, parser->name);
    boost::get_property(*g_, boost::graph_name) = parser->name;

    std::map<const char *, unsigned int> nodes;
    unsigned int iter = 0;

    for (auto state : states[parser]) {
        cstring label = state->name;
        if (state->selectExpression != nullptr &&
            state->selectExpression->is<IR::SelectExpression>()) {
            label += "\n" + toString(state->selectExpression->to<IR::SelectExpression>()->select);
        }
        add_vertex(label, VertexType::STATE);
        nodes.emplace(std::make_pair(state->name.name.c_str(), iter++));
    }

    for (auto edge : transitions[parser]) {
        auto from = nodes[edge->sourceState->name.name.c_str()];
        auto to = nodes[edge->destState->name.name.c_str()];
        add_edge((vertex_t)from, (vertex_t)to, edge->label);
    }

    parserGraphsArray.push_back(g_);
}

void ParserGraphs::postorder(const IR::ParserState *state) {
    auto parser = findContext<IR::P4Parser>();
    CHECK_NULL(parser);
    states[parser].push_back(state);
}

void ParserGraphs::postorder(const IR::PathExpression *expression) {
    auto state = findContext<IR::ParserState>();
    if (state != nullptr) {
        auto parser = findContext<IR::P4Parser>();
        CHECK_NULL(parser);
        auto decl = refMap->getDeclaration(expression->path);
        if (decl != nullptr && decl->is<IR::ParserState>()) {
            auto sc = findContext<IR::SelectCase>();
            cstring label;
            if (sc == nullptr) {
                label = "always";
            } else {
                label = toString(sc->keyset);
            }
            transitions[parser].push_back(
                new TransitionEdge(state, decl->to<IR::ParserState>(), label));
        }
    }
}

void ParserGraphs::postorder(const IR::SelectExpression *expression) {
    // transition (..) { ... } may imply a transition to
    // "reject" - if none of the cases matches.
    for (auto c : expression->selectCases) {
        if (c->keyset->is<IR::DefaultExpression>())
            // If we have a default case this will always match
            return;
    }
    auto state = findContext<IR::ParserState>();
    auto parser = findContext<IR::P4Parser>();
    CHECK_NULL(state);
    CHECK_NULL(parser);
    auto reject = parser->getDeclByName(IR::ParserState::reject);
    CHECK_NULL(reject);
    transitions[parser].push_back(
        new TransitionEdge(state, reject->to<IR::ParserState>(), "fallthrough"));
}

}  // namespace graphs
