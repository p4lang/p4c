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

/// Helpers for Boost graphs.

#ifndef BF_P4C_LIB_BOOST_GRAPH_H_
#define BF_P4C_LIB_BOOST_GRAPH_H_

#include <map>
#include <optional>

#include <boost/graph/adjacency_list.hpp>
#include <boost/tuple/tuple.hpp>

#include "lib/bitvec.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

using namespace P4;

/// Graph reachability via the Floyd-Warshall algorithm. Absent cycles, vertices are not considered
/// reachable from themselves.
template <class Graph>
class Reachability {
 public:
    using Vertex = typename Graph::vertex_descriptor;

    using EdgeSet =
        typename boost::container_gen<typename Graph::vertex_list_selector, bitvec>::type;

    /// Determines whether vertex @p v2 is reachable from the vertex @p v1.
    bool canReach(Vertex v1, Vertex v2) {
        if (forwardsReachableVertices.empty()) recompute();
        return forwardsReachableVertices.at(v1).getbit(v2);
    }

    /// @returns a bitvec representing all vertices that are reachable from @p v1 and
    /// can reach @p v2.
    bitvec reachableBetween(Vertex v1, Vertex v2) {
        if (forwardsReachableVertices.empty() || backwardsReachableVertices.empty()) recompute();
        return forwardsReachableVertices.at(v1) & backwardsReachableVertices.at(v2);
    }

    /// Clears reachability information so that it gets recomputed the next time @a canReach is
    /// called. This should be called at least once whenever the underlying graph changes after any
    /// call to @a canReach.
    void clear() {
        forwardsReachableVertices.clear();
        backwardsReachableVertices.clear();
    }

    /// Sets the sink node; no nodes will be considered reachable from this node.
    void setSink(std::optional<Vertex> sink) {
        this->sink = sink;
        clear();
    }

    explicit Reachability(const Graph &g) : g(g) {}

 private:
    /// Recomputes reachability information from the graph.
    void recompute() {
        forwardsReachableVertices.clear();
        backwardsReachableVertices.clear();

        forwardsReachableVertices.resize(boost::num_vertices(g), bitvec());
        backwardsReachableVertices.resize(boost::num_vertices(g), bitvec());

        // Ensure the reachability matrices have an entry for each vertex.
        typename Graph::vertex_iterator v, v_end;
        for (boost::tie(v, v_end) = boost::vertices(g); v != v_end; ++v) {
            forwardsReachableVertices[*v].clear();
            backwardsReachableVertices[*v].clear();
        }

        // Initialize with edges.
        typename Graph::edge_iterator edges, edges_end;
        for (boost::tie(edges, edges_end) = boost::edges(g); edges != edges_end; ++edges) {
            auto src = boost::source(*edges, g);
            auto dst = boost::target(*edges, g);
            forwardsReachableVertices[src].setbit(dst);
            backwardsReachableVertices[dst].setbit(src);
        }

        // Propagate reachability information via Floyd-Warshall.
        typename Graph::vertex_iterator mid, mid_end;
        for (boost::tie(mid, mid_end) = boost::vertices(g); mid != mid_end; ++mid) {
            // Ignore the sink node.
            if (sink && *mid == *sink) continue;

            typename Graph::vertex_iterator src, src_end;
            for (boost::tie(src, src_end) = boost::vertices(g); src != src_end; ++src) {
                recompute(backwardsReachableVertices, *src, *mid);

                // Ignore the sink node for forwards reachability.
                if (sink && *src == *sink) continue;
                recompute(forwardsReachableVertices, *src, *mid);
            }
        }
    }

    /// Helper for recompute(). If @p reachMatrix indicates that @p src can reach @p mid,
    /// then the entry for @p src is updated with nodes reachable from @p mid.
    void recompute(EdgeSet &reachMatrix, Vertex src, Vertex mid) {
        // If we can't reach mid from src, don't bother going through dsts.
        if (!reachMatrix[src].getbit(mid)) return;

        // This is a vectorized form of a loop that goes through all dsts and sets
        //
        //   reachMatrix[src][dst] |= reachMatrix[src][mid] & reachMatrix[mid][dst]
        //
        // while taking advantage of the fact that we know that
        //
        //   reachMatrix[src][mid] = 1
        reachMatrix[src] |= reachMatrix[mid];
    }

    /// The graph on which this object is operating.
    const Graph &g;

    /// Sink node. If provided, no nodes will be considered reachable from this node.
    std::optional<Vertex> sink = std::nullopt;

    /// Maps each vertex v to the set of vertices reachable from v. Vertices are not considered
    /// reachable from themselves unless the graph has cycles.
    EdgeSet forwardsReachableVertices;

    /// Maps each vertex v to the set of vertices that can reach v. Vertices are not considered
    /// reachable from themselves unless the graph has cycles.
    EdgeSet backwardsReachableVertices;
};

#endif /* BF_P4C_LIB_BOOST_GRAPH_H_ */
