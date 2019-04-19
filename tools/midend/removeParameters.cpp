/*
Copyright 2016 VMware, Inc.

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

#include "removeParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

namespace {
// Remove arguments from any embedded MethodCallExpression
class RemoveMethodCallArguments : public Transform {
    int argumentsToRemove;  // -1 => all
 public:
    explicit RemoveMethodCallArguments(int toRemove = -1) : argumentsToRemove(toRemove)
    { setName("RemoveMethodCallArguments"); }
    const IR::Node* postorder(IR::MethodCallExpression* expression) override {
        if (argumentsToRemove == -1) {
            expression->arguments = new IR::Vector<IR::Argument>();
        } else {
            auto args = new IR::Vector<IR::Argument>();
            for (int i = 0; i < static_cast<int>(expression->arguments->size()); i++) {
                if (i < argumentsToRemove)
                    continue;
                args->push_back(expression->arguments->at(i));
            }
            expression->arguments = args;
        }
        return expression;
    }
};
}  // namespace

void FindActionParameters::postorder(const IR::ActionListElement* element) {
    auto path = element->getPath();
    auto decl = refMap->getDeclaration(path, true);
    BUG_CHECK(decl->is<IR::P4Action>(), "%1%: not an action", element);
    BUG_CHECK(element->expression->is<IR::MethodCallExpression>(),
              "%1%: expected a method call", element->expression);
    invocations->bind(decl->to<IR::P4Action>(),
                      element->expression->to<IR::MethodCallExpression>(), false);
}

void FindActionParameters::postorder(const IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    if (!mi->is<P4::ActionCall>())
        return;
    auto ac = mi->to<P4::ActionCall>();

    auto table = findContext<IR::P4Table>();
    if (table != nullptr) {
        if (findContext<IR::ActionListElement>() != nullptr)
            // These are processed elsewhere
            return;
        // This is probably the default_action; we must remove some parameters
        invocations->bindDefaultAction(ac->action, expression);
    } else {
        // Direction action invocation: remove all parameters
        invocations->bind(ac->action, expression, true);
    }
}

const IR::Node* DoRemoveActionParameters::postorder(IR::P4Action* action) {
    LOG1("Visiting " << dbp(action));
    BUG_CHECK(getParent<IR::P4Control>() || getParent<IR::P4Program>(),
              "%1%: unexpected parent %2%", getOriginal(), getContext()->node);
    auto result = new IR::IndexedVector<IR::Declaration>();
    auto leftParams = new IR::IndexedVector<IR::Parameter>();
    auto initializers = new IR::IndexedVector<IR::StatOrDecl>();
    auto postamble = new IR::IndexedVector<IR::StatOrDecl>();
    auto invocation = invocations->get(getOriginal<IR::P4Action>());
    if (invocation == nullptr)
        return action;
    auto args = invocation->arguments;

    ParameterSubstitution substitution;
    substitution.populate(action->parameters, args);

    bool removeAll = invocations->removeAllParameters(getOriginal<IR::P4Action>());
    for (auto p : action->parameters->parameters) {
        if (p->direction == IR::Direction::None && !removeAll) {
            leftParams->push_back(p);
        } else {
            auto decl = new IR::Declaration_Variable(p->srcInfo, p->name,
                                                     p->annotations, p->type, nullptr);
            LOG3("Added declaration " << decl << " annotations " << p->annotations);
            result->push_back(decl);
            auto arg = substitution.lookup(p);
            if (arg == nullptr) {
                ::error("action %1%: parameter %2% must be bound", invocation, p);
                continue;
            }

            if (p->direction == IR::Direction::In ||
                p->direction == IR::Direction::InOut ||
                p->direction == IR::Direction::None) {
                auto left = new IR::PathExpression(p->name);
                auto assign = new IR::AssignmentStatement(arg->srcInfo, left, arg->expression);
                initializers->push_back(assign);
            }
            if (p->direction == IR::Direction::Out ||
                p->direction == IR::Direction::InOut) {
                auto right = new IR::PathExpression(p->name);
                auto assign = new IR::AssignmentStatement(arg->srcInfo, arg->expression, right);
                postamble->push_back(assign);
            }
        }
    }
    if (result->empty())
        return action;

    initializers->append(action->body->components);
    initializers->append(*postamble);

    action->parameters = new IR::ParameterList(action->parameters->srcInfo, *leftParams);
    action->body = new IR::BlockStatement(action->body->srcInfo, *initializers);
    LOG1("To replace " << dbp(action));
    result->push_back(action);
    return result;
}

const IR::Node* DoRemoveActionParameters::postorder(IR::ActionListElement* element) {
    RemoveMethodCallArguments rmca;
    element->expression = element->expression->apply(rmca)->to<IR::Expression>();
    return element;
}

const IR::Node* DoRemoveActionParameters::postorder(IR::MethodCallExpression* expression) {
    auto orig = getOriginal<IR::MethodCallExpression>();
    if (invocations->isCall(orig)) {
        RemoveMethodCallArguments rmca;
        return expression->apply(rmca);
    } else if (unsigned toRemove = invocations->argsToRemove(orig)) {
        RemoveMethodCallArguments rmca(toRemove);
        return expression->apply(rmca);
    }
    return expression;
}

RemoveActionParameters::RemoveActionParameters(ReferenceMap* refMap, TypeMap* typeMap,
                                               TypeChecking* typeChecking) {
    setName("RemoveActionParameters");
    auto ai = new ActionInvocation();
    // MoveDeclarations() is needed because of this case:
    // action a(inout x) { x = x + 1 }
    // bit<32> w;
    // table t() { actions = a(w); ... }
    // Without the MoveDeclarations the code would become
    // action a() { x = w; x = x + 1; w = x; } << w is not yet defined
    // bit<32> w;
    // table t() { actions = a(); ... }
    passes.emplace_back(new MoveDeclarations());
    if (!typeChecking)
        typeChecking = new TypeChecking(refMap, typeMap);
    passes.emplace_back(typeChecking);
    passes.emplace_back(new FindActionParameters(refMap, typeMap, ai));
    passes.emplace_back(new DoRemoveActionParameters(ai));
    passes.emplace_back(new ClearTypeMap(typeMap));
}

}  // namespace P4
