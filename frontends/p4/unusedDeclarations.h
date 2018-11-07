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

/** @brief Removes unused declarations.
 *
 * The following kinds of nodes are not removed even if they are unreferenced:
 *  - IR::Declaration_MatchKind
 *  - IR::Parameter
 *  - IR::Type_Error
 *  - IR::Type_Extern
 *  - IR::Type_Method
 *  - IR::Type_StructLike
 *
 * Additionally, IR::Declaration_Instance nodes for extern instances are not
 * removed but still trigger warnings.
 *
 * If @warned is non-null, unused IR::P4Table and IR::Declaration_Instance
 * nodes are stored in @warned if they are unused and removed by this pass.  A
 * compilation warning is emitted when a new node is added to @warned,
 * preventing duplicate warnings per node.
 *
 * @pre Requires an up-to-date ReferenceMap.
 */
class RemoveUnusedDeclarations : public Transform {
    const ReferenceMap* refMap;

    /** If not null, logs the following unused elements in @warn:
     *  - unused IR::P4Table nodes
     *  - unused IR::Declaration_Instance nodes
     */
    std::set<const IR::Node*>* warned;

    /** Stores @node in @warned if:
     *   - @warned is non-null,
     *   - @node is an unused declaration,
     *   - @node is not already present in @warned.
     *
     * @return true if @node is added to @warned.
     */
    bool giveWarning(const IR::Node* node);
    const IR::Node* process(const IR::IDeclaration* decl);

 public:
    explicit RemoveUnusedDeclarations(const ReferenceMap* refMap,
                                      std::set<const IR::Node*>* warned = nullptr) :
            refMap(refMap), warned(warned)
    { CHECK_NULL(refMap); setName("RemoveUnusedDeclarations"); }

    using Transform::postorder;
    using Transform::preorder;
    using Transform::init_apply;

    Visitor::profile_t init_apply(const IR::Node *root) override;

    const IR::Node* preorder(IR::P4Control* cont) override;
    const IR::Node* preorder(IR::P4Parser* cont) override;
    const IR::Node* preorder(IR::P4Table* cont) override;
    const IR::Node* preorder(IR::ParserState* state)  override;
    const IR::Node* preorder(IR::Type_Enum* type)  override;
    const IR::Node* preorder(IR::Type_SerEnum* type)  override;

    const IR::Node* preorder(IR::Declaration_Instance* decl) override;

    // The following kinds of nodes are not deleted even if they are unreferenced
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
    const IR::Node* preorder(IR::Parameter* param) override { return param; }  // never dead
    const IR::Node* preorder(IR::NamedExpression* ne) override { return ne; }  // idem

    const IR::Node* preorder(IR::Declaration_Variable* decl)  override;
    const IR::Node* preorder(IR::Declaration* decl) override { return process(decl); }
    const IR::Node* preorder(IR::Type_Declaration* decl) override { return process(decl); }
};

/** @brief Iterates RemoveUnusedDeclarations until convergence.
 *
 * If @warn is true, emit compiler warnings if an unused instance of an
 * IR::P4Table or IR::Declaration_Instance is removed.
 */
class RemoveAllUnusedDeclarations : public PassManager {
 public:
    explicit RemoveAllUnusedDeclarations(ReferenceMap* refMap, bool warn = false) {
        CHECK_NULL(refMap);

        // Unused extern instances are not removed but may still trigger
        // warnings.  The @warned set keeps track of warnings emitted in
        // previous iterations to avoid emitting duplicate warnings.
        std::set<const IR::Node*> *warned = nullptr;
        if (warn)
            warned = new std::set<const IR::Node*>();

        passes.emplace_back(
            new PassRepeated {
                new ResolveReferences(refMap),
                new RemoveUnusedDeclarations(refMap, warned)
             });
        setName("RemoveAllUnusedDeclarations");
        setStopOnError(true);
    }
};

}  // namespace P4

#endif /* _P4_UNUSEDDECLARATIONS_H_ */
