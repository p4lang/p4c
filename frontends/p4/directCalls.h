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

#ifndef _FRONTENDS_P4_DIRECTCALLS_H_
#define _FRONTENDS_P4_DIRECTCALLS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

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
class DoInstantiateCalls : public Transform {
    ReferenceMap* refMap;

    IR::IndexedVector<IR::Declaration> insert;
 public:
    explicit DoInstantiateCalls(ReferenceMap* refMap) : refMap(refMap) {
        CHECK_NULL(refMap);
        setName("DoInstantiateCalls");
    }
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
};

class InstantiateDirectCalls : public PassManager {
 public:
    explicit InstantiateDirectCalls(ReferenceMap* refMap) {
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new DoInstantiateCalls(refMap));
        setName("InstantiateDirectCalls");
    }
};

}  // namespace P4

#endif  /* _FRONTENDS_P4_DIRECTCALLS_H_ */
