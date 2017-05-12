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

#include "inferArchitecture.h"

namespace BMV2 {

using ::Model::Type_Model;
using ::Model::Param_Model;

bool InferArchitecture::preorder(const IR::P4Program* program) {
    for (auto decl : program->declarations) {
        if (decl->is<IR::Type_Package>() || decl->is<IR::Type_Extern>() ||
            decl->is<IR::Declaration_MatchKind>()) {
            // LOG3("[ " << decl << " ]");
            visit(decl);
        }
    }
    return true;
}

bool InferArchitecture::preorder(const IR::Type_Control *node) {
    if (v2model.controls.size() != 0) {
        P4::Control_Model *control_m = v2model.controls.back();
        uint32_t paramCounter = 0;
        for (auto param : node->applyParams->parameters) {
            Type_Model paramTypeModel(param->type->toString());
            Param_Model newParam(param->toString(), paramTypeModel, paramCounter++);
            control_m->elems.push_back(newParam);
        }
    }
    LOG3("...control [" << node << " ]");
    return false;
}

/// new P4::Parser_Model object
bool InferArchitecture::preorder(const IR::Type_Parser *node) {
    if (v2model.parsers.size() != 0) {
        P4::Parser_Model *parser_m = v2model.parsers.back();
        uint32_t paramCounter = 0;
        for (auto param : *node->applyParams->getEnumerator()) {
            Type_Model paramTypeModel(param->type->toString());
            Param_Model newParam(param->toString(), paramTypeModel, paramCounter++);
            parser_m->elems.push_back(newParam);
            LOG3("...... parser params [ " << param << " ]");
        }
    }
    return false;
}

/// FIXME: going from param to p4type takes a lot of work, should be as simple as
/// p4type = p4Type(param);
/// if (p4type->is<IR::Type_Parser>) { visit(p4type); }
/// else if (p4type->is<IR::Type_Control>) { visit(p4type); }
bool InferArchitecture::preorder(const IR::Type_Package *node) {
    for (auto p : node->getConstructorParameters()->parameters) {
        LOG1("package constructor param: " << p);
        auto ptype = typeMap->getType(p);
        if (ptype == nullptr)
            continue;
        if (ptype->is<IR::P4Parser>()) {
            LOG3("new parser model: " << p->toString());
            v2model.parsers.push_back(new P4::Parser_Model(p->toString()));
            visit(ptype->to<IR::P4Parser>()->type);
        } else if (ptype->is<IR::P4Control>()) {
            LOG3("new control model: " << p->toString());
            v2model.controls.push_back(new P4::Control_Model(p->toString()));
            visit(ptype->to<IR::P4Control>()->type);
        }
    }
    LOG3("...package [ " << node << " ]");
    return false;
}

/// create new Extern_Model object
/// add method and its parameters to extern.
bool InferArchitecture::preorder(const IR::Type_Extern *node) {
    P4::Extern_Model *extern_m = new P4::Extern_Model(node->toString());
    for (auto method : node->methods) {
        uint32_t paramCounter = 0;
        P4::Method_Model method_m(method->toString());
        for (auto param : *method->type->parameters->getEnumerator()) {
            Type_Model paramTypeModel(param->type->toString());
            Param_Model newParam(param->toString(), paramTypeModel,
                                 paramCounter++);
            method_m.elems.push_back(newParam);
        }
        extern_m->elems.push_back(method_m);
    }
    v2model.externs.push_back(extern_m);
    LOG3("...extern [ " << node << " ]");
    return false;
}

/// infer match type?
bool InferArchitecture::preorder(const IR::Declaration_MatchKind* kind) {
    /// add to match_kind model
    for (auto member : kind->members) {
        Type_Model *match_kind = new Type_Model(member->toString());
        v2model.match_kinds.push_back(match_kind);
        LOG1("... match kind " << match_kind);
    }
    return false;
}

}  // namespace P4
