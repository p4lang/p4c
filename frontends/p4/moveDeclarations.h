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

#ifndef _FRONTENDS_P4_MOVEDECLARATIONS_H_
#define _FRONTENDS_P4_MOVEDECLARATIONS_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"

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
    /// List of lists of declarations to move, one list per
    /// control/parser/action.
    std::vector<IR::Vector<IR::Declaration>*> toMove;
    void push() { toMove.push_back(new IR::Vector<IR::Declaration>()); }
    void pop() { BUG_CHECK(!toMove.empty(), "Empty move stack"); toMove.pop_back(); }
    IR::Vector<IR::Declaration> *getMoves() const
    { BUG_CHECK(!toMove.empty(), "Empty move stack"); return toMove.back(); }
    void addMove(const IR::Declaration* decl)
    { getMoves()->push_back(decl); }

 public:
    MoveDeclarations() { setName("MoveDeclarations"); visitDagOnce = false; }
    void end_apply(const IR::Node*) override
    { BUG_CHECK(toMove.empty(), "Non empty move stack"); }
    const IR::Node* preorder(IR::P4Action* action) override {
        if (findContext<IR::P4Control>() == nullptr)
            // If we are not in a control, move to the beginning of the action.
            // Otherwise move to the control's beginning.
            push();
        return action; }
    const IR::Node* preorder(IR::P4Control* control) override
    { push(); return control; }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { push(); return parser; }
    const IR::Node* preorder(IR::Function* function) override
    { push(); return function; }
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::Declaration_Constant* decl) override;
};

/** After MoveDeclarations, some variable declarations in the "local"
 * section of a parser and control may still have initializers; these are moved
 * into the start state, and to the beginning of the apply body repectively.
 *
 * @pre Must be run after MoveDeclarations.
 */
class MoveInitializers : public Transform {
    IR::IndexedVector<IR::StatOrDecl> *toMove;  // This contains just IR::AssignmentStatement

 public:
    MoveInitializers() {
        setName("MoveInitializers");
        toMove = new IR::IndexedVector<IR::StatOrDecl>(); }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::ParserState* state) override;
    const IR::Node* postorder(IR::P4Control* control) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_MOVEDECLARATIONS_H_ */
