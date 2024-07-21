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

#include "moveConstructors.h"

namespace p4c::P4 {

MoveConstructors::MoveConstructors() : convert(Region::Outside) { setName("MoveConstructors"); }

Visitor::profile_t MoveConstructors::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    node->apply(nameGen);

    return rv;
}

const IR::Node *MoveConstructors::preorder(IR::P4Parser *parser) {
    cmap.clear();
    convert = Region::InParserStateful;
    visit(parser->parserLocals, "parserLocals");
    convert = Region::InBody;
    visit(parser->states, "states");
    convert = Region::Outside;
    prune();
    return parser;
}

const IR::Node *MoveConstructors::preorder(IR::IndexedVector<IR::Declaration> *declarations) {
    if (convert != Region::InParserStateful) return declarations;

    bool changes = false;
    auto result = new IR::Vector<IR::Declaration>();
    for (auto s : *declarations) {
        visit(s);
        for (auto e : cmap.tmpName) {
            auto cce = e.first;
            auto decl = new IR::Declaration_Instance(cce->srcInfo, e.second, cce->constructedType,
                                                     cce->arguments);
            result->push_back(decl);
            changes = true;
        }
        result->push_back(s);
        cmap.clear();
    }
    prune();
    if (changes) return result;
    return declarations;
}

const IR::Node *MoveConstructors::postorder(IR::P4Parser *parser) {
    if (cmap.empty()) return parser;
    for (auto e : cmap.tmpName) {
        auto cce = e.first;
        auto decl = new IR::Declaration_Instance(cce->srcInfo, e.second, cce->constructedType,
                                                 cce->arguments);
        parser->parserLocals.insert(parser->parserLocals.begin(), decl);
    }
    return parser;
}

const IR::Node *MoveConstructors::preorder(IR::P4Control *control) {
    cmap.clear();
    convert = Region::InControlStateful;
    IR::IndexedVector<IR::Declaration> newDecls;
    bool changes = false;
    for (auto decl : control->controlLocals) {
        visit(decl);
        for (auto e : cmap.tmpName) {
            auto cce = e.first;
            auto inst = new IR::Declaration_Instance(cce->srcInfo, e.second, cce->constructedType,
                                                     cce->arguments);
            newDecls.push_back(inst);
            changes = true;
        }
        newDecls.push_back(decl);
        cmap.clear();
    }
    convert = Region::InBody;
    visit(control->body);
    convert = Region::Outside;
    prune();
    if (changes) {
        control->controlLocals = newDecls;
    }
    return control;
}

const IR::Node *MoveConstructors::postorder(IR::P4Control *control) {
    if (cmap.empty()) return control;
    IR::IndexedVector<IR::Declaration> newDecls;
    for (auto e : cmap.tmpName) {
        auto cce = e.first;
        auto decl = new IR::Declaration_Instance(cce->srcInfo, e.second, cce->constructedType,
                                                 cce->arguments);
        newDecls.push_back(decl);
    }
    newDecls.append(control->controlLocals);
    control->controlLocals = newDecls;
    return control;
}

const IR::Node *MoveConstructors::preorder(IR::P4Table *table) {
    prune();
    return table;
}  // skip

const IR::Node *MoveConstructors::postorder(IR::ConstructorCallExpression *expression) {
    if (convert == Region::Outside) return expression;
    auto tmpvar = nameGen.newName("tmp");
    auto tmpref = new IR::PathExpression(IR::ID(expression->srcInfo, tmpvar));
    cmap.add(expression, tmpvar);
    return tmpref;
}

}  // namespace p4c::P4
