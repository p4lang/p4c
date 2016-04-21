#ifndef _MIDEND_MOVEDECLARATIONS_H_
#define _MIDEND_MOVEDECLARATIONS_H_

#include "ir/ir.h"

namespace P4 {

// Moves all local declarations to the "top".
// This can only be done safely if all declarations have different names,
// so it has to be done after the UniqueNames pass.
class MoveDeclarations : public Transform {
    std::vector<IR::Vector<IR::Declaration>*> toMove;
    void push() { toMove.push_back(new IR::Vector<IR::Declaration>()); }
    void pop() { toMove.pop_back(); }
    IR::Vector<IR::Declaration> *getMoves() const
    { return toMove.back(); }
    void addMove(const IR::Declaration* decl)
    { getMoves()->push_back(decl); }
 public:
    const IR::Node* preorder(IR::P4Action* action) override
    { push(); return action; }
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
