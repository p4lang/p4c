#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"

#include <cstddef>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/abstract_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/collect_coverable_nodes.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

CmdStepper::CmdStepper(ExecutionState &state, AbstractSolver &solver,
                       const ProgramInfo &programInfo)
    : AbstractStepper(state, solver, programInfo) {}

bool CmdStepper::preorder(const IR::AssignmentStatement *assign) {
    logStep(assign);

    if (!SymbolicEnv::isSymbolicValue(assign->right)) {
        return stepToSubexpr(assign->right, result, state,
                             [assign](const Continuation::Parameter *v) {
                                 auto *result = assign->clone();
                                 result->right = v->param;
                                 return result;
                             });
    }

    state.markVisited(assign);
    const auto &left = AbstractExecutionState::convertReference(assign->left);
    const auto *leftType = left->type;

    // Resolve the type of the left-and assignment, if it is a type name.
    leftType = state.resolveType(leftType);
    // Although we typically expand structure assignments into individual member assignments using
    // the copyHeaders pass, some extern functions may return a list or struct expression. We can
    // not always expand these return values as we do with the expandLookahead pass.
    // Correspondingly, we need to retrieve the fields and set each member individually. This
    // assumes that all headers and structures have been flattened and no nesting is left.
    if (const auto *structType = leftType->to<IR::Type_StructLike>()) {
        const auto *listExpr = assign->right->checkedTo<IR::ListExpression>();

        std::vector<IR::StateVariable> flatRefValids;
        auto flatRefFields = state.getFlatFields(left, structType, &flatRefValids);
        // First, complete the assignments for the data structure.
        for (size_t idx = 0; idx < flatRefFields.size(); ++idx) {
            const auto &leftFieldRef = flatRefFields[idx];
            state.set(leftFieldRef, listExpr->components[idx]);
        }
        // In case of a header, we also need to set the validity bits to true.
        for (const auto &headerValid : flatRefValids) {
            state.set(headerValid, IR::getBoolLiteral(true));
        }
    } else if (leftType->is<IR::Type_Base>()) {
        state.set(left, assign->right);
    } else {
        TESTGEN_UNIMPLEMENTED("Unsupported assign type %1% node: %2%", leftType,
                              leftType->node_type_name());
    }

    state.popBody();
    result->emplace_back(state);
    return false;
}

bool CmdStepper::preorder(const IR::P4Parser *p4parser) {
    logStep(p4parser);

    auto &nextState = state.clone();

    // Register and do target-specific initialization of the parser.
    const auto *constraint = startParser(p4parser, nextState);

    // Add trace events.
    nextState.add(*new TraceEvents::ParserStart(p4parser));

    // Remove the invocation of the parser from the current body and push the resulting
    // continuation onto the continuation stack.
    nextState.popBody();

    auto handlers = getExceptionHandlers(p4parser, nextState.getBody(), nextState);
    nextState.pushCurrentContinuation(handlers);

    // Obtain the parser's namespace.
    const auto *ns = p4parser->to<IR::INamespace>();
    CHECK_NULL(ns);
    // Enter the parser's namespace.
    nextState.pushNamespace(ns);

    // Set the start state as the new body.
    const auto *startState = p4parser->states.getDeclaration<IR::ParserState>("start");
    std::vector<Continuation::Command> cmds;

    // Initialize parser-local declarations.
    for (const auto *decl : p4parser->parserLocals) {
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            nextState.declareVariable(TestgenTarget::get(), *declVar);
        }
    }
    cmds.emplace_back(startState);
    nextState.replaceBody(Continuation::Body(cmds));

    result->emplace_back(constraint, state, nextState);
    return false;
}

