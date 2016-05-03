#include "local_copyprop.h"
#include "has_side_effects.h"
#include "expr_uses.h"

class P4::LocalCopyPropagation::ElimDead : public Transform {
    LocalCopyPropagation &self;
    IR::Declaration_Variable *preorder(IR::Declaration_Variable *var) override {
        if (auto local = ::getref(self.locals, var->name)) {
            if (!local->live) {
                LOG3("  removing dead local " << var->name);
                return nullptr; }
        } else {
            BUG("local %s isn't in locals table", var->name); }
        return var; }
    IR::AssignmentStatement *postorder(IR::AssignmentStatement *as) override {
        if (auto dest = as->left->to<IR::PathExpression>()) {
            if (auto local = ::getref(self.locals, dest->path->name)) {
                if (!local->live) {
                    LOG3("  removing dead assignment to " << dest->path->name);
                    return nullptr; } } }
        return as; }

 public:
    explicit ElimDead(LocalCopyPropagation &self) : self(self) {}
};

void P4::LocalCopyPropagation::flow_merge(Visitor &a_) {
    auto &a = dynamic_cast<LocalCopyPropagation &>(a_);
    BUG_CHECK(in_action == a.in_action, "inconsitent LocalCopyPropagation state on merge");
    for (auto &local : locals) {
        if (auto merge = ::getref(a.locals, local.first)) {
            if (merge->val != local.second.val)
                local.second.val = nullptr;
            if (merge->live)
                local.second.live = true;
        } else {
            local.second.val = nullptr; } }
}

void P4::LocalCopyPropagation::dropLocalsUsing(cstring name) {
    for (auto &local : locals) {
        if (local.first == name) {
            LOG4("   dropping " << name << " as it is being assigned to");
            local.second.val = nullptr;
        } else if (local.second.val && exprUses(local.second.val, name)) {
            LOG4("   dropping " << name << " as it is being used");
            local.second.val = nullptr; } }
}

IR::Declaration_Variable *P4::LocalCopyPropagation::postorder(IR::Declaration_Variable *var) {
    if (!in_action) return var;
    if (locals.count(var->name))
        BUG("duplicate var declaration for %s", var->name);
    auto &local = locals[var->name];
    if (var->initializer) {
        if (!hasSideEffects(var->initializer)) {
            LOG3("  saving init value for " << var->name);
            local.val = var->initializer;
        } else {
            local.live = true; } }
    return var;
}

const IR::Expression *P4::LocalCopyPropagation::postorder(IR::PathExpression *path) {
    visitAgain();
    if (auto local = ::getref(locals, path->path->name)) {
        if (isWrite()) {
            return path;
        } else if (local->val) {
            LOG3("  propagating value for " << path->path->name);
            return local->val; 
        } else {
            LOG4("  using " << path->path->name << " with no propagated value");
            local->live = true; } }
    return path;
}

IR::AssignmentStatement *P4::LocalCopyPropagation::postorder(IR::AssignmentStatement *as) {
    if (as->left == as->right) { // FIXME -- need deep equals here?
        LOG3("  removing noop assignment " << *as);
        return nullptr; }
    if (auto dest = as->left->to<IR::PathExpression>()) {
        dropLocalsUsing(dest->path->name);
        if (auto local = ::getref(locals, dest->path->name)) {
            if (!hasSideEffects(as->right)) {
                LOG3("  saving value for " << dest->path->name);
                local->val = as->right; } } }
    return as;
}

IR::MethodCallExpression *P4::LocalCopyPropagation::postorder(IR::MethodCallExpression *mc) {
    if (!in_action) return mc;
    auto type = mc->method->type->to<IR::Type_Method>();
    int idx = 0;
    for (auto param : Values(type->parameters->parameters)) {
        if (param->direction == IR::Direction::Out || param->direction == IR::Direction::InOut) {
            if (auto arg = mc->arguments->at(idx)->to<IR::PathExpression>()) {
                dropLocalsUsing(arg->path->name); } }
        ++idx; }
    return mc;
}

IR::Primitive *P4::LocalCopyPropagation::postorder(IR::Primitive *prim) {
    if (!in_action) return prim;
    for (unsigned idx = 0; idx < prim->operands.size(); ++idx) {
        if (prim->isOutput(idx)) {
            dropLocalsUsing(prim->operands.at(idx)->toString()); } }
    return prim;
}

IR::P4Action *P4::LocalCopyPropagation::preorder(IR::P4Action *act) {
    in_action = true;
    if (!locals.empty()) BUG("corrupt internal data struct");
    LOG2("LocalCopyPropagation working on action " << act->name);
    LOG4(act);
    return act;
}

IR::P4Action *P4::LocalCopyPropagation::postorder(IR::P4Action *act) {
    LOG5("LocalCopyPropagation before ElimDead " << act->name);
    LOG5(act);
    act->body = act->body->apply(ElimDead(*this));
    in_action = false;
    locals.clear();
    LOG3("LocalCopyPropagation finished action " << act->name);
    LOG4(act);
    return act;
}
