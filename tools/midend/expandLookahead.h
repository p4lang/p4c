/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_EXPANDLOOKAHEAD_H_
#define _MIDEND_EXPANDLOOKAHEAD_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

/// Given an assignment like
/// a = lookahead<T>();
/// this is transformed into
/// bit<X> tmp = lookahead<sizeof<T>>();
/// a = { tmp[f1,f0], tmp[f2, f1+1], ... }
class DoExpandLookahead : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    IR::IndexedVector<IR::Declaration> newDecls;

    struct ExpansionInfo {
        const IR::Statement* statement;
        unsigned width;
        const IR::Type* origType;
        const IR::PathExpression* tmp;  // temporary used for result
    };

    const IR::Expression* expand(
        const IR::PathExpression* base, const IR::Type* type, unsigned* offset);
    void expandSetValid(const IR::Expression* base, const IR::Type* type,
                        IR::IndexedVector<IR::StatOrDecl>* output);
    ExpansionInfo* convertLookahead(const IR::MethodCallExpression* expression);

 public:
    DoExpandLookahead(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoExpandLookahead"); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { prune(); return control; }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { newDecls.clear(); return parser; }
    const IR::Node* postorder(IR::P4Parser* parser) override {
        if (!newDecls.empty())
            parser->parserLocals.append(newDecls);
        return parser;
    }
};

class ExpandLookahead : public PassManager {
 public:
    ExpandLookahead(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoExpandLookahead(refMap, typeMap));
        setName("ExpandLookahead");
    }
};

}  // namespace P4

#endif /* _MIDEND_EXPANDLOOKAHEAD_H_ */
