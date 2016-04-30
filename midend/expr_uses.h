#ifndef MIDEND_EXPR_USES_H_
#define MIDEND_EXPR_USES_H_

#include "ir/ir.h"

/* Should this be a method on IR::Expression? */

class exprUses : public Inspector {
    cstring look_for;
    bool result = false;
    bool preorder(const IR::Path *p) override { 
        if (p->name == look_for) result = true;
        return !result; }
    bool preorder(const IR::Primitive *p) override { 
        if (p->name == look_for) result = true;
        return !result; }
    bool preorder(const IR::NamedRef *n) override { 
        if (n->name == look_for) result = true;
        return !result; }
    bool preorder(const IR::Expression *) override { return !result; }
 public:
    exprUses(const IR::Expression *e, cstring n) : look_for(n) { e->apply(*this); }
    explicit operator bool () { return result; }
};


#endif /* MIDEND_EXPR_USES_H_ */
