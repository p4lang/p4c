/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "lib/nullstream.h"

#include "graphs.h"

namespace graphs {

Graphs::vertex_t Graphs::add_vertex(const cstring &name, VertexType type) {
    auto v = boost::add_vertex(*g);
    boost::put(&Vertex::name, *g, v, name);
    boost::put(&Vertex::type, *g, v, type);
    boost::put(&Vertex::visited, *g, v, false);
    return g->local_to_global(v);
}

Graphs::vertex_t Graphs::add_unique_vertex(const cstring &name, VertexType type) {
    Graph ug = *g;
    auto vertices = boost::vertices(*g);
    for (auto vit = vertices.first; vit != vertices.second; ++vit) {
        const auto &vinfo = ug[*vit];
        if (vinfo.name == name) return *vit;
    }
    return add_vertex(name, type);
}

void Graphs::add_edge(const vertex_t &from, const vertex_t &to, const cstring &name) {
    auto ep = boost::add_edge(from, to, g->root());
    boost::put(boost::edge_name, g->root(), ep.first, name);
}

void Graphs::add_unique_edge(const vertex_t &from, const vertex_t &to, const cstring &name) {
    auto outEdges = boost::out_edges(from, *g);
    for (auto oe = outEdges.first; oe != outEdges.second; ++oe) {
        cstring ename = boost::get(boost::edge_name, *g, *oe);
        if (ename == name) return;
    }
    add_edge(from, to, name);
}

boost::optional<Graphs::vertex_t> Graphs::merge_other_statements_into_vertex() {
    if (statementsStack.empty()) return boost::none;
    std::stringstream sstream;
    if (statementsStack.size() == 1) {
        statementsStack[0]->dbprint(sstream);
    } else if (statementsStack.size() == 2) {
        statementsStack[0]->dbprint(sstream);
        sstream << "\n";
        statementsStack[1]->dbprint(sstream);
    } else {
        statementsStack[0]->dbprint(sstream);
        sstream << "\n...\n";
        statementsStack.back()->dbprint(sstream);
    }
    auto v = add_vertex(cstring(sstream), VertexType::STATEMENTS);
    for (auto parent : parents)
        add_edge(parent.first, v, parent.second->label());
    parents = {{v, new EdgeUnconditional()}};
    statementsStack.clear();
    return v;
}

Graphs::vertex_t Graphs::add_and_connect_vertex(const cstring &name, VertexType type) {
    merge_other_statements_into_vertex();
    auto v = add_vertex(name, type);
    for (auto parent : parents)
        add_edge(parent.first, v, parent.second->label());
    return v;
}

Graphs::vertex_t Graphs::add_and_connect_unique_vertex(const cstring &name, VertexType type) {
    merge_other_statements_into_vertex();
    auto v = add_unique_vertex(name, type);
    for (auto parent : parents)
        add_edge(parent.first, v, parent.second->label());
    return v;
}

}  // namespace graphs
