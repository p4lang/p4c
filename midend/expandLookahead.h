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

#ifndef MIDEND_EXPANDLOOKAHEAD_H_
#define MIDEND_EXPANDLOOKAHEAD_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/// Given an assignment like
/// a = lookahead<T>();
/// this is transformed into
/// bit<sizeof(T)> tmp = lookahead<bit<sizeof(T)>>();
/// a.setValid();  // for header fields
/// a.m0 = tmp[f1,f0];
/// a.m1 = tmp[f2, f1+1];
/// ...
///
/// Optional constructor argument expandHeader sets the expandHeader flag which
/// determines whether headers (IR::Type_Header) are expanded or not.
/// Default value for the flag (when optional constructor argument is not used) is
/// true, which means that by default headers are expanded.
class DoExpandLookahead : public Transform {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    IR::IndexedVector<IR::Declaration> newDecls;
    /**
     * Some targets may support lookahead with header type argument directly without
     * need of expansion, but they might require to expand structures.
     * This pass can be used for such targets with expandHeader set to false.
     */
    bool expandHeader = true;

    struct ExpansionInfo {
        const IR::Statement *statement;
        unsigned width;
        const IR::Type *origType;
        const IR::PathExpression *tmp;  // temporary used for result
    };

    void expand(const IR::PathExpression *bitvector, const IR::Type *type, unsigned *offset,
                const IR::Expression *destination, IR::IndexedVector<IR::StatOrDecl> *output);
    ExpansionInfo *convertLookahead(const IR::MethodCallExpression *expression);

 public:
    DoExpandLookahead(ReferenceMap *refMap, TypeMap *typeMap, bool expandHeader = true)
        : refMap(refMap), typeMap(typeMap), expandHeader(expandHeader) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("DoExpandLookahead");
    }
    const IR::Node *postorder(IR::AssignmentStatement *statement) override;
    const IR::Node *postorder(IR::MethodCallStatement *statement) override;
    const IR::Node *preorder(IR::P4Control *control) override {
        prune();
        return control;
    }
    const IR::Node *preorder(IR::P4Parser *parser) override {
        newDecls.clear();
        return parser;
    }
    const IR::Node *postorder(IR::P4Parser *parser) override {
        if (!newDecls.empty()) parser->parserLocals.append(newDecls);
        return parser;
    }
};

/// Optional constructor argument expandHeader determines whether headers are
/// expanded or not.
/// See also description in class DoExpandLookahead.
class ExpandLookahead : public PassManager {
 public:
    ExpandLookahead(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr,
                    bool expandHeader = true) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoExpandLookahead(refMap, typeMap, expandHeader));
        setName("ExpandLookahead");
    }
};

}  // namespace P4

#endif /* MIDEND_EXPANDLOOKAHEAD_H_ */
