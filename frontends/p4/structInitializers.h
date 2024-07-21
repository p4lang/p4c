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

#ifndef FRONTENDS_P4_STRUCTINITIALIZERS_H_
#define FRONTENDS_P4_STRUCTINITIALIZERS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace p4c::P4 {

/// Converts some list expressions into struct initializers.
class CreateStructInitializers : public Transform, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit CreateStructInitializers(TypeMap *typeMap) : typeMap(typeMap) {
        setName("CreateStructInitializers");
        CHECK_NULL(typeMap);
    }

    const IR::Node *postorder(IR::AssignmentStatement *statement) override;
    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
    const IR::Node *postorder(IR::Operation_Relation *expression) override;
    const IR::Node *postorder(IR::Declaration_Variable *statement) override;
    const IR::Node *postorder(IR::ReturnStatement *statement) override;
};

class StructInitializers : public PassManager {
 public:
    explicit StructInitializers(TypeMap *typeMap) {
        setName("StructInitializers");
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new CreateStructInitializers(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
    }
};

}  // namespace p4c::P4

#endif /* FRONTENDS_P4_STRUCTINITIALIZERS_H_ */
