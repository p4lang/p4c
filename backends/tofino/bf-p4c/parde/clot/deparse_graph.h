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
