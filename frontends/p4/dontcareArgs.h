/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_DONTCAREARGS_H_
#define FRONTENDS_P4_DONTCAREARGS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/// This class replaces don't care arguments (_) with
/// a temporary which is unused.
class DontcareArgs : public Transform, public ResolutionContext {
    MinimalNameGenerator nameGen;
    TypeMap *typeMap;
    IR::IndexedVector<IR::Declaration> toAdd;

 public:
    explicit DontcareArgs(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DontcareArgs");
    }
    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
    const IR::Node *postorder(IR::Function *function) override {
        if (!toAdd.empty()) {
            IR::IndexedVector<IR::StatOrDecl> body;
            for (auto d : toAdd) body.push_back(d);
            body.append(function->body->components);
            function->body =
                new IR::BlockStatement(function->body->srcInfo, function->body->annotations, body);
            toAdd.clear();
        }
        return function;
    }
    const IR::Node *postorder(IR::P4Action *action) override {
        if (!toAdd.empty()) {
            IR::IndexedVector<IR::StatOrDecl> body;
            for (auto d : toAdd) body.push_back(d);
            body.append(action->body->components);
            action->body =
                new IR::BlockStatement(action->body->srcInfo, action->body->annotations, body);
            toAdd.clear();
        }
        return action;
    }
    const IR::Node *postorder(IR::P4Parser *parser) override {
        if (!toAdd.empty()) {
            toAdd.append(parser->parserLocals);
            parser->parserLocals = toAdd;
            toAdd.clear();
        }
        return parser;
    }
    const IR::Node *postorder(IR::P4Control *control) override {
        if (!toAdd.empty()) {
            toAdd.append(control->controlLocals);
            control->controlLocals = toAdd;
            toAdd.clear();
        }
        return control;
    }
    profile_t init_apply(const IR::Node *node) override;
};

class RemoveDontcareArgs : public PassManager {
 public:
    explicit RemoveDontcareArgs(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DontcareArgs(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("RemoveDontcareArgs");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_DONTCAREARGS_H_ */
