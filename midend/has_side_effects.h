#ifndef MIDEND_HAS_SIDE_EFFECTS_H_
#define MIDEND_HAS_SIDE_EFFECTS_H_

#include "ir/ir.h"

/* Should this be a method on IR::Expression? */

class hasSideEffects : public Inspector {
    bool result = false;
    bool preorder(const IR::AssignmentStatement *) override { return !(result = true); }
    /* FIXME -- currently assuming all calls and primitves have side effects */
    bool preorder(const IR::MethodCallExpression *) override { return !(result = true); }
    bool preorder(const IR::Primitive *) override { return !(result = true); }
    bool preorder(const IR::Expression *) override { return !result; }
 public:
    explicit hasSideEffects(const IR::Expression *e) { e->apply(*this); }
    explicit operator bool () { return result; }
};


#endif /* MIDEND_HAS_SIDE_EFFECTS_H_ */
