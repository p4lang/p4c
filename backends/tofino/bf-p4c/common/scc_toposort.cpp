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

#include <common/scc_toposort.h>

#include <functional>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "lib/log.h"

namespace {

// return node id to scc id mapping.
std::unordered_map<int, int> tarjan(const int n, const SccTopoSorter::Graph &graph) {
    std::unordered_map<int, int> scc_id;
    std::unordered_map<int, int> dfn;
    std::unordered_map<int, int> low;
    int dfc = 0;
    int n_scc = 0;
    std::stack<int> stack;

    std::function<void(int)> dfs = [&](int curr) -> void {
        low[curr] = dfn[curr] = ++dfc;
        stack.push(curr);
        if (graph.count(curr)) {
            for (const auto &to : graph.at(curr)) {
                if (!dfn.count(to)) {
                    dfs(to);
                    low[curr] = std::min(low[curr], low[to]);
                } else if (!scc_id.count(to)) {
                    low[curr] = std::min(low[curr], dfn[to]);
                }
            }
        }
        if (low[curr] == dfn[curr]) {
            n_scc++;
            int x;
            do {
                x = stack.top();
                stack.pop();
                scc_id[x] = n_scc;
            } while (x != curr);
        }
    };

    for (int i = 1; i <= n; i++) {
        if (!dfn.count(i)) {
            dfs(i);
        }
    }
    return scc_id;
}

// return node id to indexes in the topological sort result. Fields on the same level
// will have same index. For example,
// a <- b <- c
//      ^
//      |--- d
// c, d: 0
// b: 1
// a: 2
std::unordered_map<int, int> toposort_levels(int n, const SccTopoSorter::Graph &graph) {
    std::vector<int> n_deps(n + 1);
    for (int i = 1; i <= n; i++) {
        if (!graph.count(i)) {
            continue;
        }
        for (int to : graph.at(i)) {
            n_deps[to]++;
        }
    }

    // initialized unlocked queue to nodes without deps.
    std::queue<std::pair<int, int>> unlocked;
    for (int i = 1; i <= n; i++) {
        if (n_deps[i] == 0) {
            unlocked.push({i, 1});
        }
    }

    // toposort
    std::unordered_map<int, int> rst;
    while (!unlocked.empty()) {
        auto curr = unlocked.front();
        unlocked.pop();
        const int node_id = curr.first;
        const int n_level = curr.second;
        rst[node_id] = n_level;
        if (graph.count(node_id)) {
            for (int to : graph.at(node_id)) {
                n_deps[to]--;
                if (n_deps[to] == 0) {
                    unlocked.push({to, n_level + 1});
                }
            }
        }
    }
    return rst;
}

std::string string_graph(const int n, const SccTopoSorter::Graph &graph) {
    std::stringstream ss;
    for (int i = 1; i <= n; i++) {
        ss << i << ": [";
        if (graph.count(i)) {
            for (const int to : graph.at(i)) {
                ss << " " << to;
            }
        }
        ss << " ]\n";
    }
    return ss.str();
}

}  // namespace

int SccTopoSorter::new_node() {
    int id = ++n_nodes_i;
    graph_i[id] = {};
    return id;
}

void SccTopoSorter::validate_node_id(int id) const {
    if (id <= 0 || id > n_nodes_i) {
        std::stringstream ss;
        ss << "SccTopoSort::node_id out_of_range, valid range: [1, " << n_nodes_i << "]"
           << ", received: " << id;
        throw std::out_of_range(ss.str());
    }
}

void SccTopoSorter::add_dep(int from, int to) {
    validate_node_id(from);
    validate_node_id(to);
    graph_i[from].insert(to);
}

std::unordered_map<int, int> SccTopoSorter::scc_topo_sort() const {
    LOG5("scc_topo_sort input graph: ");
    LOG5(string_graph(n_nodes_i, graph_i));
    auto scc_mapping = tarjan(n_nodes_i, graph_i);

    // build a graph by replacing all nodes in a scc with one node.
    Graph non_scc_graph;
    int max_scc_id = 0;
    for (int i = 1; i <= n_nodes_i; i++) {
        const int from_scc_id = scc_mapping.at(i);
        max_scc_id = std::max(from_scc_id, max_scc_id);
        if (graph_i.count(i)) {
            for (const int &to : graph_i.at(i)) {
                const int to_scc_id = scc_mapping.at(to);
                if (from_scc_id != to_scc_id) {
                    non_scc_graph[from_scc_id].insert(to_scc_id);
                }
            }
        }
    }
    LOG5("scc_topo_sort non-scc graph: ");
    LOG5(string_graph(max_scc_id, non_scc_graph));

    // toposort on non-scc-graph
    auto topo_levels = toposort_levels(max_scc_id, non_scc_graph);

    // build result mapping
    std::unordered_map<int, int> rst;
    for (int i = 1; i <= n_nodes_i; i++) {
        rst[i] = topo_levels.at(scc_mapping.at(i));
    }
    return rst;
}
