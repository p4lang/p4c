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

#ifndef MIDEND_FLATTENUNIONS_H_
#define MIDEND_FLATTENUNIONS_H_

#include "./frontends/p4/parserControlFlow.h"
#include "./frontends/p4/simplifyDefUse.h"
#include "./frontends/p4/unusedDeclarations.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
namespace P4 {

/**
 * Flatten header union into its individual element. All occurrences of the header union
 * variables are replaced by the new header type variables.
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
 protected:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    std::map<cstring, std::map<cstring, cstring>> replacementMap;
    // Replacement map needed to add element-wise header declaration in right context
    std::map<IR::Declaration_Variable *, IR::IndexedVector<IR::Declaration>> replaceDVMap;

 public:
    DoFlattenHeaderUnion(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}
    const IR::Node *postorder(IR::Type_Struct *sf) override;
    const IR::Node *postorder(IR::Declaration_Variable *dv) override;
    const IR::Node *postorder(IR::Member *m) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::P4Action *action) override;
    bool hasHeaderUnionField(IR::Type_Struct *s);
};

/**
 * Flatten header union stack variabls into its individual elements. All occurrences of the header
 * union stack variables are replaced by the elements in the stack.
 * For ex:
 * header_union U {
 *     Hdr1 h1;
 *     Hdr2 h2;
 * }
 *
 * struct Headers {
 *     Hdr1 h1;
 *     U[2] u;
 * }
 *
 * is replaced by
 * struct  Headers {
 *     Hdr1 h1;
 *     U u0;
 *     U u1;
 * }
 * References to u[0] are replaced by u0. Likewise for all stack elements.
 *
 * This pass assumes that HSIndexSimplifier, ParsersUnroll passes are run before this and all
 * indices for header union stack variables are constants.
 *
 */
class DoFlattenHeaderUnionStack : public DoFlattenHeaderUnion {
    std::map<cstring, std::vector<cstring>> stackMap;

 public:
    DoFlattenHeaderUnionStack(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : DoFlattenHeaderUnion(refMap, typeMap) {
        setName("DoFlattenHeaderUnionStack");
    }
    const IR::Node *postorder(IR::Type_Struct *sf) override;
    const IR::Node *postorder(IR::ArrayIndex *e) override;
    const IR::Node *postorder(IR::Declaration_Variable *dv) override;
    bool hasHeaderUnionStackField(IR::Type_Struct *s);
};

/** This pass handles the validity semantics of header union.
 * 1) On assignment to any element of the header union, it is set to Valid and other elements
 *    of the header union are set to Invalid.
 *   a)  U u;                         | if (my_h1.isValid()) {
 *       H1 my_h1 = { 8w0 };          |     u_h1.setValid();
 *       u.h1 = my_h1;                |     // all other elements set to Invalid
 *                                    |     u_h1 = my_h1;
 *                                    | } else {
 *                                    |     u_h1.setInvalid()
 *                                    | }
 *   b) u.h1 = {16W1}                 | This is already transformed into individual element copy
 *                                    | by earlier passes and individual assignment is translated
 *                                    | as same as (a)
 *   c) u1.h1 = u2.h1                 | same as a) with my_h1 replaced with u2_h1
 *   d) u1 = u2                       | This is already transformed into individual element copy
 *                                    | by earlier passes and individual assignment is translated
 *                                    | same as (c)
 *   e) a = u.isValid()               | tmp = 0;
 *   Only one element should be valid | for elements in union
 *                                    |     if (element.isvalid)
 *                                    |         tmp += 1;
 *                                    | a = tmp == 1
 *
 * 2) When isValid is used as if statement's condition or switch expression, the expression is
 *    replaced with tmp calculated in (1e)
 *   a) switch(u.isValid())           | switch (tmp == 1)
 *   b) if(u.isValid())               | if (tmp == 1)
 * 3) When setValid method is called on a header element, it is translated to setValid for the
 *    given element and setInvalid for rest of the elements
 * 4) Equality/inequality comparisons on unions are unrolled into element-wise comparisons by
 *    earlier passes and hence no-action required here.
 */
class HandleValidityHeaderUnion : public Transform {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    IR::IndexedVector<IR::Declaration> toInsert;  // temporaries

 public:
    HandleValidityHeaderUnion(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        setName("HandleValidityHeaderUnion");
    }
    const IR::Node *postorder(IR::AssignmentStatement *assn) override;
    const IR::Node *postorder(IR::IfStatement *a) override;
    const IR::Node *postorder(IR::SwitchStatement *a) override;
    const IR::Node *postorder(IR::MethodCallStatement *mcs) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::P4Action *action) override;
    const IR::MethodCallStatement *processValidityForStr(const IR::Statement *s,
                                                         const IR::Member *m, cstring headerElement,
                                                         cstring setValid);
    const IR::Node *setInvalidforRest(const IR::Statement *s, const IR::Member *m,
                                      const IR::Type_HeaderUnion *hu, cstring exclude,
                                      bool setValidforCurrMem);
    const IR::Node *expandIsValid(const IR::Statement *a, const IR::MethodCallExpression *mce,
                                  IR::IndexedVector<IR::StatOrDecl> &code_block);
};

class RemoveUnusedHUDeclarations : public Transform {
    P4::ReferenceMap *refMap;

 public:
    explicit RemoveUnusedHUDeclarations(P4::ReferenceMap *refMap) : refMap(refMap) {}
    const IR::Node *preorder(IR::Type_HeaderUnion *type) {
        if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
            return nullptr;
        }
        return type;
    }
};

/** Passmanager to group necessary passes for flattening header unions
 * - RemoveAllUnusedDeclarations pass is used to remove the local standalone header union
 *   variable declarations
 * - The header union flattening pass introduces if statements within parser, RemoveParserIfs
 *   pass is used to convert these if statements to transition select statements
 */
class FlattenHeaderUnion : public PassManager {
 public:
    FlattenHeaderUnion(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, bool loopsUnroll = true) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new HandleValidityHeaderUnion(refMap, typeMap));
        // Stack flattening is only applicable if parser loops are unrolled and
        // header union stack elements are accessed using [] notation. This pass does not handle
        // .next .last etc accessors for stack elements.
        if (loopsUnroll) {
            passes.push_back(new DoFlattenHeaderUnionStack(refMap, typeMap));
            passes.push_back(new P4::ClearTypeMap(typeMap));
            passes.push_back(new P4::ResolveReferences(refMap));
            passes.push_back(new P4::TypeInference(refMap, typeMap, false));
            passes.push_back(new P4::TypeChecking(refMap, typeMap));
            passes.push_back(new P4::RemoveAllUnusedDeclarations(refMap));
        }
        passes.push_back(new DoFlattenHeaderUnion(refMap, typeMap));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new P4::RemoveAllUnusedDeclarations(refMap));
        passes.push_back(new P4::RemoveUnusedHUDeclarations(refMap));
        passes.push_back(new P4::RemoveParserIfs(refMap, typeMap));
    }
};
}  // namespace P4

#endif /* MIDEND_FLATTENUNIONS_H_ */
