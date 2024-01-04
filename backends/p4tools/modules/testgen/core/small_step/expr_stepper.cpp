#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"

#include <cstddef>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/gen_eq.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/irutils.h"
#include "ir/node.h"
#include "ir/solver.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "lib/source_file.h"
#include "midend/coverage.h"
#include "midend/saturationElim.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/abstract_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/lib/collect_coverable_nodes.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

ExprStepper::ExprStepper(ExecutionState &state, AbstractSolver &solver,
                         const ProgramInfo &programInfo)
    : AbstractStepper(state, solver, programInfo) {}

bool ExprStepper::preorder(const IR::BoolLiteral *boolLiteral) {
    logStep(boolLiteral);
    return stepSymbolicValue(boolLiteral);
}

bool ExprStepper::preorder(const IR::Constant *constant) {
    logStep(constant);
    return stepSymbolicValue(constant);
}

void ExprStepper::handleHitMissActionRun(const IR::Member *member) {
    auto &nextState = state.clone();
    std::vector<Continuation::Command> replacements;
    const auto *method = member->expr->to<IR::MethodCallExpression>();
    BUG_CHECK(method->method->is<IR::Member>(), "Method apply has unexpected format: %1%", method);
    replacements.emplace_back(new IR::MethodCallStatement(Util::SourceInfo(), method));
    const auto *methodName = method->method->to<IR::Member>();
    const auto *table = state.findTable(methodName);
    CHECK_NULL(table);
    if (member->member.name == IR::Type_Table::hit) {
        replacements.emplace_back(Continuation::Return(TableStepper::getTableHitVar(table)));
    } else if (member->member.name == IR::Type_Table::miss) {
        replacements.emplace_back(
            Continuation::Return(new IR::LNot(TableStepper::getTableHitVar(table))));
    } else {
        replacements.emplace_back(Continuation::Return(TableStepper::getTableActionVar(table)));
    }
    nextState.replaceTopBody(&replacements);
    result->emplace_back(nextState);
}

bool ExprStepper::preorder(const IR::Member *member) {
    logStep(member);

    if (member->expr->is<IR::MethodCallExpression>() &&
        (member->member.name == IR::Type_Table::hit ||
         member->member.name == IR::Type_Table::miss ||
         member->member.name == IR::Type_Table::action_run)) {
        handleHitMissActionRun(member);
        return false;
    }

    return stepSymbolicValue(state.get(member));
}

bool ExprStepper::preorder(const IR::ArrayIndex *arrIndex) {
    if (!SymbolicEnv::isSymbolicValue(arrIndex->right)) {
        return stepToSubexpr(arrIndex->right, result, state,
                             [arrIndex](const Continuation::Parameter *v) {
                                 auto *result = arrIndex->clone();
                                 result->right = v->param;
                                 return Continuation::Return(result);
                             });
    }
    return stepSymbolicValue(state.get(arrIndex));
}

void ExprStepper::evalActionCall(const IR::P4Action *action, const IR::MethodCallExpression *call) {
    state.markVisited(action);
    const auto *actionNameSpace = action->to<IR::INamespace>();
    BUG_CHECK(actionNameSpace, "Does not instantiate an INamespace: %1%", actionNameSpace);
    // If the action has arguments, these are usually directionless control plane input.
    // We introduce a symbolic variable that takes the argument value. This value is either
    // provided by a constant entry or synthesized by us.
    for (size_t argIdx = 0; argIdx < call->arguments->size(); ++argIdx) {
        const auto &parameters = action->parameters;
        const auto *param = parameters->getParameter(argIdx);
        const auto *paramType = state.resolveType(param->type);
        // We do not use the control plane name here. We assume that UniqueParameters and
        // UniqueNames ensure uniqueness of parameter names.
        const auto paramName = param->name;
        BUG_CHECK(param->direction == IR::Direction::None,
                  "%1%: Only directionless action parameters are supported at this point. ",
                  action);
        const auto *tableActionDataVar = new IR::PathExpression(paramType, new IR::Path(paramName));
        const auto *curArg = call->arguments->at(argIdx)->expression;
        state.set(tableActionDataVar, curArg);
    }
    state.replaceTopBody(action->body);
    // Enter the action's namespace.
    state.pushNamespace(actionNameSpace);
    result->emplace_back(state);
}

