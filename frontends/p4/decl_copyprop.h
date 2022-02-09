/* Copyright 2022 Syrmia Networks

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

#ifndef _P4_DECL_COPYPROP_H_
#define _P4_DECL_COPYPROP_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

/// DeclarationCopyPropagation pass operates on control locals block and
/// propagates variables only if they are initialized with constant values.
/// For example:
///     ...
///  control c() {
///     bit<32> a = 10;
///     S s = { x = 3, y = 4, z = 5 };
///     bit<32> b = a;
///     bit<32> c = s.y;
///     apply {
///         /* not processed */
///     }
///  }
/// <----- DeclarationCopyPropagation ----->
///     ...
///  control c() {
///     bit<32> a = 10;
///     S s = { x = 3, y = 4, z = 5 };
///     bit<32> b = 10;
///     bit<32> c = 4;
///     apply {
///         /* not processed */
///     }
///  }

class DoDeclarationCopyPropagation : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;

    /// Map for storing constant values for variables. The mapped value
    /// is a pointer to an Expression that represents the constant value
    /// for the variable with the name given as a map key.
    std::map<cstring, const IR::Expression*> varMap;

    /// Controlls where should the propagation work.
    bool working = false;

    /// If set to true limits the pass to only check if the used
    /// variables are initialized. It assumes that all of them are
    /// compile time known for the time being.
    /// This is used to postpone constants check in TypeChecker.
    bool checkOnly;

    void setCompileTimeConstant(const IR::Expression* expr) {
        typeMap->setCompileTimeConstant(expr);
    }
    cstring getName(const IR::Expression* expr);
    bool checkConst(const IR::Expression* expr);
    const IR::Expression* getValue(const IR::Expression* expr);
    const IR::Expression* findValue(const IR::Expression* expr);

 public:
    DoDeclarationCopyPropagation(ReferenceMap* refMap, TypeMap* typeMap,
                                 bool checkOnly = false)
        : refMap(refMap), typeMap(typeMap), checkOnly(checkOnly) {}
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::P4Table* table) override;
    const IR::Node* preorder(IR::MethodCallExpression* method) override;
    const IR::Node* preorder(IR::Function* func) override;
    const IR::Node* preorder(IR::BlockStatement* block) override;
    const IR::Node* preorder(IR::Member* member) override;
    const IR::Node* preorder(IR::ArrayIndex* array) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::PathExpression* path) override;
};

class DeclarationCopyPropagation : public PassManager {
 public:
    DeclarationCopyPropagation(ReferenceMap* refMap, TypeMap* typeMap,
                               bool checkOnly = false) {
        addPasses({
            new P4::DoDeclarationCopyPropagation(refMap, typeMap, checkOnly),
            new P4::ResolveReferences(refMap)});
        setName("DeclarationCopyPropagation");
    }
};

} /* namespace p4 */

#endif /* _P4_DECL_COPYPROP_H_ */
