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

#include "functionsInlining.h"

#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "lib/rtti_utils.h"

namespace P4 {

void DiscoverFunctionsInlining::postorder(const IR::MethodCallExpression *mce) {
    auto mi = P4::MethodInstance::resolve(mce, this, typeMap);
    CHECK_NULL(mi);
    auto ac = mi->to<P4::FunctionCall>();
    if (ac == nullptr) return;
    const IR::Node *caller = findContext<IR::Function>();
    if (caller == nullptr) caller = findContext<IR::P4Action>();
    if (caller == nullptr) caller = findContext<IR::P4Control>();
    if (caller == nullptr) caller = findContext<IR::P4Parser>();
    CHECK_NULL(caller);

    auto stat = findContext<IR::Statement>();
    CHECK_NULL(stat);

    BUG_CHECK(bool(RTTI::isAny<IR::MethodCallStatement, IR::AssignmentStatement>(stat)),
              "%1%: unexpected statement with call", stat);

    auto aci = new FunctionCallInfo(caller, ac->function, stat, mce);
    toInline->add(aci);
}

Visitor::profile_t DiscoverFunctionsInlining::init_apply(const IR::Node *node) {
    auto rv = Inspector::init_apply(node);
    toInline->clear();

    return rv;
}

void FunctionsInliner::end_apply(const IR::Node *) {
    BUG_CHECK(replacementStack.empty(), "Non-empty replacement stack");
    pendingReplacements.clear();
}

Visitor::profile_t FunctionsInliner::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);

    LOG2("FunctionsInliner " << toInline);

    if (!nameGen) {
        // This would apply nameGen onto node
        nameGen = std::make_unique<MinimalNameGenerator>(node);
    }

    return rv;
}

class FunctionsInliner::isLocalExpression : public Inspector, ResolutionContext {
    bool done = false;
    bool result = true;

    profile_t init_apply(const IR::Node *node) override {
        BUG_CHECK(!done, "isLocalExpression can only be applied once");
        return Inspector::init_apply(node);
    }
    void end_apply() override { done = true; }
    bool preorder(const IR::Node *) override { return result; }
    bool preorder(const IR::Path *p) override {
        if (p->absolute) return result = false;
        // cribbed from ResolutionContext::resolve -- we want to resolve the name,
        // except we want to know what scope it is found in, not what it resolves to
        const Context *ctxt = nullptr;
        while (auto scope = findOrigCtxt<IR::INamespace>(ctxt)) {
            if (scope->is<IR::P4Control>() || scope->is<IR::P4Parser>() ||
                scope->is<IR::P4Program>() || scope->is<IR::V1Control>() ||
                scope->is<IR::V1Parser>() || scope->is<IR::V1Program>()) {
                // these are "global" things that may contain functions
                return result = false;
            }
            if (!lookup(scope, p->name, P4::ResolutionType::Any).empty()) return result;
            if (scope->is<IR::Function>() || scope->is<IR::P4Action>()) {
                // no need to look further as can't have nested functions
                return result = false;
            }
        }
        BUG("failed to reach global scope");
        return result;
    }

 public:
    isLocalExpression(const IR::Expression *expr, const Visitor_Context *ctxt) {
        expr->apply(*this, ctxt);
    }
    explicit operator bool() {
        BUG_CHECK(done, "isLocalExpression not computed");
        return result;
    }
};

bool FunctionsInliner::preCaller() {
    LOG2("Visiting: " << dbp(getOriginal()));
    if (toInline->sites.count(getOriginal()) == 0) {
        prune();
        return false;
    }
    auto replMap = &toInline->sites[getOriginal()];
    replacementStack.push_back(replMap);
    return true;
}

const IR::Node *FunctionsInliner::postCaller(const IR::Node *node) {
    LOG2("Ending: " << dbp(getOriginal()));
    if (toInline->sites.count(getOriginal()) > 0) list->replace(getOriginal(), node);
    BUG_CHECK(!replacementStack.empty(), "Empty replacement stack");
    replacementStack.pop_back();
    return node;
}

