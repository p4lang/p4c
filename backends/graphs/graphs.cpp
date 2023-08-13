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

#include "graphs.h"

#include <ostream>
#include <string>

#include "lib/stringify.h"

namespace graphs {

Graphs::vertex_t Graphs::add_vertex(const cstring &name, VertexType type) {
    auto v = boost::add_vertex(*g);
    boost::put(&Vertex::name, *g, v, name);
    boost::put(&Vertex::type, *g, v, type);
    return g->local_to_global(v);
}

void Graphs::add_edge(const vertex_t &from, const vertex_t &to, const cstring &name) {
    auto ep = boost::add_edge(from, to, g->root());
    boost::put(boost::edge_name, g->root(), ep.first, name);
}

void Graphs::add_edge(const vertex_t &from, const vertex_t &to, const cstring &name,
                      unsigned cluster_id) {
    auto ep = boost::add_edge(from, to, g->root());
    boost::put(boost::edge_name, g->root(), ep.first, name);

    auto attrs = boost::get(boost::edge_attribute, g->root());

    attrs[ep.first]["ltail"] = "cluster" + Util::toString(cluster_id - 2);
    attrs[ep.first]["lhead"] = "cluster" + Util::toString(cluster_id - 1);
}

void Graphs::limitStringSize(std::stringstream &sstream, std::stringstream &helper_sstream) {
    if (helper_sstream.str().size() > 25) {
        sstream << helper_sstream.str().substr(0, 25) << "...";
    } else {
        sstream << helper_sstream.str();
    }
    helper_sstream.str("");
    helper_sstream.clear();
}

std::optional<Graphs::vertex_t> Graphs::merge_other_statements_into_vertex() {
    if (statementsStack.empty()) return std::nullopt;
    std::stringstream sstream;
    std::stringstream helper_sstream;  // to limit line width

    if (statementsStack.size() == 1) {
        statementsStack[0]->dbprint(helper_sstream);
        limitStringSize(sstream, helper_sstream);
    } else if (statementsStack.size() == 2) {
        statementsStack[0]->dbprint(helper_sstream);
        limitStringSize(sstream, helper_sstream);
        sstream << "\\n";
        statementsStack[1]->dbprint(helper_sstream);
        limitStringSize(sstream, helper_sstream);
    } else {
        statementsStack[0]->dbprint(helper_sstream);
        limitStringSize(sstream, helper_sstream);
        sstream << "\\n...\\n";
        statementsStack.back()->dbprint(helper_sstream);
        limitStringSize(sstream, helper_sstream);
    }
    auto v = add_vertex(cstring(sstream), VertexType::STATEMENTS);
    for (auto parent : parents) add_edge(parent.first, v, parent.second->label());
    parents = {{v, new EdgeUnconditional()}};
    statementsStack.clear();
    return v;
}

Graphs::vertex_t Graphs::add_and_connect_vertex(const cstring &name, VertexType type) {
    merge_other_statements_into_vertex();
    auto v = add_vertex(name, type);
    for (auto parent : parents) add_edge(parent.first, v, parent.second->label());
    return v;
}

}  // namespace graphs