bool CmdStepper::preorder(const IR::P4Control *p4control) {
    auto &nextState = state.clone();

    auto blockName = p4control->getName().name;

    // Add trace events.
    std::stringstream controlName;
    controlName << "Control " << blockName << " start";
    nextState.add(*new TraceEvents::Generic(controlName));

    // Set the emit buffer to be zero for the current control pipeline.
    nextState.resetEmitBuffer();

    // Obtain the control's namespace.
    const auto *ns = p4control->to<IR::INamespace>();
    CHECK_NULL(ns);
    // Enter the control's namespace.
    nextState.pushNamespace(ns);

    // Add control-local declarations.
    std::vector<Continuation::Command> cmds;
    for (const auto *decl : p4control->controlLocals) {
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            nextState.declareVariable(TestgenTarget::get(), *declVar);
        }
    }
    // Add the body, if it is not empty
    if (p4control->body != nullptr) {
        cmds.emplace_back(p4control->body);
    }
    // Remove the invocation of the control from the current body and push the resulting
    // continuation onto the continuation stack.
    nextState.popBody();
    // Exit terminates the entire control block (only the control).
    std::map<Continuation::Exception, Continuation> handlers;
    handlers.emplace(Continuation::Exception::Exit, Continuation::Body({}));
    nextState.pushCurrentContinuation(handlers);
    // If the cmds are not empty, replace the body
    if (!cmds.empty()) {
        Continuation::Body newBody(cmds);
        nextState.replaceBody(newBody);
    }

    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::EmptyStatement * /*empty*/) {
    auto &nextState = state.clone();
    nextState.popBody();
    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::IfStatement *ifStatement) {
    logStep(ifStatement);

    if (!SymbolicEnv::isSymbolicValue(ifStatement->condition)) {
        // Evaluate the condition.
        return stepToSubexpr(ifStatement->condition, result, state,
                             [ifStatement](const Continuation::Parameter *v) {
                                 auto *result = ifStatement->clone();
                                 result->condition = v->param;
                                 return result;
                             });
    }
    // If the condition of the if statement is tainted, the program counter is effectively tainted.
    // We insert a property that marks all execution as tainted until the if statement has
    // completed.
    // Because we may have nested taint, we need to check if we are already tainted.
    if (state.hasTaint(ifStatement->condition)) {
        auto &nextState = state.clone();
        std::vector<Continuation::Command> cmds;
        auto currentTaint = state.getProperty<bool>("inUndefinedState");
        nextState.add(*new TraceEvents::IfStatementCondition(ifStatement->condition));
        cmds.emplace_back(Continuation::PropertyUpdate("inUndefinedState", true));
        cmds.emplace_back(ifStatement->ifTrue);
        if (ifStatement->ifFalse != nullptr) {
            cmds.emplace_back(ifStatement->ifFalse);
        }
        cmds.emplace_back(Continuation::PropertyUpdate("inUndefinedState", currentTaint));
        nextState.replaceTopBody(&cmds);
        result->emplace_back(nextState);
        return false;
    }
    // Handle case where a condition is true: proceed to a body.
    {
        auto &nextState = state.clone();
        std::vector<Continuation::Command> cmds;
        nextState.add(*new TraceEvents::IfStatementCondition(ifStatement->condition));
        cmds.emplace_back(ifStatement->ifTrue);
        nextState.replaceTopBody(&cmds);

        // Some path selection strategies depend on looking ahead and collecting potential
        // statements. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(state);
            collector.updateNodeCoverage(ifStatement->ifTrue, coveredNodes);
        }
        result->emplace_back(ifStatement->condition, state, nextState, coveredNodes);
    }

    // Handle case for else body.
    {
        auto *negation = new IR::LNot(IR::Type::Boolean::get(), ifStatement->condition);
        auto &nextState = state.clone();
        nextState.add(*new TraceEvents::IfStatementCondition(ifStatement->condition));

        nextState.replaceTopBody((ifStatement->ifFalse == nullptr) ? new IR::BlockStatement()
                                                                   : ifStatement->ifFalse);
        // Some path selection strategies depend on looking ahead and collecting potential
        // statements. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(state);
            collector.updateNodeCoverage(ifStatement->ifFalse, coveredNodes);
        }
        result->emplace_back(negation, state, nextState, coveredNodes);
    }

    return false;
}

