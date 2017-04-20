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
#include "methodInstance.h"

namespace P4 {

using ::Model::Type_Model;
using ::Model::Param_Model;

bool InferArchitecture::preorder(const IR::P4Program* program) {
    for (auto decl : program->declarations) {
        if (decl->is<IR::Type_Package>() || decl->is<IR::Type_Extern>()) {
            LOG3("[ " << decl << " ]");
            visit(decl);
        }
    }
    return false;
}

bool InferArchitecture::preorder(const IR::Type_Control *node) {
    Control_Model *control_m = v2model.controls.back();
    uint32_t paramCounter = 0;
    for (auto param : node->applyParams->parameters) {
        Type_Model paramTypeModel(param->type->toString());
        Param_Model newParam(param->toString(), paramTypeModel, paramCounter++);
        control_m->elems.push_back(newParam);
    }
    LOG3("...control [" << node << " ]");
    return false;
}

/// new Parser_Model object
bool InferArchitecture::preorder(const IR::Type_Parser *node) {
    Parser_Model *parser_m = v2model.parsers.back();
    uint32_t paramCounter = 0;
    for (auto param : *node->applyParams->getEnumerator()) {
        Type_Model paramTypeModel(param->type->toString());
        Param_Model newParam(param->toString(), paramTypeModel, paramCounter++);
        parser_m->elems.push_back(newParam);
        LOG3("...... parser params [ " << param << " ]");
    }
    return false;
}

/// FIXME: going from param to p4type takes a lot of work, should be as simple as
/// p4type = p4Type(param);
/// if (p4type->is<IR::Type_Parser>) { visit(p4type); }
/// else if (p4type->is<IR::Type_Control>) { visit(p4type); }
bool InferArchitecture::preorder(const IR::Type_Package *node) {
    for (auto param : node->constructorParams->parameters) {
        BUG_CHECK(param->type->is<IR::Type_Specialized>(),
                  "Unexpected Package param type");
        auto baseType = param->type->to<IR::Type_Specialized>()->baseType;
        auto typeObj = typeMap->getType(baseType)->getP4Type();
        if (typeObj->is<IR::Type_Parser>()) {
            v2model.parsers.push_back(new Parser_Model(param->toString()));
            visit(typeObj->to<IR::Type_Parser>());
        } else if (typeObj->is<IR::Type_Control>()) {
            v2model.controls.push_back(new Control_Model(param->toString()));
            visit(typeObj->to<IR::Type_Control>());
        }
    }
    LOG3("...package [ " << node << " ]");
    return false;
}

/// create new Extern_Model object
/// add method and its parameters to extern.
bool InferArchitecture::preorder(const IR::Type_Extern *node) {
    Extern_Model *extern_m = new Extern_Model(node->toString());
    for (auto method : node->methods) {
        uint32_t paramCounter = 0;
        Method_Model method_m(method->toString());
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
    Type_Model *match_kind = new Type_Model(kind->toString());
    v2model.match_kinds.push_back(match_kind);
    LOG3("... match kind " << match_kind);
    return false;
}

} // namespace P4
