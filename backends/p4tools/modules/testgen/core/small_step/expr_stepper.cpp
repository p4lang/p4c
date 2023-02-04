#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"

#include <cstddef>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/declaration.h"
#include "ir/irutils.h"
#include "ir/node.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/safe_vector.h"
#include "midend/saturationElim.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/abstract_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/gen_eq.h"

namespace P4Tools {

namespace P4Testgen {

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
    auto *nextState = new ExecutionState(state);
    std::vector<Continuation::Command> replacements;
    const auto *method = member->expr->to<IR::MethodCallExpression>();
    BUG_CHECK(method->method->is<IR::Member>(), "Method apply has unexpected format: %1%", method);
    replacements.emplace_back(new IR::MethodCallStatement(Util::SourceInfo(), method));
    const auto *methodName = method->method->to<IR::Member>();
    const auto *table = state.getTableType(methodName);
    CHECK_NULL(table);
    if (member->member.name == IR::Type_Table::hit) {
        replacements.emplace_back(Continuation::Return(TableStepper::getTableHitVar(table)));
    } else if (member->member.name == IR::Type_Table::miss) {
        replacements.emplace_back(
            Continuation::Return(new IR::LNot(TableStepper::getTableHitVar(table))));
    } else {
        replacements.emplace_back(Continuation::Return(TableStepper::getTableActionVar(table)));
    }
    nextState->replaceTopBody(&replacements);
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

    // TODO: Do we need to handle non-numeric, non-boolean expressions?
    BUG_CHECK(member->type->is<IR::Type::Bits>() || member->type->is<IR::Type::Boolean>() ||
                  member->type->is<IR::Extracted_Varbits>(),
              "Non-numeric, non-boolean member expression: %1% Type: %2%", member,
              member->type->node_type_name());

    // Check our assumption that this is a chain of member expressions terminating in a
    // PathExpression.
    checkMemberInvariant(member);

