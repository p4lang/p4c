#ifndef _MIDEND_MOVEDECLARATIONS_H_
#define _MIDEND_MOVEDECLARATIONS_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// A local uninitialized variable in a parser state that represents
// a header must be invalid every time the parser state is entered.
class ResetHeaders : public Transform {
    TypeMap* typeMap;

    void generateResets(const IR::Type* type, const IR::Expression* expr,
                        IR::Vector<IR::StatOrDecl>* resets);
 public:
    explicit ResetHeaders(TypeMap* typeMap) : typeMap(typeMap) { CHECK_NULL(typeMap); }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
};

// Moves all local declarations to the "top".
// This can only be done safely if all declarations have different names,
// so it has to be done after the UniqueNames pass.
// We move all declarations in a control or parser to the "top", including
// the ones in the Control body and Parser states.
// Also, if the control has nested actions, we move the declarations from
// the actions to the enclosing control.
class MoveDeclarations : public Transform {
    std::vector<IR::Vector<IR::Declaration>*> toMove;
    void push() { toMove.push_back(new IR::Vector<IR::Declaration>()); }
    void pop() { BUG_CHECK(!toMove.empty(), "Empty move stack"); toMove.pop_back(); }
    IR::Vector<IR::Declaration> *getMoves() const
    { BUG_CHECK(!toMove.empty(), "Empty move stack"); return toMove.back(); }
    void addMove(const IR::Declaration* decl)
    { getMoves()->push_back(decl); }

 public:
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
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::Declaration_Constant* decl) override;
};

}  // namespace P4

#endif /* _MIDEND_MOVEDECLARATIONS_H_ */