const IR::Node *FunctionsInliner::preorder(IR::MethodCallStatement *statement) {
    auto orig = getOriginal<IR::MethodCallStatement>();
    LOG2("Visiting " << dbp(orig));
    auto replMap = getReplacementMap();
    if (replMap == nullptr) return statement;

    auto [callee, _] = get(*replMap, orig);
    if (callee == nullptr) return statement;
    return inlineBefore(callee, statement->methodCall, statement);
}

const IR::Node *FunctionsInliner::preorder(IR::MethodCallExpression *expr) {
    auto orig = getOriginal<IR::MethodCallExpression>();
    LOG2("Visiting " << dbp(orig));

    auto it = pendingReplacements.find(orig);
    if (it != pendingReplacements.end()) {
        LOG2("Performing pending replacement of " << dbp(expr) << " with " << dbp(it->second));
        return it->second;
    }

    return expr;
}

const FunctionsInliner::ReplacementMap *FunctionsInliner::getReplacementMap() const {
    if (replacementStack.empty()) return nullptr;
    return replacementStack.back();
}

void FunctionsInliner::dumpReplacementMap() const {
    auto replMap = getReplacementMap();
    LOG2("Replacement map contents");
    if (replMap == nullptr) return;
    for (auto it : *replMap)
        LOG2("\t" << it.first << " with " << it.second.first << " via " << it.second.second);
}

const IR::Node *FunctionsInliner::preorder(IR::AssignmentStatement *statement) {
    auto orig = getOriginal<IR::AssignmentStatement>();
    LOG2("Visiting " << dbp(orig));

    auto replMap = getReplacementMap();
    if (replMap == nullptr) return statement;

    auto [callee, callExpr] = get(*replMap, orig);
    if (callee == nullptr) return statement;
    BUG_CHECK(callExpr != nullptr, "%1%: expected a method call", statement->right);
    return inlineBefore(callee, callExpr, statement);
}

const IR::Node *FunctionsInliner::preorder(IR::P4Parser *parser) {
    if (preCaller()) {
        parser->visit_children(*this);
        return postCaller(parser);
    } else {
        return parser;
    }
}

const IR::Node *FunctionsInliner::preorder(IR::P4Control *control) {
    bool hasWork = preCaller();
    // We always visit the children: there may be function calls in
    // actions within the control
    control->visit_children(*this);
    if (hasWork) return postCaller(control);
    return control;
}

const IR::Node *FunctionsInliner::preorder(IR::Function *function) {
    if (preCaller()) {
        function->visit_children(*this);
        return postCaller(function);
    } else {
        return function;
    }
}

const IR::Node *FunctionsInliner::preorder(IR::P4Action *action) {
    if (preCaller()) {
        action->visit_children(*this);
        return postCaller(action);
    } else {
        return action;
    }
}

const IR::Expression *FunctionsInliner::cloneBody(const IR::IndexedVector<IR::StatOrDecl> &src,
                                                  IR::IndexedVector<IR::StatOrDecl> &dest) {
    const IR::Expression *retVal = nullptr;
    for (auto s : src) {
        if (auto rs = s->to<IR::ReturnStatement>())
            retVal = rs->expression;
        else
            dest.push_back(s);
    }
    return retVal;
}

