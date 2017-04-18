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

#include "local_copyprop.h"
#include "has_side_effects.h"
#include "expr_uses.h"

namespace P4 {

/* helper function to get the 'outermost' containing expression in an lvalue */
static const IR::Expression *lvalue_out(const IR::Expression *exp) {
    if (auto ai = exp->to<IR::ArrayIndex>())
        return lvalue_out(ai->left);
    if (auto hsr = exp->to<IR::HeaderStackItemRef>())
        return lvalue_out(hsr->base());
    if (auto sl = exp->to<IR::Slice>())
        return lvalue_out(sl->e0);
    if (auto mem = exp->to<IR::Member>())
        return lvalue_out(mem->expr);
    return exp;
}

/* LocalCopyPropagation does copy propagation and dead code elimination within a 'block'
 * the body of an action or control (TODO -- extend to parsers/states).  Within the
 * block it tracks all variables defined in the block as well as those defined outside the
 * block that are used in the block, though only those local to the block can be eliminated
 */

class DoLocalCopyPropagation::ElimDead : public Transform {
    /* Traversal to eliminate dead locals in a block after copyprop
     * we run this over just the block (body and declarations) after copyprop
     * of the block, so it only removes those vars declared in the block */
    DoLocalCopyPropagation &self;
    const IR::Node *preorder(IR::Declaration_Variable *var) override {
        if (auto local = ::getref(self.available, var->name)) {
            if (local->local && !local->live) {
                LOG3("  removing dead local " << var->name);
                return nullptr; } }
        return var; }
    IR::AssignmentStatement *postorder(IR::AssignmentStatement *as) override {
        if (auto dest = lvalue_out(as->left)->to<IR::PathExpression>()) {
            if (auto var = ::getref(self.available, dest->path->name)) {
                if (var->local && !var->live) {
                    LOG3("  removing dead assignment to " << dest->path->name);
                    return nullptr;
                } else if (var->local) {
                    LOG6("  not removing live assignment to " << dest->path->name);
                } else {
                    LOG6("  not removing assignment to non-local " << dest->path->name); } } }
        return as; }
    IR::IfStatement *postorder(IR::IfStatement *s) override {
        if (s->ifTrue == nullptr) {
            /* can't leave ifTrue == nullptr, as that will fail validation -- fold away
             * the if statement as needed */
            if (s->ifFalse == nullptr)
                return nullptr;
            s->ifTrue = s->ifFalse;
            s->ifFalse = nullptr;
            s->condition = new IR::LNot(s->condition); }
        return s; }

 public:
    explicit ElimDead(DoLocalCopyPropagation &self) : self(self) {}
};

class DoLocalCopyPropagation::RewriteTableKeys : public Transform {
    /* When doing copyprop on a control, if we have values available at a table.apply
     * call and thise values are used in the key of the table, we want to propagate them
     * into the table (replacing the write keys).  We can't do that while we're visiting
     * the control apply body, so we record the need for it in the TableInfo, and then
     * after finishing the control apply body, we call this pass to re-transform the
     * local tables in the control based on what is in their TableInfo. */
    DoLocalCopyPropagation &self;
    TableInfo           *table = nullptr;
    IR::P4Table *preorder(IR::P4Table *tbl) {
        BUG_CHECK(table == nullptr, "corrupt internal state");
        table = &self.tables[tbl->name];
        LOG3("RewriteTableKeys for table " << tbl->name);
        return tbl; }
    IR::P4Table *postorder(IR::P4Table *tbl) {
        BUG_CHECK(table == &self.tables[tbl->name], "corrupt internal state");
        table = nullptr;
        return tbl; }
    const IR::Expression *preorder(IR::PathExpression *path) {
        if (table) {
            const Visitor::Context *ctxt = nullptr;
            if (findContext<IR::KeyElement>(ctxt) && ctxt->child_index == 1) {
                if (table->key_remap.count(path->path->name)) {
                    LOG4("  rewriting key " << path->path->name << " : " <<
                         table->key_remap.at(path->path->name));
                    return table->key_remap.at(path->path->name); } } }
        return path; }

