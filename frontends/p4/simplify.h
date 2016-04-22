#ifndef _FRONTENDS_P4_SIMPLIFY_H_
#define _FRONTENDS_P4_SIMPLIFY_H_

#include "ir/ir.h"

namespace P4 {

// Determines whether an expression may have side-effects
class SideEffects : public Inspector {
 private:
    bool hasSideEffects;
    void postorder(const IR::MethodCallExpression*) override
    { hasSideEffects = true; }
    SideEffects() : hasSideEffects(false) {}
 public:
    // Returns true if the expression may have side-effects.
    static bool check(const IR::Expression* expression) {
        SideEffects se;
        expression->apply(se);
        return se.hasSideEffects;
    }
};

class SimplifyControlFlow : public Transform {
 public:
    const IR::Node* postorder(IR::BlockStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
    const IR::Node* postorder(IR::EmptyStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFY_H_ */