bool CmdStepper::preorder(const IR::MethodCallStatement *methodCallStatement) {
    logStep(methodCallStatement);
    state.markVisited(methodCallStatement);
    state.popBody();
    const auto *type = methodCallStatement->methodCall->type;
    // Table and action calls do not really return anything (for now).
    // TODO: We should not need these convoluted type checks in a method call statement.
    if (type->is<IR::Type_Action>() ||
        state.getTableType(methodCallStatement->methodCall->method) != nullptr) {
        type = IR::Type::Void::get();
    }
    state.pushCurrentContinuation(type);
    state.replaceBody(Continuation::Body({Continuation::Return(methodCallStatement->methodCall)}));
    result->emplace_back(state);
    return false;
}

bool CmdStepper::preorder(const IR::P4Program * /*program*/) {
    // Don't invoke logStep for the top-level program, as that would be overly verbose.

    // Get the initial constraints of the target. These constraints influence branch selection.
    std::optional<const Constraint *> cond = programInfo.getTargetConstraints();

    // Have the target break apart the main declaration instance.
    auto &nextState = state.clone();

    // Initialize all relevant environment variables for the respective target.
    initializeTargetEnvironment(nextState);

    // If this option is active, mandate that all packets conform to a fixed size.
    auto pktSize = TestgenOptions::get().minPktSize;
    if (pktSize != 0) {
        const auto *fixedSizeEqu =
            new IR::Equ(ExecutionState::getInputPacketSizeVar(),
                        IR::getConstant(&PacketVars::PACKET_SIZE_VAR_TYPE, pktSize));
        if (cond == std::nullopt) {
            cond = fixedSizeEqu;
        } else {
            cond = new IR::LAnd(*cond, fixedSizeEqu);
        }
    }

    const auto *topLevelBlocks = programInfo.getPipelineSequence();

    // In between pipe lines we may drop packets or exit.
    // This segment inserts a special exception to deal with this case.
    // The drop exception just terminates execution completely.
    // We first pop the current body.
    nextState.popBody();
    // Then we insert the exception handlers.
    std::map<Continuation::Exception, Continuation> handlers;
    handlers.emplace(Continuation::Exception::Drop, Continuation::Body({}));
    handlers.emplace(Continuation::Exception::Exit, Continuation::Body({}));
    handlers.emplace(Continuation::Exception::Abort, Continuation::Body({}));
    nextState.pushCurrentContinuation(handlers);

    // After, we insert the program commands into the new body and push them to the top.
    Continuation::Body newBody(*topLevelBlocks);
    nextState.replaceBody(newBody);

    result->emplace_back(cond, state, nextState);
    return false;
}