bool ExprStepper::resolveMethodCallArguments(const IR::MethodCallExpression *call) {
    IR::Vector<IR::Argument> resolvedArgs;
    const auto *method = call->method->type->checkedTo<IR::Type_MethodBase>();
    const auto &methodParams = method->parameters->parameters;
    const auto *callArguments = call->arguments;
    for (size_t idx = 0; idx < callArguments->size(); ++idx) {
        const auto *arg = callArguments->at(idx);
        const auto *param = methodParams.at(idx);
        const auto *argExpr = arg->expression;
        if (param->direction == IR::Direction::Out || SymbolicEnv::isSymbolicValue(argExpr)) {
            continue;
        }
        // If the parameter is not an out parameter (meaning we do not care about its content) and
        // the argument is not yet symbolic, try to resolve it.
        return stepToSubexpr(
            argExpr, result, state, [call, idx, param](const Continuation::Parameter *v) {
                // TODO: It seems expensive to copy the function every time we resolve an argument.
                // We should do this all at once. But how?
                // This is the same problem as in stepToListSubexpr
                // Thankfully, most method calls have less than 10 arguments.
                auto *clonedCall = call->clone();
                auto *arguments = clonedCall->arguments->clone();
                auto *arg = arguments->at(idx)->clone();
                const IR::Expression *computedExpr = v->param;
                // A parameter with direction InOut might be read and also written to.
                // We capture this ambiguity with an InOutReference.
                if (param->direction == IR::Direction::InOut) {
                    auto stateVar = ToolsVariables::convertReference(arg->expression);
                    computedExpr = new IR::InOutReference(stateVar, computedExpr);
                }
                arg->expression = computedExpr;
                (*arguments)[idx] = arg;
                clonedCall->arguments = arguments;
                return Continuation::Return(clonedCall);
            });
    }
    return true;
}

bool ExprStepper::preorder(const IR::MethodCallExpression *call) {
    logStep(call);
    // A method call expression represents an invocation of an action, a table, an extern, or
    // setValid/setInvalid.

    if (!resolveMethodCallArguments(call)) {
        return false;
    }

    // Handle method calls. These are either table invocations or extern calls.
    if (call->method->type->is<IR::Type_Method>()) {
        if (const auto *path = call->method->to<IR::PathExpression>()) {
            // Case where call->method is a PathExpression expression.
            const auto *member = new IR::Member(
                call->method->type,
                new IR::PathExpression(new IR::Type_Extern("*method"), new IR::Path("*method")),
                path->path->name);
            evalExternMethodCall(call, member->expr, member->member, call->arguments, state);
            return false;
        }

        if (const auto *method = call->method->to<IR::Member>()) {
            // Case where call->method is a Member expression. For table invocations, the
            // qualifier of the member determines the table being invoked. For extern calls,
            // the qualifier determines the extern object containing the method being invoked.
            BUG_CHECK(method->expr, "Method call has unexpected format: %1%", call);

            // Handle table calls.
            if (const auto *table = state.findTable(method)) {
                state.replaceTopBody(Continuation::Return(table));
                result->emplace_back(state);
                return false;
            }

            // Handle extern calls. They may also be of Type_SpecializedCanonical.
            if (method->expr->type->is<IR::Type_Extern>() ||
                method->expr->type->is<IR::Type_SpecializedCanonical>()) {
                evalExternMethodCall(call, method->expr, method->member, call->arguments, state);
                return false;
            }

            // Handle calls to header methods.
            if (method->expr->type->is<IR::Type_Header>() ||
                method->expr->type->is<IR::Type_HeaderUnion>()) {
                auto ref = ToolsVariables::convertReference(method->expr);
                if (method->member == "isValid") {
                    return stepGetHeaderValidity(ref);
                }

                if (method->member == "setInvalid") {
                    return stepSetHeaderValidity(ref, false);
                }

                if (method->member == "setValid") {
                    return stepSetHeaderValidity(ref, true);
                }

                BUG("Unknown method call on header instance: %1%", call);
            }

            if (method->expr->type->is<IR::Type_Stack>()) {
                if (method->member == "push_front") {
                    return stepStackPushPopFront(method->expr, call->arguments);
                }

                if (method->member == "pop_front") {
                    return stepStackPushPopFront(method->expr, call->arguments, false);
                }

                BUG("Unknown method call on stack instance: %1%", call);
            }

            BUG("Unknown method member expression: %1% of type %2%", method->expr,
                method->expr->type);
        }

        BUG("Unknown method call: %1% of type %2%", call->method, call->method->node_type_name());
    } else if (call->method->type->is<IR::Type_Action>()) {
        // Handle action calls. Actions are called by tables and are not inlined, unlike
        // functions.
        const auto *actionType = state.getP4Action(call);
        evalActionCall(actionType, call);
        return false;
    }

    BUG("Unknown method call expression: %1%", call);
}

bool ExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    TableStepper tableStepper(this, table);

    return tableStepper.eval();
}

