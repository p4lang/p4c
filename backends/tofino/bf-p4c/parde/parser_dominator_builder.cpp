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

#include "parser_dominator_builder.h"

Visitor::profile_t ParserDominatorBuilder::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);

    parser_graphs.clear();
    immediate_dominators.clear();
    immediate_post_dominators.clear();
    states.clear();

    return rv;
}

bool ParserDominatorBuilder::preorder(const IR::BFN::Parser *parser) {
    parser_graphs[parser->gress].parser = parser;
    return true;
}

bool ParserDominatorBuilder::preorder(const IR::BFN::ParserState *parser_state) {
    states.insert(parser_state);
    for (auto transition : parser_state->transitions)
        parser_graphs[parser_state->gress].add_edge(parser_state, transition->next, transition);

    // TODO: Workaround for edge case caused by compiler generated state egress::$mirror
    // not having any transitions and therefore not leading to End of Parser (EOP, nullptr) when
    // compiling some P4_14 programs. This is most likely a bug, as all states in the parser should
    // have a path leading to EOP.
    //
    if (parser_state->transitions.empty())
        parser_graphs[parser_state->gress].add_edge(parser_state, nullptr, nullptr);

    return true;
}

ParserDominatorBuilder::IndexImmediateDominatorMap ParserDominatorBuilder::get_immediate_dominators(
    Graph graph, Vertex entry) {
    return get_immediate_dominators(graph, entry, false);
}

ParserDominatorBuilder::IndexImmediateDominatorMap
ParserDominatorBuilder::get_immediate_post_dominators(boost::reverse_graph<Graph> graph,
                                                      Vertex entry) {
    return get_immediate_dominators(graph, entry, true);
}

ParserDominatorBuilder::StateImmediateDominatorMap ParserDominatorBuilder::index_map_to_state_map(
    IndexImmediateDominatorMap &idom, ReversibleParserGraph &rpg) {
    LOG7("Converting from int map to a parser state map");
    StateImmediateDominatorMap idom_state;
    for (auto &kv : idom) {
        BUG_CHECK(rpg.vertex_to_state.count(kv.first),
                  "Unknown parser state with index %1% not found", kv.first);
        const IR::BFN::ParserState *state = rpg.vertex_to_state.at(kv.first);
        const IR::BFN::ParserState *dominator;
        if (kv.second == -1) {
            dominator = state;
        } else {
            BUG_CHECK(rpg.vertex_to_state.count(kv.second),
                      "Unknown parser state with index %1% not found", kv.second);
            dominator = rpg.vertex_to_state.at(kv.second);
        }
        idom_state[state] = dominator;
        LOG7(TAB1 << ((state) ? state->name : "END OF PARSER") << " --> "
                  << ((dominator) ? dominator->name : "END OF PARSER"));
    }
    return idom_state;
}

std::set<const IR::BFN::ParserState *> ParserDominatorBuilder::get_all_dominatees(
    const IR::BFN::ParserState *state, gress_t gress, bool get_post_dominatees) {
    std::set<const IR::BFN::ParserState *> dominatees;
    if (state != nullptr) gress = state->gress;

    StateImmediateDominatorMap idom_map =
        get_post_dominatees ? immediate_post_dominators.at(gress) : immediate_dominators.at(gress);
    for (const auto &kv : idom_map) {
        if (kv.second == state && kv.first != state && kv.first != nullptr) {
            auto child_dominatees = get_all_dominatees(kv.first, gress, get_post_dominatees);
            dominatees.insert(kv.first);
            dominatees.insert(child_dominatees.begin(), child_dominatees.end());
        }
    }
    return dominatees;
}

std::set<const IR::BFN::ParserState *> ParserDominatorBuilder::get_all_dominators(
    const IR::BFN::ParserState *state, gress_t gress, bool get_post_dominators) {
    std::set<const IR::BFN::ParserState *> dominators;
    if (state != nullptr) gress = state->gress;

    StateImmediateDominatorMap idom_map =
        get_post_dominators ? immediate_post_dominators.at(gress) : immediate_dominators.at(gress);
    auto immediate_dominator = idom_map.at(state);
    if (state == immediate_dominator) return dominators;

    if (!get_post_dominators || immediate_dominator != nullptr)
        dominators.insert(immediate_dominator);
    auto idom_dominators = get_all_dominators(immediate_dominator, gress, get_post_dominators);
    dominators.insert(idom_dominators.begin(), idom_dominators.end());
    return dominators;
}

std::set<const IR::BFN::ParserState *> ParserDominatorBuilder::get_all_dominatees(
    const IR::BFN::ParserState *state, gress_t gress) {
    return get_all_dominatees(state, gress, false);
}

std::set<const IR::BFN::ParserState *> ParserDominatorBuilder::get_all_post_dominatees(
    const IR::BFN::ParserState *state, gress_t gress) {
    return get_all_dominatees(state, gress, true);
}

std::set<const IR::BFN::ParserState *> ParserDominatorBuilder::get_all_dominators(
    const IR::BFN::ParserState *state, gress_t gress) {
    return get_all_dominators(state, gress, false);
}

std::set<const IR::BFN::ParserState *> ParserDominatorBuilder::get_all_post_dominators(
    const IR::BFN::ParserState *state, gress_t gress) {
    return get_all_dominators(state, gress, true);
}

void ParserDominatorBuilder::build_dominator_maps() {
    LOG3("Building immediate dominator and post-dominator maps.");
    for (auto &kv : parser_graphs) {
        auto idoms = get_immediate_dominators(kv.second.graph, kv.second.get_entry_point());
        auto idom_map = index_map_to_state_map(idoms, kv.second);

        auto reversed_graph = boost::make_reverse_graph(kv.second.graph);
        auto ipdoms = get_immediate_post_dominators(reversed_graph, *kv.second.end);
        auto ipdom_map = index_map_to_state_map(ipdoms, kv.second);

        immediate_dominators.emplace(kv.first, idom_map);
        immediate_post_dominators.emplace(kv.first, ipdom_map);
    }
}

void ParserDominatorBuilder::end_apply() { build_dominator_maps(); }
