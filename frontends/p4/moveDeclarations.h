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

#ifndef FRONTENDS_P4_MOVEDECLARATIONS_H_
#define FRONTENDS_P4_MOVEDECLARATIONS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"

namespace P4 {

/** Moves all local declarations in a control or parser to the "top", including
 * the ones in the control body and parser states.  Also, if the control has
 * nested actions, move the declarations from the actions to the enclosing
 * control.
 *
 * @pre All declarations must have different names---eg. must be done after the
 * UniqueNames pass.
 */
class MoveDeclarations : public Transform {
    bool parsersOnly;

    /// List of lists of declarations to move, one list per
    /// control/parser/action.
    std::vector<IR::Vector<IR::Declaration> *> toMove;
    void push() { toMove.push_back(new IR::Vector<IR::Declaration>()); }
    void pop() {
        BUG_CHECK(!toMove.empty(), "Empty move stack");
        toMove.pop_back();
    }
    IR::Vector<IR::Declaration> *getMoves() const {
        BUG_CHECK(!toMove.empty(), "Empty move stack");
        return toMove.back();
    }
    void addMove(const IR::Declaration *decl) { getMoves()->push_back(decl); }

 public:
    explicit MoveDeclarations(bool parsersOnly = false) : parsersOnly(parsersOnly) {
        setName("MoveDeclarations");
        visitDagOnce = false;
    }
    void end_apply(const IR::Node *) override { BUG_CHECK(toMove.empty(), "Non empty move stack"); }
    const IR::Node *preorder(IR::P4Action *action) override {
        if (parsersOnly) {
            prune();
            return action;
        }
        if (findContext<IR::P4Control>() == nullptr)
            // If we are not in a control, move to the beginning of the action.
            // Otherwise move to the control's beginning.
            push();
        return action;
    }
    const IR::Node *preorder(IR::P4Control *control) override {
        if (parsersOnly) {
            prune();
            return control;
        }
        push();
        return control;
    }
    const IR::Node *preorder(IR::P4Parser *parser) override {
        push();
        return parser;
    }
    const IR::Node *preorder(IR::Function *function) override {
        if (parsersOnly) {
            prune();
            return function;
        }
        push();
        return function;
    }
    const IR::Node *postorder(IR::P4Action *action) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::Function *function) override;
    const IR::Node *postorder(IR::Declaration_Variable *decl) override;
    const IR::Node *postorder(IR::Declaration_Constant *decl) override;
};

/** After MoveDeclarations, some variable declarations in the "local"
 * section of a parser and control may still have initializers; these are moved
 * into a new start state, and to the beginning of the apply body repectively.
 *
 * @pre Must be run after MoveDeclarations.
 */
class MoveInitializers : public Transform {
    ReferenceMap *refMap;
    IR::IndexedVector<IR::StatOrDecl> *toMove;  // This contains just IR::AssignmentStatement
    const IR::ParserState *oldStart;  // nullptr if we do not want to rename the start state
    cstring newStartName;             // name allocated to the old start state

 public:
    explicit MoveInitializers(ReferenceMap *refMap)
        : refMap(refMap), oldStart(nullptr), newStartName("") {
        setName("MoveInitializers");
        CHECK_NULL(refMap);
        toMove = new IR::IndexedVector<IR::StatOrDecl>();
    }
    const IR::Node *preorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::Declaration_Variable *decl) override;
    const IR::Node *postorder(IR::ParserState *state) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::Path *path) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_MOVEDECLARATIONS_H_ */
