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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_DEPARSE_GRAPH_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_DEPARSE_GRAPH_H_

#include "bf-p4c/lib/boost_graph.h"
#include "bf-p4c/phv/phv_fields.h"

// Declare a type and boost property for DeparseGraph vertices.
namespace boost {
enum vertex_deparse_graph_t { vertex_deparse_graph };
BOOST_INSTALL_PROPERTY(vertex, deparse_graph);
}  // namespace boost

/// Encodes adjacency information for the deparser. Nodes represent fields or constants that get
/// deparsed. A forward edge from n1 to n2 (and a back edge from n2 to n1) exists if n1 is deparsed
/// right before n2 is deparsed.
class DeparseGraph {
 public:
    struct NodeInfo {
        /// Non-null if this node represents the deparsing of a constant.
        const IR::Constant *constant;

        /// Non-null if this node represents the deparsing of a field.
        const PHV::Field *field;

        bool isConstant() const { return constant; }
        bool isField() const { return field; }

        // This seems to be required by Boost, but breaks the representation invariant that at
        // least one of @constant and @field is non-null. :(
        NodeInfo() : constant(nullptr), field(nullptr) {}

        explicit NodeInfo(const IR::Constant *constant) : constant(constant), field(nullptr) {
            BUG_CHECK(constant, "Created a deparse graph node with a null constant");
        }

        explicit NodeInfo(const PHV::Field *field) : constant(nullptr), field(field) {
            BUG_CHECK(field, "Created a deparse graph node with a null field");
        }
    };

 private:
    typedef boost::adjacency_list<boost::vecS, boost::vecS,
                                  boost::bidirectionalS,  // Directed edges.
                                  NodeInfo                // Label vertices with NodeInfo.
                                  >
        Graph;

 public:
    using Node = typename Graph::vertex_descriptor;

    /// @returns the internal node corresponding to the given field, creating the node if one
    /// doesn't already exist.
    Node addField(const PHV::Field *f);

    /// @returns a fresh internal node for representing a deparsed constant.
    Node addConst(const IR::Constant *c);

    /// Adds an edge from src to dst. @returns true if a new edge is created as a result.
    bool addEdge(Node src, Node dst);

    /// Determines whether @p f1 might be deparsed before @p f2.
    bool canReach(const PHV::Field *f1, const PHV::Field *f2) const;

    /// @returns a description of all nodes that might be deparsed between @p f1 and @p f2.
    const std::vector<NodeInfo> nodesBetween(const PHV::Field *f1, const PHV::Field *f2) const;

    void clear();

 private:
    /// The underlying Boost graph backing this DeparseGraph.
    Graph g;

    /// All fields in this graph.
    ordered_set<const PHV::Field *> fields;

    /// Maps fields in this graph to internal Boost graph vertices.
    std::map<const PHV::Field *, Node> fieldToVertex;

    mutable Reachability<Graph> reachability;

 public:
    DeparseGraph() : reachability(g) {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_DEPARSE_GRAPH_H_ */