const IR::Statement *FunctionsInliner::inlineBefore(const IR::Node *calleeNode,
                                                    const IR::MethodCallExpression *mce,
                                                    const IR::Statement *statement) {
    LOG2("Inlining: " << dbp(calleeNode) << " before " << dbp(statement));

    auto callee = calleeNode->to<IR::Function>();
    BUG_CHECK(callee, "%1%: expected a function", calleeNode);

    IR::IndexedVector<IR::StatOrDecl> body;
    ParameterSubstitution subst;   // rewrites for params
    TypeVariableSubstitution tvs;  // empty

    ParameterSubstitution substitution;  // map params to actual arguments
    substitution.populate(callee->type->parameters, mce->arguments);

    // parameters that need copyout
    std::vector<std::pair<cstring, const IR::Argument *>> needCopyout;

    // evaluate in and inout parameters in order
    for (auto param : callee->type->parameters->parameters) {
        auto argument = substitution.lookup(param);
        cstring newName = nameGen->newName(param->name.name.string_view());
        if ((param->direction == IR::Direction::Out || param->direction == IR::Direction::InOut) &&
            isLocalExpression(argument->expression, getChildContext())) {
            // If the actual parameter is local to the caller, we can just rewrite the callee
            // to access it directly, without the overhead of copying it in or out
            subst.add(param, argument);
        } else if (param->direction == IR::Direction::In ||
                   param->direction == IR::Direction::InOut) {
            auto vardecl = new IR::Declaration_Variable(newName, param->annotations, param->type);
            body.push_back(vardecl);
            auto copyin =
                new IR::AssignmentStatement(new IR::PathExpression(newName), argument->expression);
            body.push_back(copyin);
            subst.add(param, new IR::Argument(argument->srcInfo, argument->name,
                                              new IR::PathExpression(newName)));
            if (param->direction == IR::Direction::InOut)
                needCopyout.emplace_back(newName, argument);
        } else if (param->direction == IR::Direction::None) {
            // This works because there can be no side-effects in the evaluation of this
            // argument.
            subst.add(param, argument);
        } else if (param->direction == IR::Direction::Out) {
            // uninitialized variable
            auto vardecl = new IR::Declaration_Variable(newName, param->annotations, param->type);
            subst.add(param, new IR::Argument(argument->srcInfo, argument->name,
                                              new IR::PathExpression(newName)));
            body.push_back(vardecl);
            needCopyout.emplace_back(newName, argument);
        }
    }

    // FIXME: SubstituteParameters is a ResolutionContext, but we are recreating
    // it again and again killing lookup caches. We'd need to have instead some
    // separate DeclarationLookup that would allow us to perform necessary
    // lookups in the context of callee. We can probably pre-seed it first for
    // all possible callees in replMap.
    SubstituteParameters sp(nullptr, &subst, &tvs);
    auto clone = callee->apply(sp, getChildContext());
    if (::P4::errorCount() > 0) return statement;
    CHECK_NULL(clone);
    BUG_CHECK(clone->is<IR::Function>(), "%1%: not an function", clone);
    auto funclone = clone->to<IR::Function>();
    auto retExpr = cloneBody(funclone->body->components, body);

    // copy out and inout parameters
    for (auto [newName, argument] : needCopyout) {
        auto right = new IR::PathExpression(newName);
        auto copyout = new IR::AssignmentStatement(argument->expression, right);
        body.push_back(copyout);
    }

    if (auto assign = statement->to<IR::AssignmentStatement>()) {
        // copy the return value
        CHECK_NULL(retExpr);
        // If we can replace RHS immediately, do it here, otherwise add return
        // value to pending list
        if (assign->right == mce)
            body.push_back(new IR::AssignmentStatement(assign->srcInfo, assign->left, retExpr));
        else {
            body.push_back(assign->clone());
            auto [it, inserted] = pendingReplacements.emplace(mce, retExpr);
            BUG_CHECK(inserted, "%1%: duplicate value for pending replacements", it->first);
        }
    } else {
        BUG_CHECK(statement->is<IR::MethodCallStatement>(), "%1%: expected a method call",
                  statement);
        // ignore the returned value.
    }

    auto result = new IR::BlockStatement(statement->srcInfo, body);
    LOG2("Replacing " << dbp(statement) << " with " << dbp(result));
    return result;
}

}  // namespace P4
