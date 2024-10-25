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

#include "deparse_graph.h"

DeparseGraph::Node DeparseGraph::addField(const PHV::Field *f) {
    if (fieldToVertex.count(f)) return fieldToVertex.at(f);

    fields.insert(f);
    reachability.clear();
    return fieldToVertex[f] = boost::add_vertex(NodeInfo(f), g);
}

DeparseGraph::Node DeparseGraph::addConst(const IR::Constant *c) {
    reachability.clear();
    return boost::add_vertex(NodeInfo(c), g);
}

bool DeparseGraph::addEdge(Node src, Node dst) {
    // Look for a pre-existing edge.
    typename Graph::out_edge_iterator out, end;
    for (boost::tie(out, end) = boost::out_edges(src, g); out != end; ++out) {
        if (boost::target(*out, g) == dst) return false;
    }

    // No pre-existing edge, so make one.
    auto maybeNewEdge = boost::add_edge(src, dst, g);

    // A vector-based adjacency_list (i.e. Graph) is a multigraph.
    // Inserting edges should always create new edges.
    BUG_CHECK(maybeNewEdge.second, "Boost Graph Library failed to add edge.");

    reachability.clear();

    return true;
}

bool DeparseGraph::canReach(const PHV::Field *f1, const PHV::Field *f2) const {
    BUG_CHECK(fieldToVertex.count(f1), "Field %1% not found in deparser graph", f1->name);
    BUG_CHECK(fieldToVertex.count(f2), "Field %1% not found in deparser graph", f2->name);

    auto n1 = fieldToVertex.at(f1);
    auto n2 = fieldToVertex.at(f2);

    return reachability.canReach(n1, n2);
}

const std::vector<DeparseGraph::NodeInfo> DeparseGraph::nodesBetween(const PHV::Field *f1,
                                                                     const PHV::Field *f2) const {
    BUG_CHECK(fieldToVertex.count(f1), "Field %1% not found in deparser graph", f1->name);
    BUG_CHECK(fieldToVertex.count(f2), "Field %1% not found in deparser graph", f2->name);

    auto n1 = fieldToVertex.at(f1);
    auto n2 = fieldToVertex.at(f2);

    std::vector<NodeInfo> result;
    for (auto node : reachability.reachableBetween(n1, n2)) {
        result.push_back(g[node]);
    }

    return result;
}

void DeparseGraph::clear() {
    g.clear();
    reachability.clear();
    fields.clear();
    fieldToVertex.clear();
}
