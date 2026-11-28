/*
Copyright 2025 Nvidia, Inc.

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

#include "checkArgAlias.h"

#include "methodInstance.h"

namespace P4 {

// find the enclosing 'location' expression from the current context.  That
// is, any Member(s) or ArrayIndex(s) that are a parents of the current node.
const IR::Expression *CheckArgAlias::FindCaptures::getRefCtxt() {
    auto *ctxt = getChildContext();
    for (; auto *p = ctxt->parent; ctxt = p) {
        if (p->node->is<IR::Member>()) continue;
        if (p->node->is<IR::ArrayIndex>() && p->child_index == 0) continue;
        break;
    }
    return ctxt->node->to<IR::Expression>();
}

bool CheckArgAlias::FindCaptures::preorder(const IR::PathExpression *pe) {
    const Context *ctxt = nullptr;
    auto last = visiting.end();
    --last;
    while (auto scope = findOrigCtxt<IR::INamespace>(ctxt)) {
        auto rv = lookup(scope, pe->path->name, ResolutionType::Any);
        if (!rv.empty()) break;
        if (last->second == scope->getNode()) --last;
    }
    auto expr = getRefCtxt();
    for (++last; last < visiting.end(); ++last) {
        LOG4(expr << " escapes from " << DBPrint::Brief << last->first);
        result[last->first][pe->path->name].push_back(expr);
    }
    return true;
}

bool CheckArgAlias::FindCaptures::preorder(const IR::IApply *ia) {
    LOG3("CheckArgAlias::preorder(IA): " << DBPrint::Brief << ia);
    auto *d = ia->to<IR::IDeclaration>();
    visiting.push_back(std::make_pair(d, d->getNode()));
    return true;
}

bool CheckArgAlias::FindCaptures::preorder(const IR::IFunctional *fn) {
    LOG3("CheckArgAlias::preorder(IF): " << DBPrint::Brief << fn);
    auto *d = fn->to<IR::IDeclaration>();
    visiting.push_back(std::make_pair(d, d->getNode()));
    return true;
}

const IR::Expression *CheckArgAlias::Check::baseExpr(const IR::Expression *e, int *depth) {
    if (depth) ++*depth;
    if (auto *mem = e->to<IR::Member>()) return baseExpr(mem->expr, depth);
    if (auto *ai = e->to<IR::ArrayIndex>()) return baseExpr(ai->left, depth);
    return e;
}

int CheckArgAlias::Check::baseExprDepth(const IR::Expression *e) {
    int depth = 0;
    baseExpr(e, &depth);
    return depth;
}

bool CheckArgAlias::Check::overlaps(const IR::Expression *e1, const IR::Expression *e2) {
    while (auto *ai = e1->to<IR::ArrayIndex>()) e1 = ai->left;
    while (auto *ai = e2->to<IR::ArrayIndex>()) e2 = ai->left;
    auto *m1 = e1->to<IR::Member>();
    auto *m2 = e2->to<IR::Member>();
    if (m1 && m2) {
        if (m1->expr->type == m2->expr->type) return m1->member == m2->member;
        if (baseExprDepth(m1->expr) > baseExprDepth(m2->expr))
            return overlaps(m1->expr, e2);
        else
            return overlaps(e1, m2->expr);
    } else if (m1) {
        return overlaps(m1->expr, e2);
    } else if (m2) {
        return overlaps(e1, m2->expr);
    }
    BUG_CHECK(e1->is<IR::PathExpression>(), "'%1%' not a PathExpression", e1);
    BUG_CHECK(e2->is<IR::PathExpression>(), "'%1%' not a PathExpression", e2);
    BUG_CHECK(e1->equiv(*e2), "'%1%' and '%2%' are different", e1, e2);
    return e1->equiv(*e2);
}

bool CheckArgAlias::Check::preorder(const IR::MethodCallExpression *mce) {
    LOG3("CheckArgAlias::Check::preorder(MCE): " << mce);
    auto *mi = MethodInstance::resolve(mce, this, self.typeMap);
    BUG_CHECK(mi, "MethodInstance::resolve failed");
    if (!self.captures.any(mi->callee())) return true;
    auto arg = mi->expr->arguments->begin();
    auto *params = mi->actualMethodType->parameters;
    for (auto it = params->begin(); it < params->end(); ++it, ++arg) {
        if (arg == mi->expr->arguments->end()) {
            // at the end of the argument list -- rest of params must be directionless
            // and this must be an action call
            BUG_CHECK(mi->is<P4::ActionCall>(), "not an action call?");
            break;
        }
        auto *param = *it;
        if (param->direction == IR::Direction::None) continue;
        if (param->direction == IR::Direction::In) continue;
        auto *exp = (*arg)->expression;
        while (auto *sl = exp->to<IR::AbstractSlice>()) exp = sl->e0;
        auto *pe = baseExpr(exp)->to<IR::PathExpression>();
        BUG_CHECK(pe, "not a PathExpression");
        for (auto *cap : self.captures(mi->callee(), pe->path->name)) {
            if (overlaps(exp, cap)) {
                error(ErrorType::ERR_INVALID, "%1%%2% binds value accessed by callee %3%%4%",
                      param->direction == IR::Direction::InOut ? "inout argument" : "out argument",
                      (*arg)->srcInfo, mi->callee(), cap->srcInfo);
                // only trigger once per call
                return true;
            }
        }
    }
    return true;
}

}  // namespace P4
