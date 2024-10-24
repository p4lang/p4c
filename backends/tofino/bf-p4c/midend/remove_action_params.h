/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * \defgroup RemoveActionParameters BFN::RemoveActionParameters
 * \ingroup midend
 * \brief Set of passes that specialize the p4c/frontends/RemoveActionParameters class.
 *
 * This moves action parameters out of the actions.
 */
#ifndef BF_P4C_MIDEND_REMOVE_ACTION_PARAMS_H_
#define BF_P4C_MIDEND_REMOVE_ACTION_PARAMS_H_

#include "frontends/p4/removeParameters.h"

namespace BFN {

/**
 * \ingroup RemoveActionParameters
 */
class DoRemoveActionParametersTofino : public P4::DoRemoveActionParameters {
 public:
    explicit DoRemoveActionParametersTofino(P4::ActionInvocation *inv)
        : P4::DoRemoveActionParameters(inv) {}

    /**
     * Check if there is a mark_to_drop action and don't replace parameters.
     */
    bool DoCheckReplaceParam(IR::P4Action *action, const IR::Parameter *p) {
        int paramUses = 0;
        int num_mark_to_drop = 0;
        bool replace = true;
        for (auto abc : action->body->components) {
            /* iterate thought all statements, and find the uses of smeta */
            /* if the only use is in mark_to_drop -- that's not a use! */
            if (abc->is<IR::AssignmentStatement>()) {
                auto as = abc->to<IR::AssignmentStatement>();
                // TODO: Handle assignment statements with expressions
                if (p->name == as->left->toString())
                    paramUses++;
                else if (p->name == as->right->toString())
                    paramUses++;
            } else if (abc->is<IR::MethodCallStatement>()) {
                auto mcs = abc->to<IR::MethodCallStatement>();
                auto mc = mcs->methodCall;
                auto mce = mc->to<IR::MethodCallExpression>();
                if ("mark_to_drop" == mce->method->toString()) {
                    num_mark_to_drop++;
                } else {
                    for (auto arg : *(mce->arguments)) {
                        auto *argMem = arg->expression->to<IR::Member>();
                        if (argMem && p->name == argMem->toString()) paramUses++;
                    }
                }
            } else {
                // TODO: Handle block statements
                LOG3("not handling statement: " << abc);
            }
        }
        if (num_mark_to_drop > 0 && paramUses == 0) {
            replace = false;
        }
        return replace;
    }

    const IR::Node *postorder(IR::P4Action *action) {
        LOG1("Visiting " << dbp(action));
        BUG_CHECK(getParent<IR::P4Control>() || getParent<IR::P4Program>(),
                  "%1%: unexpected parent %2%", getOriginal(), getContext()->node);
        auto result = new IR::IndexedVector<IR::Declaration>();
        auto leftParams = new IR::IndexedVector<IR::Parameter>();
        auto initializers = new IR::IndexedVector<IR::StatOrDecl>();
        auto postamble = new IR::IndexedVector<IR::StatOrDecl>();
        auto invocation = getInvocations()->get(getOriginal<IR::P4Action>());
        if (invocation == nullptr) return action;
        auto args = invocation->arguments;

        P4::ParameterSubstitution substitution;
        substitution.populate(action->parameters, args);

        bool removeAll = getInvocations()->removeAllParameters(getOriginal<IR::P4Action>());
        int numParamsNotReplaced = 0;
        for (auto p : action->parameters->parameters) {
            bool replaceParam = DoCheckReplaceParam(action, p);
            if (p->direction == IR::Direction::None && !removeAll) {
                leftParams->push_back(p);
            } else {
                if (replaceParam) {
                    auto decl = new IR::Declaration_Variable(p->srcInfo, p->name, p->annotations,
                                                             p->type, nullptr);
                    LOG3("Added declaration " << decl << " annotations " << p->annotations);
                    result->push_back(decl);

                    auto arg = substitution.lookup(p);
                    if (arg == nullptr) {
                        error("action %1%: parameter %2% must be bound", invocation, p);
                        continue;
                    }

                    if (p->direction == IR::Direction::In || p->direction == IR::Direction::InOut ||
                        p->direction == IR::Direction::None) {
                        auto left = new IR::PathExpression(p->name);
                        auto assign =
                            new IR::AssignmentStatement(arg->srcInfo, left, arg->expression);
                        initializers->push_back(assign);
                    }

                    if (p->direction == IR::Direction::Out ||
                        p->direction == IR::Direction::InOut) {
                        auto right = new IR::PathExpression(p->name);
                        auto assign =
                            new IR::AssignmentStatement(arg->srcInfo, arg->expression, right);
                        postamble->push_back(assign);
                    }
                } else {
                    numParamsNotReplaced++;
                }
            }
        }

        if (result->empty()) {
            if (numParamsNotReplaced > 0) {
                for (auto abc : action->body->components) {
                    if (abc->is<IR::MethodCallStatement>()) {
                        auto mcs = abc->to<IR::MethodCallStatement>();
                        auto mc = mcs->methodCall;
                        auto mce = mc->to<IR::MethodCallExpression>();
                        // Add mark_to_drop action component with standard_metadata arg
                        if ("mark_to_drop" == mce->method->toString()) {
                            auto new_mce = new IR::MethodCallExpression(mce->method, args);
                            abc = new IR::MethodCallStatement(new_mce);
                        }
                    }
                    initializers->push_back(abc);
                }
            } else {
                return action;
            }
        } else {
            LOG1("To replace " << dbp(action));
            initializers->append(action->body->components);
            initializers->append(*postamble);
        }

        action->parameters = new IR::ParameterList(action->parameters->srcInfo, *leftParams);
        action->body = new IR::BlockStatement(action->body->srcInfo, *initializers);
        result->push_back(action);
        return result;
    }
};

/**
 * \ingroup RemoveActionParameters
 * \brief Top level PassManager that governs moving of action parameters out of the actions
 *        (specialized for BFN).
 */
class RemoveActionParameters : public PassManager {
 public:
    RemoveActionParameters(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                           P4::TypeChecking *tc = nullptr) {
        setName("RemoveActionParameters");
        auto ai = new P4::ActionInvocation();
        // MoveDeclarations() is needed because of this case:
        // action a(inout x) { x = x + 1 }
        // bit<32> w;
        // table t() { actions = a(w); ... }
        // Without the MoveDeclarations the code would become
        // action a() { x = w; x = x + 1; w = x; } << w is not yet defined
        // bit<32> w;
        // table t() { actions = a(); ... }
        passes.emplace_back(new P4::MoveDeclarations());
        if (!tc) tc = new P4::TypeChecking(refMap, typeMap);
        passes.emplace_back(tc);
        passes.emplace_back(new P4::FindActionParameters(typeMap, ai));
        passes.emplace_back(new DoRemoveActionParametersTofino(ai));
        passes.emplace_back(new P4::ClearTypeMap(typeMap));
    }
};

}  // end namespace BFN

#endif /* BF_P4C_MIDEND_REMOVE_ACTION_PARAMS_H_ */
