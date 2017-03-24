/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _P4_UNUSEDDECLARATIONS_H_
#define _P4_UNUSEDDECLARATIONS_H_

#include "ir/ir.h"
#include "../common/resolveReferences/resolveReferences.h"

namespace P4 {

class RemoveUnusedDeclarations : public Transform {
    const ReferenceMap* refMap;
    const IR::Node* process(const IR::IDeclaration* decl);
    bool warn;

 public:
    RemoveUnusedDeclarations(const ReferenceMap* refMap, bool warn) : refMap(refMap), warn(warn)
    { setName("RemoveUnusedDeclarations"); }

    using Transform::postorder;
    using Transform::preorder;
    using Transform::init_apply;

    Visitor::profile_t init_apply(const IR::Node *root) override;

    const IR::Node* preorder(IR::P4Control* cont) override;
    const IR::Node* preorder(IR::P4Parser* cont) override;
    const IR::Node* preorder(IR::P4Table* cont) override;
    const IR::Node* preorder(IR::ParserState* state)  override;
    const IR::Node* preorder(IR::Type_Enum* type)  override;

    const IR::Node* preorder(IR::Declaration_Instance* decl) override;

    // Do not delete the following even if unused
    const IR::Node* preorder(IR::Type_Error* type) override
    { prune(); return type; }
    const IR::Node* preorder(IR::Declaration_MatchKind* decl) override
    { prune(); return decl; }
    const IR::Node* preorder(IR::Type_StructLike* type) override
    { prune(); return type; }
    const IR::Node* preorder(IR::Type_Extern* type) override
    { prune(); return type; }
    const IR::Node* preorder(IR::Type_Method* type) override
    { prune(); return type; }

    const IR::Node* preorder(IR::Declaration_Variable* decl)  override;
    const IR::Node* preorder(IR::Parameter* param) override { return param; }  // never dead
    const IR::Node* preorder(IR::Declaration* decl) override { return process(decl); }
    const IR::Node* preorder(IR::Type_Declaration* decl) override { return process(decl); }
};

// Iterates RemoveUnusedDeclarations until convergence.
class RemoveAllUnusedDeclarations : public PassManager {
 public:
    explicit RemoveAllUnusedDeclarations(ReferenceMap* refMap, bool warn = false) {
        CHECK_NULL(refMap);
        passes.emplace_back(
            new PassRepeated {
                new ResolveReferences(refMap),
                new RemoveUnusedDeclarations(refMap, warn)
             });
        setName("RemoveAllUnusedDeclarations");
        setStopOnError(true);
    }
};

}  // namespace P4

#endif /* _P4_UNUSEDDECLARATIONS_H_ */
