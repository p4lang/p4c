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

#ifndef EXTENSIONS_BF_P4C_COMMON_SCC_TOPOSORT_H_
#define EXTENSIONS_BF_P4C_COMMON_SCC_TOPOSORT_H_

#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// SccTopoSorter is an opinionated non-boost implementation topological sort algorithm
// that supports directed graph with strongly connected components.
class SccTopoSorter {
 public:
    using Graph = std::unordered_map<int, std::unordered_set<int>>;

 private:
    int n_nodes_i = 0;
    Graph graph_i;

 public:
    /// create a new node and return a new node_id, starting from 1.
    int new_node();

    /// add a -> b to the graph, i.e. a is a dependency of b,
    /// duplicated edges will be ignored.
    void add_dep(int a, int b);

    /// run SCC and topological sort on the internal graph, returns
    /// a mapping from node_id to toposort index.
    /// For example, Assume a <- b represents a depends on b, and we have
    //
    /// a <- b <- c <-> e
    ///      ^
    ///      |
    ///      d
    //
    /// will be converted to,
    /// a <- b <- {c, e}
    ///      ^
    ///      |
    ///      d
    //
    /// and the output will be,
    /// d, c, e: 1
    /// b: 2
    /// a: 3
    std::unordered_map<int, int> scc_topo_sort() const;

 private:
    void validate_node_id(int i) const;
};

#endif  /* EXTENSIONS_BF_P4C_COMMON_SCC_TOPOSORT_H_ */
