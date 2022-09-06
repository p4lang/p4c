/*
Copyright 2022 Intel Corp.

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

#ifndef _MIDEND_FLATTENUNIONS_H_
#define _MIDEND_FLATTENUNIONS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "./frontends/p4/simplifyDefUse.h"
#include "./frontends/p4/unusedDeclarations.h"
#include "./frontends/p4/parserControlFlow.h"
namespace P4 {

/**
 * Flatten header union into its individual element.
 * For ex:
 * header_union U {
 *     Hdr1 h1;
 *     Hdr2 h2;
 * }
 *
 * struct Headers {
 *     Hdr1 h1;
 *     U u;
 * }
 *
 * is replaced by
 * struct  Headers {
 *     Hdr1 h1;
 *     Hdr1 u_h1;
 *     Hdr2 u_h2;
 * }
 */
class DoFlattenHeaderUnion : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    IR::IndexedVector<IR::Declaration> toInsert;  // temporaries
    std::map<cstring, std::map<cstring, cstring>> replacementMap;

 public:
    DoFlattenHeaderUnion(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) :
                         refMap(refMap), typeMap(typeMap){}
    const IR::Node* postorder(IR::Type_Struct* sf) override;
    const IR::Node* postorder(IR::Declaration_Variable *dv) override;
    const IR::Node* postorder(IR::Member* m) override;
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
};

class HandleValidityHeaderUnion : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    IR::IndexedVector<IR::Declaration> toInsert;  // temporaries


 public:
    HandleValidityHeaderUnion(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) :
                         refMap(refMap), typeMap(typeMap){}
    const IR::Node* postorder(IR::AssignmentStatement* assn) override;
    const IR::Node* postorder(IR::IfStatement *a) override;
    const IR::Node* postorder(IR::SwitchStatement* a) override;
    const IR::Node* postorder(IR::MethodCallStatement* mcs) override;
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::MethodCallStatement* processValidityForStr(const IR::Statement *s,
                                                         const IR::Member *m,
                                                         cstring headerElement,
                                                         cstring setValid);
    const IR::Node* setInvalidforRest(const IR::Statement *s, const IR::Member *m,
                                      const IR::Type_HeaderUnion *hu, cstring exclude,
                                      bool setValidforCurrMem);
    const IR::Node * expandIsValid(const IR::Statement *a, const IR::MethodCallExpression *mce);
};


class FlattenHeaderUnion : public PassManager {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;

 public:
    FlattenHeaderUnion(P4::ReferenceMap *refMap, P4::TypeMap* typeMap) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new P4::SimplifyDefUse(refMap, typeMap));
        passes.push_back(new HandleValidityHeaderUnion(refMap, typeMap));
        passes.push_back(new DoFlattenHeaderUnion(refMap, typeMap));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.emplace_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new P4::SimplifyDefUse(refMap, typeMap));
        passes.push_back(new P4::RemoveAllUnusedDeclarations(refMap));
        passes.push_back(new P4::RemoveParserIfs(refMap, typeMap));
    }
};
}  // namespace P4

#endif /* _MIDEND_FLATTENUNIONS_H_ */