 public:
    explicit RewriteTableKeys(DoLocalCopyPropagation &self) : self(self) {}
};

void DoLocalCopyPropagation::flow_merge(Visitor &a_) {
    auto &a = dynamic_cast<DoLocalCopyPropagation &>(a_);
    BUG_CHECK(working == a.working, "inconsitent DoLocalCopyPropagation state on merge");
    for (auto &var : available) {
        if (auto merge = ::getref(a.available, var.first)) {
            if (merge->val != var.second.val)
                var.second.val = nullptr;
            if (merge->live)
                var.second.live = true;
        } else {
            var.second.val = nullptr; } }
    need_key_rewrite |= a.need_key_rewrite;
}

void DoLocalCopyPropagation::dropValuesUsing(cstring name) {
    LOG6("dropValuesUsing(" << name << ")");
    for (auto &var : available) {
        LOG7("  checking " << var.first << " = " << var.second.val);
        if (var.first == name) {
            LOG4("   dropping " << (var.second.val ? "" : "(nop) ") << name <<
                 " as it is being assigned to");
            var.second.val = nullptr;
        } else if (var.second.val && exprUses(var.second.val, name)) {
            LOG4("   dropping " << (var.second.val ? "" : "(nop) ") << var.first <<
                 " as it uses " << name);
            var.second.val = nullptr; } }
}

void DoLocalCopyPropagation::visit_local_decl(const IR::Declaration_Variable *var) {
    LOG4("Visiting " << var);
    if (available.count(var->name))
        BUG("duplicate var declaration for %s", var->name);
    auto &local = available[var->name];
    local.local = true;
    if (var->initializer) {
        if (!hasSideEffects(var->initializer)) {
            LOG3("  saving init value for " << var->name << ": " << var->initializer);
            local.val = var->initializer;
        } else {
            local.live = true; } }
}

const IR::Node *DoLocalCopyPropagation::postorder(IR::Declaration_Variable *var) {
    if (!working) return var;
    visit_local_decl(var);
    return var;
}

const IR::Expression *DoLocalCopyPropagation::postorder(IR::PathExpression *path) {
    if (inferForTable) {
        const Visitor::Context *ctxt = nullptr;
        if (findContext<IR::KeyElement>(ctxt) && ctxt->child_index == 1)
            inferForTable->keyreads.insert(path->path->name); }
    if (!working) return path;
    if (isWrite()) {
        dropValuesUsing(path->path->name);
        if (inferForFunc)
            inferForFunc->writes.insert(path->path->name);
        if (isRead() || findContext<IR::MethodCallExpression>()) {
            /* If this is being used as an 'out' param of a method call, its not really
             * read, but we can't dead-code eliminate it without eliminating the entire
             * call, so we mark it as live.  Unfortunate as we then won't dead-code
             * remove other assignmnents. */
            if (auto var = ::getref(available, path->path->name)) {
                LOG4("  using " << path->path->name << " in read-write");
                var->live = true; }
            if (inferForFunc)
                inferForFunc->reads.insert(path->path->name); }
        return path; }
    if (auto var = ::getref(available, path->path->name)) {
        if (var->val) {
            LOG3("  propagating value for " << path->path->name << ": " << var->val);
            return var->val;
        } else {
            LOG4("  using " << path->path->name << " with no propagated value");
            var->live = true; } }
    if (inferForFunc)
        inferForFunc->reads.insert(path->path->name);
    return path;
}

IR::AssignmentStatement *DoLocalCopyPropagation::preorder(IR::AssignmentStatement *as) {
    if (!working) return as;
    // visit the source subtree first, before the destination subtree
    // make sure child indexes are set properly so we can detect writes -- these are the
    // extra arguments to 'visit' in order to make introspection vis the Visitor::Context
    // work.  Normally this is all managed by the auto-generated visit_children methods,
    // but since we're overriding that here AND P4WriteContext cares about it (that's how
    // it determines whether something is a write or a read), it needs to be correct
    // This is error-prone and fragile
    visit(as->right, "right", 1);
    visit(as->left, "left", 0);
    prune();
    return postorder(as);
}

IR::AssignmentStatement *DoLocalCopyPropagation::postorder(IR::AssignmentStatement *as) {
    if (as->left == as->right) {   // FIXME -- need deep equals here?
        LOG3("  removing noop assignment " << *as);
        return nullptr; }
    if (!working) return as;
    if (auto dest = as->left->to<IR::PathExpression>()) {
        if (!hasSideEffects(as->right)) {
            if (as->right->is<IR::ListExpression>()) {
                /* FIXME -- List Expressions need to be turned into constructor calls before
                 * we can copyprop them */
                return as; }
            if (exprUses(as->right, dest->path->name)) {
                /* can't propagate the value as it is defined in terms of itself.
                 * FIXME -- we could propagate if we introduced a new temp, but that
                 * may make things worse rather than better */
                return as; }
            LOG3("  saving value for " << dest->path->name << ": " << as->right);
            available[dest->path->name].val = as->right; }
    } else {
        LOG3("dest of assignment is " << as->left << " so skipping");
    }
    return as;
}

IR::MethodCallExpression *DoLocalCopyPropagation::postorder(IR::MethodCallExpression *mc) {
    if (!working) return mc;
    if (auto mem = mc->method->to<IR::Member>()) {
        if (auto obj = mem->expr->to<IR::PathExpression>()) {
            if (tables.count(obj->path->name)) {
                LOG3("table apply method call " << mc->method);
                apply_table(&tables[obj->path->name]);
                return mc;
            } else if (obj->type->is<IR::Type_Extern>()) {
                auto name = obj->path->name + '.' + mem->member;
                if (methods.count(name)) {
                    LOG3("concrete method call " << name);
                    apply_function(&methods[name]);
                    return mc;
                } else {
                    LOG3("extern method call " << mc->method << " does nothing");
                    // FIXME -- is this safe? If an abstract method in outer scope that
                    // we haven't seen a definition for, it might affects non-locals?
                    return mc; } } } }
    if (auto fn = mc->method->to<IR::PathExpression>()) {
        if (actions.count(fn->path->name)) {
            LOG3("action method call " << mc->method);
            apply_function(&actions[fn->path->name]);
            return mc; } }
    LOG3("unknown method call " << mc->method << " clears all nonlocal saved values");
    for (auto &var : Values(available)) {
        if (!var.local) {
            var.val = nullptr;
            var.live = true; } }
    return mc;
}

IR::P4Action *DoLocalCopyPropagation::preorder(IR::P4Action *act) {
    BUG_CHECK(!working && !inferForFunc && available.empty(), "corrupt internal data struct");
    working = true;
    inferForFunc = &actions[act->name];
    LOG2("DoLocalCopyPropagation working on action " << act->name);
    LOG4(act);
    return act;
}

IR::P4Action *DoLocalCopyPropagation::postorder(IR::P4Action *act) {
    LOG5("DoLocalCopyPropagation before ElimDead " << act->name);
    LOG5(act);
    BUG_CHECK(inferForFunc == &actions[act->name], "corrupt internal data struct");
    act->body = act->body->apply(ElimDead(*this))->to<IR::BlockStatement>();
    working = false;
    available.clear();
    LOG3("DoLocalCopyPropagation finished action " << act->name);
    LOG4("reads=" << inferForFunc->reads << " writes=" << inferForFunc->writes);
    LOG4(act);
    inferForFunc = nullptr;
    return act;
}

IR::Function *DoLocalCopyPropagation::preorder(IR::Function *fn) {
    BUG_CHECK(!working && !inferForFunc && available.empty(), "corrupt internal data struct");
    auto name = findContext<IR::Declaration_Instance>()->name + '.' + fn->name;
    working = true;
    inferForFunc = &methods[name];
    LOG2("DoLocalCopyPropagation working on function " << name);
    LOG4(fn);
    return fn;
}

IR::Function *DoLocalCopyPropagation::postorder(IR::Function *fn) {
    auto name = findContext<IR::Declaration_Instance>()->name + '.' + fn->name;
    LOG5("DoLocalCopyPropagation before ElimDead " << name);
    LOG5(fn);
    BUG_CHECK(inferForFunc == &methods[name], "corrupt internal data struct");
    fn->body = fn->body->apply(ElimDead(*this))->to<IR::BlockStatement>();
    working = false;
    available.clear();
    LOG3("DoLocalCopyPropagation finished function " << name);
    LOG4("reads=" << inferForFunc->reads << " writes=" << inferForFunc->writes);
    LOG4(fn);
    inferForFunc = nullptr;
    return fn;
}

IR::P4Control *DoLocalCopyPropagation::preorder(IR::P4Control *ctrl) {
    BUG_CHECK(!working && available.empty(), "corrupt internal data struct");
    visit(ctrl->type, "type");
    visit(ctrl->constructorParams, "constructorParams");
    ctrl->controlLocals.visit_children(*this);
    if (working || !available.empty()) BUG("corrupt internal data struct");
    working = true;
    LOG2("DoLocalCopyPropagation working on control " << ctrl->name);
    LOG4(ctrl);
    need_key_rewrite = false;
    for (auto local : ctrl->controlLocals)
        if (auto var = local->to<IR::Declaration_Variable>())
            visit_local_decl(var);
    visit(ctrl->body, "body");
    if (need_key_rewrite)
        ctrl->controlLocals = *ctrl->controlLocals.apply(RewriteTableKeys(*this));
    LOG5("DoLocalCopyPropagation before ElimDead " << ctrl->name);
    LOG5(ctrl);
    ctrl->controlLocals = *ctrl->controlLocals.apply(ElimDead(*this));
    ctrl->body = ctrl->body->apply(ElimDead(*this))->to<IR::BlockStatement>();
    working = false;
    available.clear();
    LOG3("DoLocalCopyPropagation finished control " << ctrl->name);
    LOG4(ctrl);
    prune();
    return ctrl;
}

void DoLocalCopyPropagation::apply_function(DoLocalCopyPropagation::FuncInfo *act) {
    for (auto write : act->writes)
        dropValuesUsing(write);
    for (auto read : act->reads)
        if (auto var = ::getref(available, read))
            var->live = true;
}

void DoLocalCopyPropagation::apply_table(DoLocalCopyPropagation::TableInfo *tbl) {
    ++tbl->apply_count;
    for (auto key : tbl->keyreads) {
        if (auto var = ::getref(available, key)) {
            if (var->val && lvalue_out(var->val)->is<IR::PathExpression>()) {
                if (tbl->apply_count > 1 &&
                    (!tbl->key_remap.count(key) || *tbl->key_remap.at(key) != *var->val)) {
                    /* FIXME -- need deep expr comparison here, not shallow */
                    LOG3("  different values used in different applies for key " << key);
                    tbl->key_remap.erase(key);
                    var->live = true;
                } else {
                    LOG3("  will propagate value into table key " << key << ": " << var->val);
                    tbl->key_remap.emplace(key, var->val);
                    need_key_rewrite = true; }
            } else {
                tbl->key_remap.erase(key);
                LOG4("  table using " << key << " with " <<
                     (var->val ? "value to complex for key" : "no propagated value"));
                var->live = true; } } }
    for (auto action : tbl->actions)
        apply_function(&actions[action]);
}

IR::P4Table *DoLocalCopyPropagation::preorder(IR::P4Table *tbl) {
    BUG_CHECK(!inferForTable, "corrupt internal data struct");
    inferForTable = &tables[tbl->name];
    inferForTable->keyreads.clear();
    for (auto ale : tbl->getActionList()->actionList)
        inferForTable->actions.insert(ale->getPath()->name);
    return tbl;
}

IR::P4Table *DoLocalCopyPropagation::postorder(IR::P4Table *tbl) {
    BUG_CHECK(inferForTable == &tables[tbl->name], "corrupt internal data struct");
    LOG4("table " << tbl->name << " reads=" << inferForTable->keyreads <<
         " actions=" << inferForTable->actions);
    inferForTable = nullptr;
    return tbl;
}

}  // namespace P4
