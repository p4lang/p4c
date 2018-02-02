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

#include "actionsInlining.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

void AInlineWorkList::dbprint(std::ostream& out) const {
    for (auto t : sites) {
        out << t.first;
        for (auto c : t.second) {
            out << std::endl << "\t" << c.first << " => " << c.second;
        }
    }
}

void ActionsInlineList::analyze() {
    P4::CallGraph<const IR::P4Action*> cg("Actions call-graph");

    for (auto c : toInline)
        cg.calls(c->caller, c->callee);

    // must inline from leaves up
    std::vector<const IR::P4Action*> order;
    cg.sort(order);
    for (auto c : order) {
        // This is quadratic, but hopefully the call graph is not too large
        for (auto ci : toInline) {
            if (ci->caller == c)
                inlineOrder.push_back(ci);
        }
    }

    std::reverse(inlineOrder.begin(), inlineOrder.end());
}

AInlineWorkList* ActionsInlineList::next() {
    if (inlineOrder.size() == 0)
        return nullptr;

    std::set<const IR::P4Action*> callers;
    auto result = new AInlineWorkList();

    // Find actions that can be inlined simultaneously.
    // This traversal is in topological order starting from leaf callees.
    // We stop at the first action which calls one of the actions
    // we have already selected.
    while (!inlineOrder.empty()) {
        auto last = inlineOrder.back();
        if (callers.find(last->callee) != callers.end())
            break;
        inlineOrder.pop_back();
        result->add(last);
        callers.emplace(last->caller);
    }
    BUG_CHECK(!result->empty(), "Empty list of methods to inline");
    return result;
}

void DiscoverActionsInlining::postorder(const IR::MethodCallStatement* mcs) {
    auto mi = P4::MethodInstance::resolve(mcs, refMap, typeMap);
    CHECK_NULL(mi);
    auto ac = mi->to<P4::ActionCall>();
    if (ac == nullptr)
        return;
    auto caller = findContext<IR::P4Action>();
    if (caller == nullptr) {
        if (findContext<IR::P4Parser>() != nullptr) {
            ::error("%1%: action invocation in parser not supported", mcs);
        } else if (findContext<IR::P4Control>() == nullptr) {
            BUG("%1%: unexpected action invocation", mcs);
        }
        return;
    }

    auto aci = new ActionCallInfo(caller, ac->action, mcs);
    toInline->add(aci);
}

const IR::Node* InlineActionsDriver::preorder(IR::P4Program* program) {
    LOG1("Inline actions driver");
    const IR::P4Program* prog = program;
    toInline->analyze();
    while (auto todo = toInline->next()) {
        LOG1("Processing " << todo);
        inliner->prepare(toInline, todo);
        prog = prog->apply(*inliner);
        if (::errorCount() > 0)
            break;
    }

    prune();
    return prog;
}

Visitor::profile_t ActionsInliner::init_apply(const IR::Node* node) {
    P4::ResolveReferences solver(refMap, true);
    node->apply(solver);
    LOG1("ActionsInliner " << toInline);
    return Transform::init_apply(node);
}

const IR::Node* ActionsInliner::preorder(IR::P4Action* action) {
    if (toInline->sites.count(getOriginal<IR::P4Action>()) == 0)
        prune();
    replMap = &toInline->sites[getOriginal<IR::P4Action>()];
    LOG1("Visiting: " << getOriginal());
    return action;
}

const IR::Node* ActionsInliner::postorder(IR::P4Action* action) {
    if (toInline->sites.count(getOriginal<IR::P4Action>()) > 0)
        list->replace(getOriginal<IR::P4Action>(), action);
    replMap = nullptr;
    return action;
}

const IR::Node* ActionsInliner::preorder(IR::MethodCallStatement* statement) {
    auto orig = getOriginal<IR::MethodCallStatement>();
    LOG1("Visiting " << orig);
    if (replMap == nullptr)
        return statement;

    auto callee = get(*replMap, orig);
    if (callee == nullptr)
        return statement;

    LOG1("Inlining: " << toInline);
    IR::IndexedVector<IR::StatOrDecl> body;
    ParameterSubstitution subst;
    TypeVariableSubstitution tvs;  // empty

    std::map<const IR::Parameter*, cstring> paramRename;

    // evaluate in and inout parameters in order
    auto it = statement->methodCall->arguments->begin();
    for (auto param : callee->parameters->parameters) {
        auto initializer = *it;
        cstring newName = refMap->newName(param->name);
        paramRename.emplace(param, newName);
        if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
            auto vardecl = new IR::Declaration_Variable(newName, param->annotations,
                                                        param->type, initializer);
            body.push_back(vardecl);
            subst.add(param, new IR::PathExpression(newName));
        } else if (param->direction == IR::Direction::None) {
            // This works because there can be no side-effects in the evaluation of this
            // argument.
            subst.add(param, initializer);
        } else if (param->direction == IR::Direction::Out) {
            // uninitialized variable
            auto vardecl = new IR::Declaration_Variable(newName,
                                                        param->annotations, param->type);
            subst.add(param, new IR::PathExpression(newName));
            body.push_back(vardecl);
        }
        ++it;
    }

    P4::SubstituteParameters sp(refMap, &subst, &tvs);
    auto clone = callee->apply(sp);
    if (::errorCount() > 0)
        return statement;
    CHECK_NULL(clone);
    BUG_CHECK(clone->is<IR::P4Action>(), "%1%: not an action", clone);
    auto actclone = clone->to<IR::P4Action>();
    body.append(actclone->body->components);

    // copy out and inout parameters
    it = statement->methodCall->arguments->begin();
    for (auto param : callee->parameters->parameters) {
        auto left = *it;
        if (param->direction == IR::Direction::InOut || param->direction == IR::Direction::Out) {
            cstring newName = ::get(paramRename, param);
            auto right = new IR::PathExpression(newName);
            auto copyout = new IR::AssignmentStatement(left, right);
            body.push_back(copyout);
        }
        ++it;
    }

    auto annotations = callee->annotations->where(
        [](const IR::Annotation* a) { return a->name != IR::Annotation::nameAnnotation; });
    auto result = new IR::BlockStatement(statement->srcInfo, annotations, body);
    LOG1("Replacing " << orig << " with " << result);
    return result;
}

}  // namespace P4