    // This case may happen when we directly assign taint, that is not bound in the symbolic
    // environment to an expression.
    if (SymbolicEnv::isSymbolicValue(member)) {
        return stepSymbolicValue(member);
    }
    return stepSymbolicValue(state.get(member));
}

void ExprStepper::evalActionCall(const IR::P4Action *action, const IR::MethodCallExpression *call) {
    const auto *actionNameSpace = action->to<IR::INamespace>();
    BUG_CHECK(actionNameSpace, "Does not instantiate an INamespace: %1%", actionNameSpace);
    auto *nextState = new ExecutionState(state);
    // If the action has arguments, these are usually directionless control plane input.
    // We introduce a zombie variable that takes the argument value. This value is either
    // provided by a constant entry or synthesized by us.
    for (size_t argIdx = 0; argIdx < call->arguments->size(); ++argIdx) {
        const auto &parameters = action->parameters;
        const auto *param = parameters->getParameter(argIdx);
        const auto *paramType = param->type;
        const auto paramName = param->name;
        BUG_CHECK(param->direction == IR::Direction::None,
                  "%1%: Only directionless action parameters are supported at this point. ",
                  action);
        const auto &tableActionDataVar = Utils::getZombieVar(paramType, 0, paramName);
        const auto *curArg = call->arguments->at(argIdx)->expression;
        nextState->set(tableActionDataVar, curArg);
    }
    nextState->replaceTopBody(action->body);
    // Enter the action's namespace.
    nextState->pushNamespace(actionNameSpace);
    result->emplace_back(nextState);
}

bool ExprStepper::preorder(const IR::MethodCallExpression *call) {
    logStep(call);
    // A method call expression represents an invocation of an action, a table, an extern, or
    // setValid/setInvalid.

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
            if (const auto *table = state.getTableType(method)) {
                auto *nextState = new ExecutionState(state);
                nextState->replaceTopBody(Continuation::Return(table));
                result->emplace_back(nextState);
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
                if (method->member == "isValid") {
                    return stepGetHeaderValidity(method->expr);
                }

                if (method->member == "setInvalid") {
                    return stepSetHeaderValidity(method->expr, false);
                }

                if (method->member == "setValid") {
                    return stepSetHeaderValidity(method->expr, true);
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
        // Handle action calls. Actions are called by tables and are not inlined, unlike
        // functions.
    } else if (const auto *action = state.getActionDecl(call->method)) {
        evalActionCall(action, call);
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
    if (state.hasTaint(mux->e0)) {
        auto *nextState = new ExecutionState(state);
        nextState->replaceTopBody(
            Continuation::Return(programInfo.createTargetUninitialized(mux->type, true)));
        result->emplace_back(nextState);
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

        auto *nextState = new ExecutionState(state);
        P4::Coverage::CoverageSet coveredStmts;
        expr->apply(CollectStatements2(coveredStmts, state));
        nextState->replaceTopBody(Continuation::Return(expr));
        result->emplace_back(cond, state, nextState, coveredStmts);
    }

    return false;
}

bool ExprStepper::preorder(const IR::PathExpression *pathExpression) {
    logStep(pathExpression);

    // If we are referencing a parser state, step into the state.
    const auto *decl = state.findDecl(pathExpression)->getNode();
    if (decl->is<IR::ParserState>()) {
        return stepSymbolicValue(decl);
    }
    auto *nextState = new ExecutionState(state);
    // ValueSets can be declared in parsers and are usually set by the control plane.
    // We simply return the contained valueSet.
    if (const auto *valueSet = decl->to<IR::P4ValueSet>()) {
        nextState->replaceTopBody(Continuation::Return(valueSet));
        result->emplace_back(nextState);
        return false;
    }
    // Otherwise convert the path expression into a qualified member and return it.
    auto pathRef = nextState->convertPathExpr(pathExpression);
    nextState->replaceTopBody(Continuation::Return(pathRef));
    result->emplace_back(nextState);
    return false;
}

bool ExprStepper::preorder(const IR::P4ValueSet *valueSet) {
    logStep(valueSet);

    auto vsSize = valueSet->size->checkedTo<IR::Constant>()->value;
    auto *nextState = new ExecutionState(state);
    IR::Vector<IR::Expression> components;
    // TODO: Fill components with values when we have an API.
    const auto *pvsType = valueSet->elementType;
    pvsType = state.resolveType(pvsType);
    TESTGEN_UNIMPLEMENTED("Value Set not yet fully implemented");

    nextState->popBody();
    result->emplace_back(nextState);
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
        auto *nextState = new ExecutionState(state);
        nextState->replaceTopBody(Continuation::Return(P4::SaturationElim::eliminate(binary)));
        result->emplace_back(nextState);
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
        stepNoMatch();
        return false;
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
    for (const auto *selectCase : selectCases) {
        auto *nextState = new ExecutionState(state);

        if (!SymbolicEnv::isSymbolicValue(selectExpression->select)) {
            // Evaluate the expression being selected.
            return stepToListSubexpr(selectExpression->select, result, state,
                                     [selectExpression](const IR::ListExpression *listExpr) {
                                         auto *result = selectExpression->clone();
                                         result->select = listExpr;
                                         return Continuation::Return(result);
                                     });
        }

        // Handle case where the first select case matches: proceed to the next parser state,
        // guarded by its path condition.
        const auto *matchCondition = GenEq::equate(selectExpression->select, selectCase->keyset);

        // TODO: Implement the taint case for select expressions.
        // In the worst case, this means the entire parser is tainted.
        if (state.hasTaint(matchCondition)) {
            TESTGEN_UNIMPLEMENTED(
                "The SelectExpression %1% is trying to match on a tainted key set."
                " This means it is matching on uninitialized data."
                " P4Testgen currently does not support this case.",
                selectExpression);
        }
        const auto *decl = state.findDecl(selectCase->state)->getNode();

        nextState->replaceTopBody(Continuation::Return(selectCase->state));
        P4::Coverage::CoverageSet coveredStmts;
        decl->apply(CollectStatements2(coveredStmts, state));
        result->emplace_back(new IR::LAnd(missCondition, matchCondition), state, nextState,
                             coveredStmts);
        missCondition = new IR::LAnd(new IR::LNot(matchCondition), missCondition);
    }

    return false;
}

bool ExprStepper::preorder(const IR::ListExpression *listExpression) {
    if (!SymbolicEnv::isSymbolicValue(listExpression)) {
        // Evaluate the expression being selected.
        return stepToListSubexpr(listExpression, result, state,
                                 [listExpression](const IR::ListExpression *listExpr) {
                                     const auto *result = listExpression->clone();
                                     result = listExpr;
                                     return Continuation::Return(result);
                                 });
    }
    return stepSymbolicValue(listExpression);
}

bool ExprStepper::preorder(const IR::StructExpression *structExpression) {
    if (!SymbolicEnv::isSymbolicValue(structExpression)) {
        // Evaluate the expression being selected.
        return stepToStructSubexpr(structExpression, result, state,
                                   [structExpression](const IR::StructExpression *structExpr) {
                                       const auto *result = structExpression->clone();
                                       result = structExpr;
                                       return Continuation::Return(result);
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

void ExprStepper::stepNoMatch() { stepToException(Continuation::Exception::NoMatch); }

}  // namespace P4Testgen

}  // namespace P4Tools
