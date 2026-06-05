/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_DIRECTCALLS_H_
#define FRONTENDS_P4_DIRECTCALLS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

/**
   This pass replaces direct invocations of controls or parsers
   with an instantiation followed by an invocation.  For example:

control c() { apply {} }
control d() { apply { c.apply(); }}

is replaced with

control c() { apply {} }
control d() { @name("c") c() c_inst; { c_inst.apply(); }}
*/
class InstantiateDirectCalls : public Transform, public ResolutionContext {
    MinimalNameGenerator nameGen;  // used to generate new names

    IR::IndexedVector<IR::Declaration> insert;

 public:
    InstantiateDirectCalls() { setName("InstantiateDirectCalls"); }
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::MethodCallExpression *expression) override;

    profile_t init_apply(const IR::Node *node) override {
        auto rv = Transform::init_apply(node);
        node->apply(nameGen);
        return rv;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_DIRECTCALLS_H_ */
