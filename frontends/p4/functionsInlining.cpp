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
#include "frontends/p4/callGraph.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

void DiscoverFunctionsInlining::postorder(const IR::MethodCallExpression* mce) {
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    CHECK_NULL(mi);
    auto ac = mi->to<P4::FunctionCall>();
    if (ac == nullptr)
        return;
    const IR::Node* caller = findContext<IR::Function>();
    if (caller == nullptr)
        caller = findContext<IR::P4Action>();
    if (caller == nullptr)
        caller = findContext<IR::P4Control>();
    if (caller == nullptr)
        caller = findContext<IR::P4Parser>();
    CHECK_NULL(caller);
    auto stat = findContext<IR::Statement>();
    CHECK_NULL(stat);
    BUG_CHECK(stat->is<IR::MethodCallStatement>() || stat->is<IR::AssignmentStatement>(),
              "%1%: unexpected statement with call", stat);

    auto aci = new FunctionCallInfo(caller, ac->function, stat);
    toInline->add(aci);
}

void FunctionsInliner::end_apply(const IR::Node*) {
    BUG_CHECK(replacementStack.empty(), "Non-empty replacement stack");
}

Visitor::profile_t FunctionsInliner::init_apply(const IR::Node* node) {
    P4::ResolveReferences solver(refMap, true);
    node->apply(solver);
    LOG2("FunctionsInliner " << toInline);
    return Transform::init_apply(node);
}

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

const IR::Node* FunctionsInliner::postCaller(const IR::Node* node) {
    LOG2("Ending: " << dbp(getOriginal()));
    if (toInline->sites.count(getOriginal()) > 0)
        list->replace(getOriginal(), node);
    BUG_CHECK(!replacementStack.empty(), "Empty replacement stack");
    replacementStack.pop_back();
    return node;
}

const IR::Node* FunctionsInliner::preorder(IR::MethodCallStatement* statement) {
    auto orig = getOriginal<IR::MethodCallStatement>();
    LOG2("Visiting " << dbp(orig));
    auto replMap = getReplacementMap();
    if (replMap == nullptr)
        return statement;

    auto callee = get(*replMap, orig);
    if (callee == nullptr)
        return statement;
    return inlineBefore(callee, statement->methodCall, statement);
}

const FunctionsInliner::ReplacementMap* FunctionsInliner::getReplacementMap() const {
    if (replacementStack.empty())
        return nullptr;
    return replacementStack.back();
}

void FunctionsInliner::dumpReplacementMap() const {
    auto replMap = getReplacementMap();
    LOG2("Replacement map contents");
    if (replMap == nullptr)
        return;
    for (auto it : *replMap)
        LOG2("\t" << it.first << " with " << it.second);
}

const IR::Node* FunctionsInliner::preorder(IR::AssignmentStatement* statement) {
    auto orig = getOriginal<IR::AssignmentStatement>();
    LOG2("Visiting " << dbp(orig));
    auto replMap = getReplacementMap();
    if (replMap == nullptr)
        return statement;

    auto callee = get(*replMap, orig);
    if (callee == nullptr)
        return statement;
    auto call = statement->right->to<IR::MethodCallExpression>();
    BUG_CHECK(call != nullptr,
              "%1%: expected a method call", statement->right);
    return inlineBefore(callee, call, statement);
}

const IR::Node* FunctionsInliner::preorder(IR::P4Parser* parser) {
    if (preCaller()) {
        parser->visit_children(*this);
        return postCaller(parser);
    } else {
        return parser;
    }
}

const IR::Node* FunctionsInliner::preorder(IR::P4Control* control) {
    bool hasWork = preCaller();
    // We always visit the children: there may be function calls in
    // actions within the control
    control->visit_children(*this);
    if (hasWork)
        return postCaller(control);
    return control;
}

const IR::Node* FunctionsInliner::preorder(IR::Function* function) {
    if (preCaller()) {
        function->visit_children(*this);
        return postCaller(function);
    } else {
        return function;
    }
}

