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

#include "inlining.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace BMV2 {

Visitor::profile_t SimpleControlsInliner::init_apply(const IR::Node* node) {
    P4::ResolveReferences solver(refMap, true);
    node->apply(solver);
    return AbstractInliner::init_apply(node);
}

const IR::Node* SimpleControlsInliner::preorder(IR::P4Control* caller) {
    auto orig = getOriginal<IR::P4Control>();
    if (toInline->callerToWork.find(orig) == toInline->callerToWork.end()) {
        prune();
        return caller;
    }

    workToDo = &toInline->callerToWork[orig];
    LOG1("Simple inliner " << caller);
    auto stateful = new IR::IndexedVector<IR::Declaration>();
    for (auto s : caller->controlLocals) {
        auto inst = s->to<IR::Declaration_Instance>();
        if (inst == nullptr ||
            workToDo->declToCallee.find(inst) == workToDo->declToCallee.end()) {
            // If some element with the same name is already there we don't reinsert it
            // since this program is generated from a P4 v1.0 program by duplicating
            // elements.
            if (stateful->getDeclaration(s->name) == nullptr)
                stateful->push_back(s);
        } else {
            auto callee = workToDo->declToCallee[inst];
            CHECK_NULL(callee);
            P4::ParameterSubstitution subst;
            // This is correct only if the arguments have no side-effects.
            // There should be a prior pass to ensure this fact.  This is
            // true for programs that come out of the P4 v1.0 front-end.
            subst.populate(callee->getConstructorParameters(), inst->arguments);
            P4::TypeVariableSubstitution tvs;
            if (inst->type->is<IR::Type_Specialized>()) {
                auto spec = inst->type->to<IR::Type_Specialized>();
                tvs.setBindings(callee->getNode(), callee->getTypeParameters(), spec->arguments);
            }
            P4::SubstituteParameters sp(refMap, &subst, &tvs);
            auto clone = callee->getNode()->apply(sp);
            if (clone == nullptr)
                return caller;
            for (auto i : clone->to<IR::P4Control>()->controlLocals) {
                if (stateful->getDeclaration(i->name) == nullptr)
                    stateful->push_back(i);
            }
            workToDo->declToCallee[inst] = clone->to<IR::IContainer>();
        }
    }

    visit(caller->body);
    auto result = new IR::P4Control(caller->srcInfo, caller->name, caller->type,
                                    caller->constructorParams, *stateful,
                                    caller->body);
    list->replace(orig, result);
    workToDo = nullptr;
    prune();
    return result;
}

const IR::Node* SimpleControlsInliner::preorder(IR::MethodCallStatement* statement) {
    if (workToDo == nullptr)
        return statement;
    auto orig = getOriginal<IR::MethodCallStatement>();
    if (workToDo->callToInstance.find(orig) == workToDo->callToInstance.end())
        return statement;
    LOG1("Inlining invocation " << orig);
    auto decl = workToDo->callToInstance[orig];
    CHECK_NULL(decl);
    prune();
    return workToDo->declToCallee[decl]->to<IR::P4Control>()->body;
}

Visitor::profile_t SimpleActionsInliner::init_apply(const IR::Node* node) {
    P4::ResolveReferences solver(refMap, true);
    node->apply(solver);
    LOG1("SimpleActionsInliner " << toInline);
    return Transform::init_apply(node);
}

const IR::Node* SimpleActionsInliner::preorder(IR::P4Action* action) {
    if (toInline->sites.count(getOriginal<IR::P4Action>()) == 0)
        prune();
    replMap = &toInline->sites[getOriginal<IR::P4Action>()];
    LOG1("Visiting: " << getOriginal());
    return action;
}

const IR::Node* SimpleActionsInliner::postorder(IR::P4Action* action) {
    if (toInline->sites.count(getOriginal<IR::P4Action>()) > 0)
        list->replace(getOriginal<IR::P4Action>(), action);
    replMap = nullptr;
    return action;
}

const IR::Node* SimpleActionsInliner::preorder(IR::MethodCallStatement* statement) {
    LOG1("Visiting " << getOriginal());
    if (replMap == nullptr)
        return statement;

    auto callee = get(*replMap, getOriginal<IR::MethodCallStatement>());
    if (callee == nullptr)
        return statement;

    LOG1("Inlining: " << toInline);
    P4::ParameterSubstitution subst;
    subst.populate(callee->parameters, statement->methodCall->arguments);
    P4::TypeVariableSubstitution tvs;  // empty
    P4::SubstituteParameters sp(refMap, &subst, &tvs);
    auto clone = callee->apply(sp);
    if (::errorCount() > 0)
        return statement;
    CHECK_NULL(clone);
    BUG_CHECK(clone->is<IR::P4Action>(), "%1%: not an action", clone);
    auto actclone = clone->to<IR::P4Action>();
    auto result = actclone->body;
    LOG1("Replacing " << statement << " with " << result);
    return result;
}

}  // namespace BMV2