bool CmdStepper::preorder(const IR::ParserState *parserState) {
    logStep(parserState);

    auto &nextState = state.clone();

    nextState.add(*new TraceEvents::ParserState(parserState));

    if (parserState->name == IR::ParserState::accept) {
        nextState.popContinuation();
        result->emplace_back(nextState);
        return false;
    }

    if (parserState->name == IR::ParserState::reject) {
        nextState.replaceTopBody(Continuation::Exception::Reject);
        result->emplace_back(nextState);
        return false;
    }

    // Push a new continuation that will take the next state as an argument and execute the state
    // as a command.
    // Create a parameter for the continuation we're about to build.
    const auto *v =
        Continuation::genParameter(IR::Type_State::get(), "nextState", state.getNamespaceContext());

    // Create the continuation itself.
    Continuation::Body kBody({v->param});
    Continuation k(v, kBody);
    nextState.pushContinuation(*new ExecutionState::StackFrame(k, state.getNamespaceContext()));

    // Enter the parser state's namespace
    nextState.pushNamespace(parserState);

    // Replace the parser state with its non-declaration components, followed by the select
    // expression.
    std::vector<Continuation::Command> cmds;
    for (const auto *component : parserState->components) {
        if (component->is<IR::IDeclaration>() && !component->is<IR::Declaration_Variable>()) {
            continue;
        }
        if (const auto *declVar = component->to<IR::Declaration_Variable>()) {
            nextState.declareVariable(TestgenTarget::get(), *declVar);
        } else {
            cmds.emplace_back(component);
        }
    }

    // Add the next parser state(s) to the cmds
    cmds.emplace_back(Continuation::Return(parserState->selectExpression));

    nextState.replaceTopBody(&cmds);
    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::BlockStatement *block) {
    logStep(block);

    auto &nextState = state.clone();
    std::vector<Continuation::Command> cmds;
    // Do not forget to add the namespace of the block statement.
    nextState.pushNamespace(block);
    // TODO (Fabian): Remove this? What is this for?
    for (const auto *component : block->components) {
        if (component->is<IR::IDeclaration>() && !component->is<IR::Declaration_Variable>()) {
            continue;
        }
        if (const auto *declVar = component->to<IR::Declaration_Variable>()) {
            nextState.declareVariable(TestgenTarget::get(), *declVar);
        } else {
            cmds.emplace_back(component);
        }
    }

    if (!cmds.empty()) {
        nextState.replaceTopBody(&cmds);
    } else {
        nextState.popBody();
    }

    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::ExitStatement *e) {
    logStep(e);
    auto &nextState = state.clone();
    nextState.markVisited(e);
    nextState.add(*new TraceEvents::Generic("Exit"));
    nextState.replaceTopBody(Continuation::Exception::Exit);
    result->emplace_back(nextState);
    return false;
}

const Constraint *CmdStepper::startParser(const IR::P4Parser *parser, ExecutionState &nextState) {
    // Reset the parser cursor to zero.
    const auto *parserCursorVarType = &PacketVars::PACKET_SIZE_VAR_TYPE;

    // Constrain the input packet size to its maximum.
    const auto *boolType = IR::Type::Boolean::get();
    const Constraint *result =
        new IR::Leq(boolType, ExecutionState::getInputPacketSizeVar(),
                    IR::getConstant(parserCursorVarType, ExecutionState::getMaxPacketLength()));

    // Constrain the input packet size to be a multiple of 8 bits. Do this by constraining the
    // lowest three bits of the packet size to 0.
    const auto *threeBitType = IR::Type::Bits::get(3);
    result = new IR::LAnd(
        boolType, result,
        new IR::Equ(boolType,
                    new IR::Slice(threeBitType, ExecutionState::getInputPacketSizeVar(),
                                  IR::getConstant(parserCursorVarType, 2),
                                  IR::getConstant(parserCursorVarType, 0)),
                    IR::getConstant(threeBitType, 0)));

    // Call the implementation for the specific target.
    // If we get a constraint back, add it to the result.
    if (auto constraintOpt = startParserImpl(parser, nextState)) {
        result = new IR::LAnd(boolType, result, *constraintOpt);
    }

    return result;
}