bool ExprStepper::preorder(const IR::Mux *mux) {
    logStep(mux);

    if (!SymbolicEnv::isSymbolicValue(mux->e0)) {
        return stepToSubexpr(mux->e0, result, state, [mux](const Continuation::Parameter *v) {
            auto *result = mux->clone();
            result->e0 = v->param;
            return Continuation::Return(result);
        });
    }
    // If the Mux condition  is tainted, just return a taint constant.
    if (Taint::hasTaint(mux->e0)) {
        state.replaceTopBody(
            Continuation::Return(programInfo.createTargetUninitialized(mux->type, true)));
        result->emplace_back(state);
        return false;
    }
    // A list of path conditions paired with the resulting expression for each branch.
    std::list<std::pair<const IR::Expression *, const IR::Expression *>> branches = {
        {mux->e0, mux->e1},
        {new IR::LNot(mux->e0->type, mux->e0), mux->e2},
    };

    for (const auto &entry : branches) {
        const auto *cond = entry.first;
        const auto *expr = entry.second;

        auto &nextState = state.clone();
        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(state);
            collector.updateNodeCoverage(expr, coveredNodes);
        }
        nextState.replaceTopBody(Continuation::Return(expr));
        result->emplace_back(cond, state, nextState, coveredNodes);
    }

    return false;
}

bool ExprStepper::preorder(const IR::PathExpression *pathExpression) {
    logStep(pathExpression);
    // If the path expression is a Type_MatchKind, convert it to a StringLiteral.
    if (pathExpression->type->is<IR::Type_MatchKind>()) {
        state.replaceTopBody(Continuation::Return(
            new IR::StringLiteral(IR::Type_MatchKind::get(), pathExpression->path->name)));
        result->emplace_back(state);
        return false;
    }
    // Otherwise convert the path expression into a qualified member and return it.
    state.replaceTopBody(Continuation::Return(state.get(pathExpression)));
    result->emplace_back(state);
    return false;
}

bool ExprStepper::preorder(const IR::P4ValueSet *valueSet) {
    logStep(valueSet);

    auto vsSize = valueSet->size->checkedTo<IR::Constant>()->value;
    IR::Vector<IR::Expression> components;
    // TODO: Fill components with values when we have an API.
    const auto *pvsType = valueSet->elementType;
    pvsType = state.resolveType(pvsType);
    TESTGEN_UNIMPLEMENTED("Value Set not yet fully implemented");

    state.popBody();
    result->emplace_back(state);
    return false;
}

bool ExprStepper::preorder(const IR::Operation_Binary *binary) {
    logStep(binary);

    if (!SymbolicEnv::isSymbolicValue(binary->left)) {
        return stepToSubexpr(binary->left, result, state,
                             [binary](const Continuation::Parameter *v) {
                                 auto *result = binary->clone();
                                 result->left = v->param;
                                 return Continuation::Return(result);
                             });
    }

    if (!SymbolicEnv::isSymbolicValue(binary->right)) {
        return stepToSubexpr(binary->right, result, state,
                             [binary](const Continuation::Parameter *v) {
                                 auto *result = binary->clone();
                                 result->right = v->param;
                                 return Continuation::Return(result);
                             });
    }

    // Handle saturating arithmetic expressions by translating them into Mux expressions.
    if (P4::SaturationElim::isSaturationOperation(binary)) {
        state.replaceTopBody(Continuation::Return(P4::SaturationElim::eliminate(binary)));
        result->emplace_back(state);
        return false;
    }

    return stepSymbolicValue(binary);
}

bool ExprStepper::preorder(const IR::Operation_Unary *unary) {
    logStep(unary);

    if (!SymbolicEnv::isSymbolicValue(unary->expr)) {
        return stepToSubexpr(unary->expr, result, state, [unary](const Continuation::Parameter *v) {
            auto *result = unary->clone();
            result->expr = v->param;
            return Continuation::Return(result);
        });
    }

    return stepSymbolicValue(unary);
}

