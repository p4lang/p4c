/*
Copyright 2017 VMware, Inc.

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

#ifndef FRONTENDS_P4_DIRECTCALLS_H_
#define FRONTENDS_P4_DIRECTCALLS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace p4c::P4 {

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

}  // namespace p4c::P4

#endif /* FRONTENDS_P4_DIRECTCALLS_H_ */
