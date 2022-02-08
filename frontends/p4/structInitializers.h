/*
Copyright 2018 VMware, Inc.

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

#ifndef _FRONTENDS_P4_STRUCTINITIALIZERS_H_
#define _FRONTENDS_P4_STRUCTINITIALIZERS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

/// Converts some list expressions into struct initializers.
class CreateStructInitializers : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    IR::IndexedVector<IR::StatOrDecl> *toMove;
 public:
    CreateStructInitializers(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap) {
        setName("CreateStructInitializers");
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
    }

    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::Operation_Relation* expression) override;
    const IR::Node* postorder(IR::Declaration_Variable* statement) override;
    const IR::Node* postorder(IR::ReturnStatement* statement) override;
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;

    static const IR::Expression* defaultValue(const IR::Type* type, const Util::SourceInfo srcInfo);
};
/// Creates temporary variable for every default initialization that
/// shows up in an inconvenient place
/// Inconvenient places are operands in operation relation, return statements,
/// arguments of method call expressions
class MoveDefaultInitialization : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    IR::IndexedVector<IR::StatOrDecl> *toMove;
 public:
    MoveDefaultInitialization(ReferenceMap* refMap, TypeMap* typeMap) :
        refMap(refMap), typeMap(typeMap) {
        setName("MoveDefaultInitialization");
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        toMove = new IR::IndexedVector<IR::StatOrDecl>();
    }
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::Operation_Relation* expression) override;
    const IR::Node* postorder(IR::ReturnStatement* statement) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::ParserState* state) override;
};
class StructInitializers : public PassManager {
 public:
    StructInitializers(ReferenceMap* refMap, TypeMap* typeMap) {
        setName("StructInitializers");
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new MoveDefaultInitialization(refMap, typeMap));
        /// TypeChecking is needed for the newly created variables
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new CreateStructInitializers(refMap, typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_STRUCTINITIALIZERS_H_ */