const IR::Node* FunctionsInliner::preorder(IR::P4Action* action) {
    if (preCaller()) {
        action->visit_children(*this);
        return postCaller(action);
    } else {
        return action;
    }
}

const IR::Expression* FunctionsInliner::cloneBody(
    const IR::IndexedVector<IR::StatOrDecl>& src,
    IR::IndexedVector<IR::StatOrDecl>& dest) {
    const IR::Expression* retVal = nullptr;
    for (auto s : src) {
        if (auto rs = s->to<IR::ReturnStatement>())
            retVal = rs->expression;
        else
            dest.push_back(s);
    }
    return retVal;
}

const IR::Node* FunctionsInliner::inlineBefore(
    const IR::Node* calleeNode, const IR::MethodCallExpression* mce,
    const IR::Statement* statement) {
    LOG2("Inlining: " << dbp(calleeNode) << " before " << dbp(statement));

    auto callee = calleeNode->to<IR::Function>();
    BUG_CHECK(callee, "%1%: expected a function", calleeNode);

    IR::IndexedVector<IR::StatOrDecl> body;
    ParameterSubstitution subst;
    TypeVariableSubstitution tvs;  // empty

    std::map<const IR::Parameter*, cstring> paramRename;
    ParameterSubstitution substitution;
    substitution.populate(callee->type->parameters, mce->arguments);

    // evaluate in and inout parameters in order
    for (auto param : callee->type->parameters->parameters) {
        auto argument = substitution.lookup(param);
        cstring newName = refMap->newName(param->name);
        paramRename.emplace(param, newName);
        if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
            auto vardecl = new IR::Declaration_Variable(newName, param->annotations,
                                                        param->type, argument->expression);
            body.push_back(vardecl);
            subst.add(param, new IR::Argument(
                argument->srcInfo, argument->name, new IR::PathExpression(newName)));
        } else if (param->direction == IR::Direction::None) {
            // This works because there can be no side-effects in the evaluation of this
            // argument.
            subst.add(param, argument);
        } else if (param->direction == IR::Direction::Out) {
            // uninitialized variable
            auto vardecl = new IR::Declaration_Variable(newName,
                                                        param->annotations, param->type);
            subst.add(param, new IR::Argument(
                argument->srcInfo, argument->name, new IR::PathExpression(newName)));
            body.push_back(vardecl);
        }
    }

    SubstituteParameters sp(refMap, &subst, &tvs);
    auto clone = callee->apply(sp);
    if (::errorCount() > 0)
        return statement;
    CHECK_NULL(clone);
    BUG_CHECK(clone->is<IR::Function>(), "%1%: not an function", clone);
    auto funclone = clone->to<IR::Function>();
    auto retExpr = cloneBody(funclone->body->components, body);

    // copy out and inout parameters
    for (auto param : callee->type->parameters->parameters) {
        auto left = substitution.lookup(param);
        if (param->direction == IR::Direction::InOut || param->direction == IR::Direction::Out) {
            cstring newName = ::get(paramRename, param);
            auto right = new IR::PathExpression(newName);
            auto copyout = new IR::AssignmentStatement(left->expression, right);
            body.push_back(copyout);
        }
    }

    if (auto assign = statement->to<IR::AssignmentStatement>()) {
        // copy the return value
        CHECK_NULL(retExpr);
        auto setRetval = new IR::AssignmentStatement(assign->srcInfo, assign->left, retExpr);
        body.push_back(setRetval);
    } else {
        BUG_CHECK(statement->is<IR::MethodCallStatement>(),
                  "%1%: expected a method call", statement);
        // ignore the returned value.
    }

    auto result = new IR::BlockStatement(statement->srcInfo, body);
    LOG2("Replacing " << dbp(statement) << " with " << dbp(result));
    return result;
}

}  // namespace P4
