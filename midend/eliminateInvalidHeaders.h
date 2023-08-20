/*
Copyright 2022 VMware, Inc.

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

#ifndef MIDEND_ELIMINATEINVALIDHEADERS_H_
#define MIDEND_ELIMINATEINVALIDHEADERS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Replaces invalid header expressions by variables
 * which are explicitly initialized to be invalid.
 * A similar operation is performed for invalid header unions.
 * This cannot be done for constant declarations, though.
 */
class DoEliminateInvalidHeaders final : public Transform {
    ReferenceMap *refMap;
    IR::IndexedVector<IR::StatOrDecl> statements;
    std::vector<const IR::Declaration_Variable *> variables;

 public:
    DoEliminateInvalidHeaders(ReferenceMap *refMap) : refMap(refMap) {
        setName("DoEliminateInvalidHeaders");
        CHECK_NULL(refMap);
    }
    const IR::Node *postorder(IR::InvalidHeader *expression) override;
    const IR::Node *postorder(IR::InvalidHeaderUnion *expression) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::ParserState *parser) override;
    const IR::Node *postorder(IR::P4Action *action) override;
};

class EliminateInvalidHeaders final : public PassManager {
 public:
    EliminateInvalidHeaders(ReferenceMap *refMap, TypeMap *typeMap,
                            TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoEliminateInvalidHeaders(refMap));
        setName("EliminateInvalidHeaders");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATEINVALIDHEADERS_H_ */
