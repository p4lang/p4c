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

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"

namespace P4C::P4 {

void DiscoverActionsInlining::postorder(const IR::MethodCallStatement *mcs) {
    auto mi = P4::MethodInstance::resolve(mcs, this, typeMap);
    CHECK_NULL(mi);
    auto ac = mi->to<P4::ActionCall>();
    if (ac == nullptr) return;
    auto caller = findContext<IR::P4Action>();
    if (caller == nullptr) {
        if (findContext<IR::P4Parser>() != nullptr) {
            ::P4C::error(ErrorType::ERR_UNSUPPORTED, "%1%: action invocation in parser not allowed",
                         mcs);
        } else if (findContext<IR::P4Control>() == nullptr) {
            BUG("%1%: unexpected action invocation", mcs);
        }
        return;
    }

    auto aci = new ActionCallInfo(caller, ac->action, mcs, mcs->methodCall);
    toInline->add(aci);
}

Visitor::profile_t ActionsInliner::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);

    LOG2("ActionsInliner " << toInline);
    if (!nameGen) {
        // This would apply nameGen onto node
        nameGen = std::make_unique<MinimalNameGenerator>(node);
    }

    return rv;
}

const IR::Node *ActionsInliner::preorder(IR::P4Action *action) {
    if (toInline->sites.count(getOriginal<IR::P4Action>()) == 0) prune();
    replMap = &toInline->sites[getOriginal<IR::P4Action>()];
    LOG2("Visiting: " << getOriginal());
    return action;
}

const IR::Node *ActionsInliner::postorder(IR::P4Action *action) {
    if (toInline->sites.count(getOriginal<IR::P4Action>()) > 0)
        list->replace(getOriginal<IR::P4Action>(), action);
    replMap = nullptr;
    return action;
}

const IR::Node *ActionsInliner::preorder(IR::MethodCallStatement *statement) {
    auto orig = getOriginal<IR::MethodCallStatement>();
    LOG2("Visiting " << orig);
    if (replMap == nullptr) return statement;

    auto [callee, _] = get(*replMap, orig);
    if (callee == nullptr) return statement;

    LOG2("Inlining: " << callee);
    IR::IndexedVector<IR::StatOrDecl> body;
    ParameterSubstitution subst;
    TypeVariableSubstitution tvs;  // empty

    std::map<const IR::Parameter *, cstring> paramRename;
    ParameterSubstitution substitution;
    substitution.populate(callee->parameters, statement->methodCall->arguments);

    // evaluate in and inout parameters in order
    for (const auto *param : callee->parameters->parameters) {
        const auto *argument = substitution.lookup(param);
        cstring newName = nameGen->newName(param->name.name.string_view());
        paramRename.emplace(param, newName);
        if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
            const auto *vardecl = new IR::Declaration_Variable(newName, param->annotations,
                                                               param->type, argument->expression);
            body.push_back(vardecl);
            subst.add(param, new IR::Argument(argument->srcInfo, argument->name,
                                              new IR::PathExpression(newName)));
        } else if (param->direction == IR::Direction::None) {
            // This works because there can be no side-effects in the evaluation of this
            // argument.
            if (!argument) {
                ::P4C::error(ErrorType::ERR_UNINITIALIZED, "%1%: No argument supplied for %2%",
                             statement, param);
                continue;
            }
            subst.add(param, argument);
        } else if (param->direction == IR::Direction::Out) {
            // uninitialized variable
            const auto *vardecl =
                new IR::Declaration_Variable(newName, param->annotations, param->type);
            subst.add(param, new IR::Argument(argument->srcInfo, argument->name,
                                              new IR::PathExpression(newName)));
            body.push_back(vardecl);
        }
    }

    // FIXME: SubstituteParameters is a ResolutionContext, but we are recreating
    // it again and again killing lookup caches. We'd need to have instead some
    // separate DeclarationLookup that would allow us to perform necessary
    // lookups in the context of callee. We can probably pre-seed it first for
    // all possible callees in replMap.
    SubstituteParameters sp(nullptr, &subst, &tvs);
    sp.setCalledBy(this);
    auto clone = callee->apply(sp, getContext());
    if (::P4C::errorCount() > 0) return statement;
    CHECK_NULL(clone);
    BUG_CHECK(clone->is<IR::P4Action>(), "%1%: not an action", clone);
    auto actclone = clone->to<IR::P4Action>();
    body.append(actclone->body->components);

    // copy out and inout parameters
    for (auto param : callee->parameters->parameters) {
        auto left = substitution.lookup(param);
        if (param->direction == IR::Direction::InOut || param->direction == IR::Direction::Out) {
            cstring newName = ::P4C::get(paramRename, param);
            auto right = new IR::PathExpression(newName);
            auto copyout = new IR::AssignmentStatement(left->expression, right);
            body.push_back(copyout);
        }
    }

    auto annotations = callee->annotations->where([&](const IR::Annotation *a) {
        return !(a->name == IR::Annotation::nameAnnotation ||
                 (a->name == IR::Annotation::noWarnAnnotation && a->getSingleString() == "unused"));
    });
    auto result = new IR::BlockStatement(statement->srcInfo, annotations, body);
    LOG2("Replacing " << orig << " with " << result);
    return result;
}

}  // namespace P4C::P4