bool ExprStepper::preorder(const IR::SelectExpression *selectExpression) {
    logStep(selectExpression);

    auto selectCases = selectExpression->selectCases;
    // If there are no select cases, then the select expression has failed to match on anything.
    // Delegate to stepNoMatch.
    if (selectCases.empty()) {
        stepNoMatch("Parser select expression has no alternatives.");
        return false;
    }
    if (!SymbolicEnv::isSymbolicValue(selectExpression->select)) {
        // Evaluate the expression being selected.
        return stepToListSubexpr(selectExpression->select, result, state,
                                 [selectExpression](const IR::BaseListExpression *listExpr) {
                                     auto *result = selectExpression->clone();
                                     result->select = listExpr->checkedTo<IR::ListExpression>();
                                     return Continuation::Return(result);
                                 });
    }

    for (size_t idx = 0; idx < selectCases.size(); ++idx) {
        const auto *selectCase = selectCases.at(idx);
        // Getting P4ValueSet from PathExpression , to highlight a particular case of processing
        // P4ValueSet.
        if (const auto *pathExpr = selectCase->keyset->to<IR::PathExpression>()) {
            const auto *decl = state.findDecl(pathExpr)->getNode();
            // Until we have test support for value set, we simply remove them from the match list.
            // TODO: Implement value sets.
            if (decl->is<IR::P4ValueSet>()) {
                selectCases.erase(selectCases.begin() + idx);
                // Since we erased an element from the vector, we need to reset the index.
                idx = idx - 1;
                continue;
            }
        }
        if (!SymbolicEnv::isSymbolicValue(selectCase->keyset)) {
            // Evaluate the keyset in the first select case.
            return stepToSubexpr(
                selectCase->keyset, result, state,
                [selectExpression, selectCase, &idx](const Continuation::Parameter *v) {
                    auto *newSelectCase = selectCase->clone();
                    newSelectCase->keyset = v->param;

                    auto *result = selectExpression->clone();
                    result->selectCases[idx] = newSelectCase;
                    return Continuation::Return(result);
                });
        }
    }

    const IR::Expression *missCondition = IR::getBoolLiteral(true);
    bool hasDefault = false;
    for (const auto *selectCase : selectCases) {
        auto &nextState = state.clone();

        // Handle case where the first select case matches: proceed to the next parser state,
        // guarded by its path condition.
        const auto *matchCondition = GenEq::equate(selectExpression->select, selectCase->keyset);
        hasDefault = hasDefault || selectCase->keyset->is<IR::DefaultExpression>();

        // TODO: Implement the taint case for select expressions.
        // In the worst case, this means the entire parser is tainted.
        if (Taint::hasTaint(matchCondition)) {
            TESTGEN_UNIMPLEMENTED(
                "The SelectExpression %1% is trying to match on a tainted key set."
                " This means it is matching on uninitialized data."
                " P4Testgen currently does not support this case.",
                selectExpression);
        }

        const auto *decl = state.findDecl(selectCase->state)->getNode();
        nextState.replaceTopBody(Continuation::Return(decl));
        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(state);
            collector.updateNodeCoverage(decl, coveredNodes);
        }
        result->emplace_back(new IR::LAnd(missCondition, matchCondition), state, nextState,
                             coveredNodes);
        missCondition = new IR::LAnd(new IR::LNot(matchCondition), missCondition);
    }

    // Generate implicit NoMatch.
    if (!hasDefault) {
        stepNoMatch("Parser select expression did not match any alternatives.", missCondition);
    }

    return false;
}

bool ExprStepper::preorder(const IR::BaseListExpression *listExpression) {
    logStep(listExpression);

    if (!SymbolicEnv::isSymbolicValue(listExpression)) {
        // Evaluate the expression being selected.
        return stepToListSubexpr(
            listExpression, result, state,
            [](const IR::BaseListExpression *listExpr) { return Continuation::Return(listExpr); });
    }
    return stepSymbolicValue(listExpression);
}

bool ExprStepper::preorder(const IR::StructExpression *structExpression) {
    logStep(structExpression);

    if (!SymbolicEnv::isSymbolicValue(structExpression)) {
        // Evaluate the expression being selected.
        return stepToStructSubexpr(structExpression, result, state,
                                   [](const IR::StructExpression *structExpr) {
                                       return Continuation::Return(structExpr);
                                   });
    }
    return stepSymbolicValue(structExpression);
}

bool ExprStepper::preorder(const IR::Slice *slice) {
    logStep(slice);

    if (!SymbolicEnv::isSymbolicValue(slice->e0)) {
        return stepToSubexpr(slice->e0, result, state, [slice](const Continuation::Parameter *v) {
            auto *result = slice->clone();
            result->e0 = v->param;
            return Continuation::Return(result);
        });
    }

    // Check that slices values are constants.
    slice->e1->checkedTo<IR::Constant>();
    slice->e2->checkedTo<IR::Constant>();

    return stepSymbolicValue(slice);
}

void ExprStepper::stepNoMatch(std::string traceLog, const IR::Expression *condition) {
    auto &noMatchState = condition ? state.clone() : state;
    noMatchState.add(*new TraceEvents::GenericDescription("NoMatch", traceLog));
    noMatchState.replaceTopBody(Continuation::Exception::NoMatch);
    if (condition) {
        result->emplace_back(condition, state, noMatchState);
    } else {
        result->emplace_back(state);
    }
}

}  // namespace P4Tools::P4Testgen
