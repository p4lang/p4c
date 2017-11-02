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

#include <boost/graph/graphviz.hpp>

#include <iostream>

#include "graphs.h"

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "lib/path.h"

namespace graphs {

using pGraph = ParserGraphs::Graph;
using vertex_t = ParserGraphs::vertex_t;

ParserGraphs::ParserGraphs(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                             const cstring &graphsDir)
    : refMap(refMap), typeMap(typeMap), graphsDir(graphsDir) {
    visitDagOnce = false;
}

void ParserGraphs::writeGraphToFile(const pGraph &g, const cstring &name) {
    auto path = Util::PathName(graphsDir).join(name + ".dot");
    auto out = openFile(path.toString(), false);
    if (out == nullptr) {
        ::error("Failed to open file %1%", path.toString());
        return;
    }
    boost::write_graphviz(*out, g);
}

cstring ParserGraphs::stringRepr(mpz_class value, unsigned bytes) {
    cstring sign = "";
    const char* r;
    cstring filler = "";
    if (value < 0) {
        value =- value;
        r = mpz_get_str(nullptr, 16, value.get_mpz_t());
        sign = "-";
    } else {
        r = mpz_get_str(nullptr, 16, value.get_mpz_t());
    }
    if (bytes > 0) {
        int digits = bytes * 2 - strlen(r);
        BUG_CHECK(digits >= 0, "Cannot represent %1% on %2% bytes", value, bytes);
        filler = std::string(digits, '0');
    }
    return sign + "0x" + filler + r;
}

void ParserGraphs::convertSimpleKey(const IR::Expression* keySet,
                                    mpz_class& value, mpz_class& mask) {
    if (keySet->is<IR::Mask>()) {
        auto mk = keySet->to<IR::Mask>();
        if (!mk->left->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->left);
            return;
        }
        if (!mk->right->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->right);
            return;
        }
        value = mk->left->to<IR::Constant>()->value;
        mask = mk->right->to<IR::Constant>()->value;
    } else if (keySet->is<IR::Constant>()) {
        value = keySet->to<IR::Constant>()->value;
        mask = -1;
    } else if (keySet->is<IR::BoolLiteral>()) {
        value = keySet->to<IR::BoolLiteral>()->value ? 1 : 0;
        mask = -1;
    } else {
        ::error("%1% must evaluate to a compile-time constant", keySet);
        value = 0;
        mask = 0;
    }
}

unsigned ParserGraphs::combine(const IR::Expression* keySet,
                                const IR::ListExpression* select,
                                mpz_class& value, mpz_class& mask) {
    value = 0;
    mask = 0;
    unsigned totalWidth = 0;
    if (keySet->is<IR::DefaultExpression>()) {
        return totalWidth;
    } else if (keySet->is<IR::ListExpression>()) {
        auto le = keySet->to<IR::ListExpression>();
        BUG_CHECK(le->components.size() == select->components.size(),
                "%1%: mismatched select", select);
        unsigned index = 0;

        bool noMask = true;
        for (auto it = select->components.begin();
                it != select->components.end(); ++it) {
            auto e = *it;
            auto keyElement = le->components.at(index);

            auto type = typeMap->getType(e, true);
            int width = type->width_bits();
            BUG_CHECK(width > 0, "%1%: unknown width", e);

            mpz_class key_value, mask_value;
            convertSimpleKey(keyElement, key_value, mask_value);
            unsigned w = 8 * ROUNDUP(width, 8);
            totalWidth += ROUNDUP(width, 8);
            value = Util::shift_left(value, w) + key_value;
            if (mask_value != -1) {
                mask = Util::shift_left(mask, w) + mask_value;
                noMask = false;
            }
            LOG3("Shifting " << " into key " << key_value << " &&& " << mask_value <<
                    " result is " << value << " &&& " << mask);
            index++;
        }

        if (noMask)
            mask = -1;
        return totalWidth;
    } else {
        BUG_CHECK(select->components.size() == 1, "%1%: mismatched select/label", select);
        convertSimpleKey(keySet, value, mask);
        auto type = typeMap->getType(select->components.at(0), true);
        return ROUNDUP(type->width_bits(), 8);
    }
}

bool ParserGraphs::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::ParserBlock>()) {
            auto name = it.second->to<IR::ParserBlock>()->container->name;
            pGraph g_;
            g = &g_;
            boost::get_property(g_, boost::graph_name) = name;
            start_v = add_vertex("_START_", VertexType::PARSER);
            exit_v = add_vertex("_EXIT_", VertexType::PARSER);
            accept_v = add_vertex("accept", VertexType::DEFAULT);
            parents = {{start_v, new EdgeUnconditional()}};

            visit(it.second->getNode());

            for (auto parent : defParents)
                add_edge(parent.first, accept_v, parent.second->label());
            add_edge(accept_v, exit_v, (new EdgeUnconditional())->label());
            BUG_CHECK(g_.is_root(), "Invalid graph");
            GraphAttributeSetter()(g_);
            writeGraphToFile(g_, name);
        }
    }
    return false;
}

bool ParserGraphs::preorder(const IR::ParserBlock *block) {
    visit(block->container);
    return false;
}

bool ParserGraphs::preorder(const IR::P4Parser *pars) {
    for (auto state : pars->states) {
        if (state->name == IR::ParserState::start) {
            visit(state);
            break;
        }
    }
    return false;
}

bool ParserGraphs::preorder(const IR::ParserState *state) {
    std::stringstream sstream;
    if (state->selectExpression != nullptr) {
        VertexType vertexType;
        if (state->name == IR::ParserState::start) {
            vertexType = VertexType::START;
        } else {
            vertexType = VertexType::HEADER;
        }
        auto v = add_and_connect_vertex(state->name, vertexType);
        parents.clear();

        if (state->selectExpression->is<IR::PathExpression>()) {
            parents = {{v, new EdgeSwitch(new IR::DefaultExpression())}};
            auto stpath = state->selectExpression->to<IR::PathExpression>();
            auto decl = refMap->getDeclaration(stpath->path, true);
            auto nstate = decl->to<IR::ParserState>();
            if (nstate->name == IR::ParserState::accept) {
                defParents.emplace_back(v, new EdgeSwitch(new IR::DefaultExpression()));
                return false;
            }
            visit(decl->to<IR::ParserState>());
        } else if (state->selectExpression->is<IR::SelectExpression>()) {
            auto se = state->selectExpression->to<IR::SelectExpression>();
            for (auto sc : se->selectCases) {
                auto key = sc->keyset;
                auto pexpr = sc->state;
                key->dbprint(sstream);
                mpz_class value, mask;
                unsigned bytes = combine(sc->keyset, se->select, value, mask);
                parents = {{v, new EdgeSelect(stringRepr(value, bytes))}};
                auto decl = refMap->getDeclaration(pexpr->path, true);
                auto nstate = decl->to<IR::ParserState>();
                if (nstate->name == IR::ParserState::accept) {
                    defParents.emplace_back(v, new EdgeSwitch(new IR::DefaultExpression()));
                    continue;
                }
                visit(nstate);
            }
        } else {
            LOG1("unexpected selectExpression");
        }

    } else {
        LOG1("default case");
    }

    return false;
}

}  // namespace graphs
