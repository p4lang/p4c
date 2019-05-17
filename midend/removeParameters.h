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

#ifndef _MIDEND_REMOVEPARAMETERS_H_
#define _MIDEND_REMOVEPARAMETERS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

/**
 * For each action that is invoked keep the list of arguments that
 * it's called with. There must be only one call of each action;
 * this is done by LocalizeActions.
 */
class ActionInvocation {
    std::map<const IR::P4Action*, const IR::MethodCallExpression*> invocations;
    std::set<const IR::P4Action*> all;  // for these actions remove all parameters
    std::set<const IR::MethodCallExpression*> calls;
    /// how many arguments to remove from each default action
    std::map<const IR::MethodCallExpression*, unsigned> defaultActions;

 public:
    void bind(const IR::P4Action* action, const IR::MethodCallExpression* invocation,
              bool allParams) {
        CHECK_NULL(action);
        BUG_CHECK(invocations.find(action) == invocations.end(),
                  "%1%: action called twice", action);
        invocations.emplace(action, invocation);
        if (allParams)
            all.emplace(action);
        calls.emplace(invocation);
    }
    void bindDefaultAction(const IR::P4Action* action,
                           const IR::MethodCallExpression* defaultInvocation) {
        // We must have a binding for this action already.
        auto actionCallInvocation = ::get(invocations, action);
        CHECK_NULL(actionCallInvocation);
        // We must remove all arguments which are bound in the action list.
        unsigned boundArgs = actionCallInvocation->arguments->size();
        defaultActions.emplace(defaultInvocation, boundArgs);
    }
    const IR::MethodCallExpression* get(const IR::P4Action* action) const {
        return ::get(invocations, action);
    }
    bool removeAllParameters(const IR::P4Action* action) const {
        return all.find(action) != all.end();
    }
    bool isCall(const IR::MethodCallExpression* expression) const {
        return calls.find(expression) != calls.end();
    }
    unsigned argsToRemove(const IR::MethodCallExpression* defaultCall) const {
        if (defaultActions.find(defaultCall) == defaultActions.end())
            return 0;
        return ::get(defaultActions, defaultCall);
    }
};

class FindActionParameters : public Inspector {
    ReferenceMap*     refMap;
    TypeMap*          typeMap;
    ActionInvocation* invocations;
 public:
    FindActionParameters(ReferenceMap* refMap,
                         TypeMap* typeMap, ActionInvocation* invocations) :
            refMap(refMap), typeMap(typeMap), invocations(invocations) {
        CHECK_NULL(refMap); CHECK_NULL(invocations); CHECK_NULL(typeMap);
        setName("FindActionParameters"); }

    void postorder(const IR::ActionListElement* element) override;
    void postorder(const IR::MethodCallExpression* expression) override;
};

/**
 * Removes parameters of an action which are in/inout/out.
 *
 * \code{.cpp}
 * control c(inout bit<32> x) {
 *    action a(in bit<32> arg) { x = arg; }
 *    table t() { actions = { a(10); } }
 *    apply { ... } }
 * \endcode
 *
 * is converted to
 *
 * \code{.cpp}
 * control c(inout bit<32> x) {
 *    bit<32> arg;
 *    action a() { arg = 10; x = arg; }
 *    table t() { actions = { a; } }
 *    apply { ... } }
 * \endcode
 *
 * @pre This pass requires each action to have a single caller.
 *      It must run after the LocalizeActions pass, which
 *      in turn must be run after actions inlining.
 * @post in/inout/out parameters of an action are removed.
 */
class DoRemoveActionParameters : public Transform {
    ActionInvocation* invocations;
 public:
    explicit DoRemoveActionParameters(ActionInvocation* invocations) :
            invocations(invocations)
    { CHECK_NULL(invocations); setName("DoRemoveActionParameters"); }

    const IR::Node* postorder(IR::P4Action* table) override;
    const IR::Node* postorder(IR::ActionListElement* element) override;
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    ActionInvocation* getInvocations() {
        return invocations;
    }
};

class RemoveActionParameters : public PassManager {
 public:
    RemoveActionParameters(ReferenceMap* refMap, TypeMap* typeMap,
                           TypeChecking* typeChecking = nullptr);
};

}  // namespace P4

#endif /* _MIDEND_REMOVEPARAMETERS_H_ */
