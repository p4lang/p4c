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
    auto stateful = new IR::NameMap<IR::Declaration, ordered_map>();
    for (auto s : *caller->statefulEnumerator()) {
        auto inst = s->to<IR::Declaration_Instance>();
        if (inst == nullptr ||
            workToDo->declToCallee.find(inst) == workToDo->declToCallee.end()) {
            // If some element with the same name is already there we don't reinsert it
            // since this program is generated from a P4 v1.0 program by duplicating
            // elements.
            if (stateful->getUnique(s->name) == nullptr)
                stateful->addUnique(s->name, s);
        } else {
            auto callee = workToDo->declToCallee[inst];
            CHECK_NULL(callee);
            IR::ParameterSubstitution subst;
            // This is correct only if the arguments have no side-effects.
            // There should be a prior pass to ensure this fact.  This is
            // true for programs that come out of the P4 v1.0 front-end.
            subst.populate(callee->getConstructorParameters(), inst->arguments);
            IR::TypeVariableSubstitution tvs;
            if (inst->type->is<IR::Type_Specialized>()) {
                auto spec = inst->type->to<IR::Type_Specialized>();
                tvs.setBindings(callee->getNode(), callee->getTypeParameters(), spec->arguments);
            }
            P4::SubstituteParameters sp(refMap, &subst, &tvs);
            auto clone = callee->getNode()->apply(sp);
            if (clone == nullptr)
                return caller;
            for (auto i : *clone->to<IR::P4Control>()->statefulEnumerator()) {
                if (stateful->getUnique(i->name) == nullptr)
                    stateful->addUnique(i->name, i);
            }
            workToDo->declToCallee[inst] = clone->to<IR::IContainer>();
        }
    }

    visit(caller->body);
    auto result = new IR::P4Control(caller->srcInfo, caller->name, caller->type,
                                    caller->constructorParams, std::move(*stateful),
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
    if (workToDo->callToinstance.find(orig) == workToDo->callToinstance.end())
        return statement;
    LOG1("Inlining invocation " << orig);
    auto decl = workToDo->callToinstance[orig];
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
    IR::ParameterSubstitution subst;
    subst.populate(callee->parameters, statement->methodCall->arguments);
    IR::TypeVariableSubstitution tvs;  // empty
    P4::SubstituteParameters sp(refMap, &subst, &tvs);
    auto clone = callee->apply(sp);
    if (::errorCount() > 0)
        return statement;
    CHECK_NULL(clone);
    BUG_CHECK(clone->is<IR::P4Action>(), "%1%: not an action", clone);
    auto actclone = clone->to<IR::P4Action>();
    auto result = new IR::BlockStatement(actclone->body->srcInfo, actclone->body);
    LOG1("Replacing " << statement << " with " << result);
    return result;
}

}  // namespace BMV2
