#include "ir.h"
#include "lib/log.h"
#include "frontends/common/typeMap.h"

bool P4WriteContext::isWrite() {
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node)
        return false;
    if (auto *prim = ctxt->node->to<IR::Primitive>())
        return prim->isOutput(ctxt->child_index);
    if (ctxt->node->is<IR::AssignmentStatement>())
        return ctxt->child_index == 0;
    if (ctxt->node->is<IR::Vector<IR::Expression>>()) {
        if (!ctxt->parent || !ctxt->parent->node)
            return false;
        if (auto *mc = ctxt->parent->node->to<IR::MethodCallExpression>()) {
            auto type = mc->method->type->to<IR::Type_Method>();
            if (!type) {
                /* FIXME -- can't find the type of the method -- should be a BUG? */
                return true; }
            auto param = type->parameters->getParameter(ctxt->child_index);
            return param->direction == IR::Direction::Out ||
                   param->direction == IR::Direction::InOut; }
        if (ctxt->parent->node->is<IR::ConstructorCallExpression>()) {
            /* FIXME -- no constructor types?  assume all arguments are inout? */
            return true; } }
    return false;
}
