#ifndef _inline_control_flow_h_
#define _inline_control_flow_h_

#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

class InlineControlFlow : public Transform {
public:
    InlineControlFlow(const IR::V1Program *gl) : global(gl) {}
private:
    const IR::V1Program *global;

    const IR::Node *preorder(IR::Apply *a) override {
        if (global && !global->get<IR::V1Table>(a->name))
            error("%s: No table named %s", a->srcInfo, a->name);
        return a;
    }
    const IR::Node *preorder(IR::Primitive *p) override {
        if (auto cf = global ? global->get<IR::V1Control>(p->name) : 0) {
            const IR::V1Control *control;
            if (auto act = findContext<IR::ActionFunction>())
                error("%s: Trying to call control flow %s in action %s", p->srcInfo,
                      p->name, act->name);
            else if (auto table = findContext<IR::V1Table>())
                error("%s: Trying to call control flow %s in table %s", p->srcInfo,
                      p->name, table->name);
            else if ((control = findContext<IR::V1Control>()) && control->name == p->name)
                error("%s: Recursive call to control flow %s", p->srcInfo, p->name);
            else
                return cf->code; }
        return p;
    }
};

#endif /* _inline_control_flow_h_ */
