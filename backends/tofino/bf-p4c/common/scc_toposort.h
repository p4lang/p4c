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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_SCC_TOPOSORT_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_SCC_TOPOSORT_H_

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

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_SCC_TOPOSORT_H_ */