IR::SwitchStatement *CmdStepper::replaceSwitchLabels(const IR::SwitchStatement *switchStatement) {
    const auto *member = switchStatement->expression->to<IR::Member>();
    BUG_CHECK(member != nullptr && member->member.name == IR::Type_Table::action_run,
              "Invalid format of %1% for action_run", switchStatement->expression);
    const auto *methodCall = member->expr->to<IR::MethodCallExpression>();
    BUG_CHECK(methodCall, "Invalid format of %1% for action_run", member->expr);
    const auto *tableCall = methodCall->method->to<IR::Member>();
    BUG_CHECK(tableCall, "Invalid format of %1% for action_run", methodCall->method);
    const auto *table = state.getTableType(methodCall->method->to<IR::Member>());
    CHECK_NULL(table);
    auto actionVar = TableStepper::getTableActionVar(table);
    IR::Vector<IR::SwitchCase> newCases;
    std::map<cstring, int> actionsIds;
    for (size_t index = 0; index < table->getActionList()->size(); index++) {
        actionsIds.emplace(table->getActionList()->actionList.at(index)->getName().toString(),
                           index);
    }
    for (const auto *switchCase : switchStatement->cases) {
        auto *newSwitchCase = switchCase->clone();
        // Do not replace default expression labels.
        if (!newSwitchCase->label->is<IR::DefaultExpression>()) {
            newSwitchCase->label =
                IR::getConstant(actionVar->type, actionsIds[switchCase->label->toString()]);
        }
        newCases.push_back(newSwitchCase);
    }
    auto *newSwitch = switchStatement->clone();
    newSwitch->cases = newCases;
    return newSwitch;
}

bool CmdStepper::preorder(const IR::SwitchStatement *switchStatement) {
    logStep(switchStatement);

    if (!SymbolicEnv::isSymbolicValue(switchStatement->expression)) {
        // Evaluate the keyset in the first select case.
        return stepToSubexpr(
            switchStatement->expression, result, state,
            [switchStatement, this](const Continuation::Parameter *v) {
                if (!switchStatement->expression->type->is<IR::Type_ActionEnum>()) {
                    BUG("Only switch statements with action_run as expression are supported.");
                }
                // If the switch statement has table action as expression, replace
                // the case labels with indices.
                auto *newSwitch = replaceSwitchLabels(switchStatement);
                newSwitch->expression = v->param;
                return newSwitch;
            });
    }
    // After we have executed, we simple pick the index that matches with the returned constant.
    auto &nextState = state.clone();
    std::vector<Continuation::Command> cmds;
    // If the switch expression is tainted, we can not predict which case will be chosen. We taint
    // the program counter and execute all of the statements.
    P4::Coverage::CoverageSet coveredNodes;
    if (state.hasTaint(switchStatement->expression)) {
        auto currentTaint = state.getProperty<bool>("inUndefinedState");
        cmds.emplace_back(Continuation::PropertyUpdate("inUndefinedState", true));
        for (const auto *switchCase : switchStatement->cases) {
            if (switchCase->statement != nullptr) {
                cmds.emplace_back(switchCase->statement);
            }
        }
        cmds.emplace_back(Continuation::PropertyUpdate("inUndefinedState", currentTaint));
    } else {
        // Otherwise, we pick the switch statement case in a normal fashion.
        bool hasMatched = false;
        for (const auto *switchCase : switchStatement->cases) {
            if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
                // Some path selection strategies depend on looking ahead and collecting potential
                // statements. If that is the case, apply the CoverableNodesScanner visitor.
                auto collector = CoverableNodesScanner(state);
                collector.updateNodeCoverage(switchCase->statement, coveredNodes);
            }
            // We have either matched already, or still need to match.
            hasMatched = hasMatched || switchStatement->expression->equiv(*switchCase->label);
            // Nothing to do with this statement. Fall through to the next case.
            if (switchCase->statement == nullptr) {
                continue;
            }
            // If any of the values in the match list hits, execute the switch case block.
            if (hasMatched) {
                cmds.emplace_back(switchCase->statement);
                // If the statement is a block, we do not fall through and terminate execution.
                if (switchCase->statement->is<IR::BlockStatement>()) {
                    break;
                }
            }
            // The default label must be last. Always break here.
            if (switchCase->label->is<IR::DefaultExpression>()) {
                cmds.emplace_back(switchCase->statement);
                break;
            }
        }
    }
    BUG_CHECK(!cmds.empty(), "Switch statements should have at least one case (default).");
    nextState.replaceTopBody(&cmds);
    result->emplace_back(std::nullopt, state, nextState, coveredNodes);

    return false;
}

}  // namespace P4Tools::P4Testgen
