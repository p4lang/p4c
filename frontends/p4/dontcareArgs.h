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

#ifndef _FRONTENDS_P4_DONTCAREARGS_H_
#define _FRONTENDS_P4_DONTCAREARGS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/// This class replaces don't care arguments (_) with
/// a temporary which is unused.
class DontcareArgs : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    IR::IndexedVector<IR::Declaration> toAdd;

 public:
    DontcareArgs(ReferenceMap* refMap, TypeMap* typeMap): refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DontcareArgs"); }
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::Function* function) override {
        IR::IndexedVector<IR::StatOrDecl> body;
        for (auto d : toAdd)
            body.push_back(d);
        body.append(function->body->components);
        function->body = new IR::BlockStatement(function->body->srcInfo, body);
        toAdd.clear();
        return function;
    }
    const IR::Node* postorder(IR::P4Parser* parser) override {
        toAdd.append(parser->parserLocals);
        parser->parserLocals = toAdd;
        toAdd.clear(); return parser; }
    const IR::Node* postorder(IR::P4Control* control) override {
        toAdd.append(control->controlLocals);
        control->controlLocals = toAdd;
        toAdd.clear(); return control; }
};

class RemoveDontcareArgs : public PassManager {
 public:
    RemoveDontcareArgs(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DontcareArgs(refMap, typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("RemoveDontcareArgs");
    }
};

}  // namespace P4

#endif  /* _FRONTENDS_P4_DONTCAREARGS_H_ */
