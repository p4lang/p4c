#include "backends/p4tools/testgen/core/small_step/cmd_stepper.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <ostream>
#include <vector>

#include <boost/none.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

#include "backends/p4tools/common/compiler/hs_index_simplify.h"
#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/safe_vector.h"

#include "backends/p4tools/testgen//lib/exceptions.h"
#include "backends/p4tools/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

CmdStepper::CmdStepper(ExecutionState& state, AbstractSolver& solver,
                       const ProgramInfo& programInfo)
    : AbstractStepper(state, solver, programInfo) {}

void CmdStepper::declareStructLike(ExecutionState* nextState, const IR::Expression* parentExpr,
                                   const IR::Type_StructLike* structType) const {
    std::vector<const IR::Member*> validFields;
    auto fields = nextState->getFlatFields(parentExpr, structType, &validFields);
    // We also need to initialize the validity bits of the headers. These are false.
    for (const auto* validField : validFields) {
        nextState->set(validField, IRUtils::getBoolLiteral(false));
    }
    // For each field in the undefined struct, we create a new zombie variable.
    // If the variable does not have an initializer we need to create a new zombie for it.
    // For now we just use the name directly.
    for (const auto* field : fields) {
        nextState->set(field, programInfo.createTargetUninitialized(field->type, false));
    }
}

void CmdStepper::initializeBlockParams(const IR::Type_Declaration* typeDecl,
                                       const std::vector<cstring>* blockParams,
                                       ExecutionState* nextState) const {
    const IR::ParameterList* params = nullptr;

    // Collect parameters.
    if (const auto* p4parser = typeDecl->to<IR::P4Parser>()) {
        params = p4parser->getApplyParameters();
    } else if (const auto* p4control = typeDecl->to<IR::P4Control>()) {
        params = p4control->getApplyParameters();
    } else {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                              typeDecl->node_type_name());
    }
    for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
        const auto* param = params->getParameter(paramIdx);
        const auto* paramType = param->type;
        // Retrieve the identifier of the global architecture map using the parameter index.
        auto archRef = blockParams->at(paramIdx);
        // Irrelevant parameter. Ignore.
        if (archRef == nullptr) {
            continue;
        }
        // We need to resolve type names.
        if (const auto* tn = paramType->to<IR::Type_Name>()) {
            paramType = nextState->resolveType(tn);
        }
        const auto* paramPath = new IR::PathExpression(paramType, new IR::Path(archRef));
        if (const auto* ts = paramType->to<IR::Type_StructLike>()) {
            declareStructLike(nextState, paramPath, ts);
        } else if (paramType->is<IR::Type_Base>()) {
            auto paramRef = nextState->convertPathExpr(paramPath);
            nextState->set(paramRef, programInfo.createTargetUninitialized(paramRef->type, false));
        } else {
            P4C_UNIMPLEMENTED("Unsupported initialization type %1%", paramType->node_type_name());
        }
    }
}

bool CmdStepper::preorder(const IR::AssignmentStatement* assign) {
    logStep(assign);

    if (!SymbolicEnv::isSymbolicValue(assign->right)) {
        return stepToSubexpr(assign->right, result, state,
                             [assign](const Continuation::Parameter* v) {
                                 auto* result = assign->clone();
                                 result->right = v->param;
                                 return result;
                             });
    }

    state.markVisited(assign);
    const auto* left = assign->left;
    const auto* leftType = left->type;

    // Resolve the type of the left-and assignment, if it is a type name.
    if (leftType->is<IR::Type_Name>()) {
        leftType = state.resolveType(leftType->to<IR::Type_Name>());
    }
    // Although we typically expand structure assignments into individual member assignments using
    // the copyHeaders pass, some extern functions may return a list or struct expression. We can
    // not always expand these return values as we do with the expandLookahead pass.
    // Correspondingly, we need to retrieve the fields and set each member individually. This
    // assumes that all headers and structures have been flattened and no nesting is left.
    if (const auto* structType = leftType->to<IR::Type_StructLike>()) {
        const auto* listExpr = assign->right->checkedTo<IR::ListExpression>();

        std::vector<const IR::Member*> flatRefValids;
        auto flatRefFields = state.getFlatFields(left, structType, &flatRefValids);
        // First, complete the assignments for the data structure.
        for (size_t idx = 0; idx < flatRefFields.size(); ++idx) {
            const auto* leftFieldRef = flatRefFields[idx];
            state.set(leftFieldRef, listExpr->components[idx]);
        }
        // In case of a header, we also need to set the validity bits to true.
        for (const auto* headerValid : flatRefValids) {
            state.set(headerValid, IRUtils::getBoolLiteral(true));
        }
    } else if (leftType->is<IR::Type_Base>()) {
        // Convert a path expression into the respective member.
        if (const auto* path = assign->left->to<IR::PathExpression>()) {
            left = state.convertPathExpr(path);
        }
        // Check that all members have the correct format. All members end with a pathExpression.
        checkMemberInvariant(left);
        state.set(left, assign->right);
    } else {
        TESTGEN_UNIMPLEMENTED("Unsupported assign type %1% node: %2%", leftType,
                              leftType->node_type_name());
    }

    state.popBody();
    result->emplace_back(&state);
    return false;
}

bool CmdStepper::preorder(const IR::Declaration_Variable* decl) {
    const auto* declType = decl->type;
    if (declType->is<IR::Type_Name>()) {
        declType = state.resolveType(declType->to<IR::Type_Name>());
    }
    if (decl->initializer != nullptr) {
        TESTGEN_UNIMPLEMENTED("Unsupported initializer %s for declaration variable.",
                              decl->initializer);
    }

    if (const auto* structType = declType->to<IR::Type_StructLike>()) {
        const auto* parentExpr = new IR::PathExpression(structType, new IR::Path(decl->name.name));
        declareStructLike(&state, parentExpr, structType);
    } else if (const auto* stackType = declType->to<IR::Type_Stack>()) {
        const auto* stackSizeExpr = stackType->size;
        auto stackSize = stackSizeExpr->checkedTo<IR::Constant>()->asInt();
        const auto* stackElemType = stackType->elementType;
        if (stackElemType->is<IR::Type_Name>()) {
            stackElemType = state.resolveType(stackElemType->to<IR::Type_Name>());
        }
        const auto* structType = stackElemType->checkedTo<IR::Type_StructLike>();
        for (auto idx = 0; idx < stackSize; idx++) {
            const auto* parentExpr = HSIndexToMember::produceStackIndex(
                structType, new IR::PathExpression(stackType, new IR::Path(decl->name.name)), idx);
            declareStructLike(&state, parentExpr, structType);
        }
    } else if (declType->is<IR::Type_Base>()) {
        // If the variable does not have an initializer we need to create a new zombie for it.
        // For now we just use the name directly.
        const auto& left = state.convertPathExpr(new IR::PathExpression(decl->name));
        state.set(left, programInfo.createTargetUninitialized(decl->type, false));
    } else {
        TESTGEN_UNIMPLEMENTED("Unsupported declaration type %1% node: %2%", declType,
                              declType->node_type_name());
    }
    state.popBody();
    result->emplace_back(&state);
    return false;
}

bool CmdStepper::preorder(const IR::P4Parser* p4parser) {
    logStep(p4parser);

    auto* nextState = new ExecutionState(state);

    auto blockName = p4parser->getName().name;

    // Register and do target-specific initialization of the parser.
    const auto* constraint = startParser(p4parser, nextState);

    // Add trace events.
    nextState->add(new TraceEvent::ParserStart(p4parser, blockName));

    // Remove the invocation of the parser from the current body and push the resulting
    // continuation onto the continuation stack.
    nextState->popBody();

    auto handlers = getExceptionHandlers(p4parser, nextState->getBody(), nextState);
    nextState->pushCurrentContinuation(handlers);

    // Obtain the parser's namespace.
    const auto* ns = p4parser->to<IR::INamespace>();
    CHECK_NULL(ns);
    // Enter the parser's namespace.
    nextState->pushNamespace(ns);

    // Set the start state as the new body.
    const auto* startState = p4parser->states.getDeclaration<IR::ParserState>("start");
    std::vector<Continuation::Command> replacements;
    // Add parser-local declarations.
    for (const auto* decl : p4parser->parserLocals) {
        if (decl->is<IR::IDeclaration>() && !decl->is<IR::Declaration_Variable>()) {
            continue;
        }
        replacements.emplace_back(decl);
    }
    replacements.emplace_back(startState);
    nextState->replaceBody(Continuation::Body(replacements));

    result->emplace_back(constraint, state, nextState);
    return false;
}

bool CmdStepper::preorder(const IR::P4Control* p4control) {
    auto* nextState = new ExecutionState(state);

    auto blockName = p4control->getName().name;

    // Add trace events.
    std::stringstream controlName;
    controlName << "Control " << blockName << " start";
    nextState->add(new TraceEvent::Generic(controlName));

    // Set the emit buffer to be zero for the current control pipeline.
    nextState->resetEmitBuffer();

    // Add control-local declarations.
    std::vector<Continuation::Command> replacements;
    for (const auto* decl : p4control->controlLocals) {
        if (decl->is<IR::IDeclaration>() && !decl->is<IR::Declaration_Variable>()) {
            continue;
        }
        replacements.emplace_back(decl);
    }
    // Add the body, if it is not empty
    if (p4control->body != nullptr) {
        replacements.emplace_back(p4control->body);
    }
    // Remove the invocation of the control from the current body and push the resulting
    // continuation onto the continuation stack.
    nextState->popBody();
    // Exit terminates the entire control pipeline (only the control).
    std::map<Continuation::Exception, Continuation> handlers;
    handlers.emplace(Continuation::Exception::Exit, Continuation::Body({}));
    nextState->pushCurrentContinuation(handlers);
    // If the replacements are not empty, replace the body
    if (!replacements.empty()) {
        Continuation::Body newBody(replacements);
        nextState->replaceBody(newBody);
    }

    // Obtain the control's namespace.
    const auto* ns = p4control->to<IR::INamespace>();
    CHECK_NULL(ns);
    // Enter the control's namespace.
    nextState->pushNamespace(ns);

    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::EmptyStatement* /*empty*/) {
    auto* nextState = new ExecutionState(state);
    nextState->popBody();
    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::IfStatement* ifStatement) {
    logStep(ifStatement);

    if (!SymbolicEnv::isSymbolicValue(ifStatement->condition)) {
        // Evaluate the condition.
        return stepToSubexpr(ifStatement->condition, result, state,
                             [ifStatement](const Continuation::Parameter* v) {
                                 auto* result = ifStatement->clone();
                                 result->condition = v->param;
                                 return result;
                             });
    }
    // If the condition of the if statement is tainted, the program counter is effectively tainted.
    // We insert a property that marks all execution as tainted until the if statement has
    // completed.
    // Because we may have nested taint, we need to check if we are already tainted.
    if (state.hasTaint(ifStatement->condition)) {
        auto* nextState = new ExecutionState(state);
        std::vector<Continuation::Command> cmds;
        auto currentTaint = state.getProperty<bool>("inUndefinedState");
        nextState->add(
            new TraceEvent::PreEvalExpression(ifStatement->condition, "Tainted If Statement"));
        cmds.emplace_back(Continuation::PropertyUpdate("inUndefinedState", true));
        cmds.emplace_back(ifStatement->ifTrue);
        if (ifStatement->ifFalse != nullptr) {
            cmds.emplace_back(ifStatement->ifFalse);
        }
        cmds.emplace_back(Continuation::PropertyUpdate("inUndefinedState", currentTaint));
        nextState->replaceTopBody(&cmds);
        result->emplace_back(nextState);
        return false;
    }
    // Handle case where a condition is true: proceed to a body.
    {
        auto* nextState = new ExecutionState(state);
        std::vector<Continuation::Command> cmds;
        nextState->add(
            new TraceEvent::PreEvalExpression(ifStatement->condition, "If Statement true"));
        cmds.emplace_back(ifStatement->ifTrue);
        nextState->replaceTopBody(&cmds);
        result->emplace_back(ifStatement->condition, state, nextState);
    }

    // Handle case for else body.
    {
        auto* negation = new IR::LNot(IR::Type::Boolean::get(), ifStatement->condition);
        auto* nextState = new ExecutionState(state);
        nextState->add(
            new TraceEvent::PreEvalExpression(ifStatement->condition, "If Statement false"));

        nextState->replaceTopBody((ifStatement->ifFalse == nullptr) ? new IR::BlockStatement()
                                                                    : ifStatement->ifFalse);
        result->emplace_back(negation, state, nextState);
    }

    return false;
}

bool CmdStepper::preorder(const IR::MethodCallStatement* methodCallStatement) {
    logStep(methodCallStatement);
    state.markVisited(methodCallStatement);
    state.popBody();
    const auto* type = methodCallStatement->methodCall->type;
    // do not push continuation for table application
    if (methodCallStatement->methodCall->method->type->is<IR::Type_Method>() &&
        methodCallStatement->methodCall->method->is<IR::PathExpression>()) {
        type = methodCallStatement->methodCall->type;
    } else if (state.getActionDecl(methodCallStatement->methodCall->method) != nullptr ||
               state.getTableType(methodCallStatement->methodCall->method) != nullptr) {
        type = IR::Type::Void::get();
    }
    state.pushCurrentContinuation(type);
    state.replaceBody(Continuation::Body({Continuation::Return(methodCallStatement->methodCall)}));
    result->emplace_back(&state);
    return false;
}

bool CmdStepper::preorder(const IR::P4Program* /*program*/) {
    // Don't invoke logStep for the top-level program, as that would be overly verbose.

    // Get the initial constraints of the target. These constraints influence branch selection.
    boost::optional<const Constraint*> cond = programInfo.getTargetConstraints();

    // Have the target break apart the main declaration instance.
    auto* nextState = new ExecutionState(state);

    // Initialize all relevant environment variables for the respective target.
    initializeTargetEnvironment(nextState);

    // We mandate all packets to conform to a fixed size. This is a temporary change.
    auto pktSize = TestgenOptions::get().packetSize;
    if (pktSize != 0) {
        auto maxPktLength = ExecutionState::getMaxPacketLength_bits();
        if (pktSize < 0 || pktSize > maxPktLength) {
            ::error("Invalid input packet size. The valid range is from 1 to %s bits.",
                    maxPktLength);
            return false;
        }
        const auto* fixedSizeEqu =
            new IR::Equ(ExecutionState::getInputPacketSizeVar(),
                        IRUtils::getConstant(ExecutionState::getPacketSizeVarType(), pktSize));
        if (cond == boost::none) {
            cond = fixedSizeEqu;
        } else {
            cond = new IR::LAnd(*cond, fixedSizeEqu);
        }
    }

    const auto* topLevelBlocks = programInfo.getPipelineSequence();
    // If we are just producing input packets, remove anything appearing after the first parser.
    // TODO: Move this into getPipelineSequence? Also, clean up.
    if (TestgenOptions::get().inputPacketOnly) {
        auto* blocks = new std::vector<Continuation::Command>();
        for (const auto& block : *topLevelBlocks) {
            blocks->push_back(block);
            const auto* p4Node = boost::get<const IR::Node*>(&block);
            if (p4Node == nullptr) {
                continue;
            }
            if (const auto* cce = (*p4Node)->to<IR::ConstructorCallExpression>()) {
                if (cce->type->is<IR::Type_Parser>()) {
                    break;
                }
            }
        }
        topLevelBlocks = blocks;
    }

    // In between pipe lines we may drop packets or exit.
    // This segment inserts a special exception to deal with this case.
    // The drop exception just terminates execution completely.
    // We first pop the current body.
    nextState->popBody();
    // Then we insert the exception handlers.
    std::map<Continuation::Exception, Continuation> handlers;
    handlers.emplace(Continuation::Exception::Drop, Continuation::Body({}));
    handlers.emplace(Continuation::Exception::Exit, Continuation::Body({}));
    handlers.emplace(Continuation::Exception::Abort, Continuation::Body({}));
    nextState->pushCurrentContinuation(handlers);

    // After, we insert the program commands into the new body and push them to the top.
    Continuation::Body newBody(*topLevelBlocks);
    nextState->replaceBody(newBody);

    result->emplace_back(cond, state, nextState);
    return false;
}

bool CmdStepper::preorder(const IR::ParserState* parserState) {
    logStep(parserState);

    auto* nextState = new ExecutionState(state);

    nextState->add(new TraceEvent::ParserState(parserState));

    if (parserState->name == IR::ParserState::accept) {
        nextState->popContinuation();
        result->emplace_back(nextState);
        return false;
    }

    if (parserState->name == IR::ParserState::reject) {
        nextState->replaceTopBody(Continuation::Exception::Reject);
        result->emplace_back(nextState);
        return false;
    }

    // Push a new continuation that will take the next state as an argument and execute the state
    // as a command.
    // Create a parameter for the continuation we're about to build.
    const auto* v =
        Continuation::genParameter(IR::Type_State::get(), "nextState", state.getNamespaceContext());

    // Create the continuation itself.
    Continuation::Body kBody({v->param});
    Continuation k(v, kBody);
    nextState->pushContinuation(new ExecutionState::StackFrame(k, state.getNamespaceContext()));

    // Enter the parser state's namespace
    nextState->pushNamespace(parserState);

    // Replace the parser state with its non-declaration components, followed by the select
    // expression.
    std::vector<Continuation::Command> replacements;
    for (const auto* statOrDecl : parserState->components) {
        if (statOrDecl->is<IR::IDeclaration>() && !statOrDecl->is<IR::Declaration_Variable>()) {
            continue;
        }
        replacements.emplace_back(statOrDecl);
    }

    // Add the next parser state(s) to the replacements
    replacements.emplace_back(Continuation::Return(parserState->selectExpression));

    nextState->replaceTopBody(&replacements);
    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::BlockStatement* block) {
    logStep(block);

    auto* nextState = new ExecutionState(state);
    std::vector<Continuation::Command> replacements;
    // Do not forget to add the namespace of the block statement.
    nextState->pushNamespace(block);
    // TODO (Fabian): Remove this? What is this for?
    for (const auto* statOrDecl : block->components) {
        if (statOrDecl->is<IR::IDeclaration>() && !statOrDecl->is<IR::Declaration_Variable>()) {
            continue;
        }
        replacements.emplace_back(statOrDecl);
    }

    if (!replacements.empty()) {
        nextState->replaceTopBody(&replacements);
    } else {
        nextState->popBody();
    }

    result->emplace_back(nextState);
    return false;
}

bool CmdStepper::preorder(const IR::ExitStatement* e) {
    logStep(e);
    auto* nextState = new ExecutionState(state);
    nextState->markVisited(e);
    nextState->replaceTopBody(Continuation::Exception::Exit);
    result->emplace_back(nextState);
    return false;
}

const Constraint* CmdStepper::startParser(const IR::P4Parser* parser, ExecutionState* nextState) {
    // Reset the parser cursor to zero.
    const auto* parserCursorVarType = ExecutionState::getPacketSizeVarType();

    // Constrain the input packet size to its maximum.
    const auto* boolType = IR::Type::Boolean::get();
    const Constraint* result = new IR::Leq(
        boolType, ExecutionState::getInputPacketSizeVar(),
        IRUtils::getConstant(parserCursorVarType, ExecutionState::getMaxPacketLength_bits()));

    // Constrain the input packet size to be a multiple of 8 bits. Do this by constraining the
    // lowest three bits of the packet size to 0.
    const auto* threeBitType = IR::Type::Bits::get(3);
    result = new IR::LAnd(
        boolType, result,
        new IR::Equ(boolType,
                    new IR::Slice(threeBitType, ExecutionState::getInputPacketSizeVar(),
                                  IRUtils::getConstant(parserCursorVarType, 2),
                                  IRUtils::getConstant(parserCursorVarType, 0)),
                    IRUtils::getConstant(threeBitType, 0)));

    // Call the implementation for the specific target.
    // If we get a constraint back, add it to the result.
    if (auto constraintOpt = startParser_impl(parser, nextState)) {
        result = new IR::LAnd(boolType, result, *constraintOpt);
    }

    return result;
}

IR::SwitchStatement* CmdStepper::replaceSwitchLabels(const IR::SwitchStatement* switchStatement) {
    const auto* member = switchStatement->expression->to<IR::Member>();
    BUG_CHECK(member != nullptr && member->member.name == IR::Type_Table::action_run,
              "Invalid format of %1% for action_run", switchStatement->expression);
    const auto* methodCall = member->expr->to<IR::MethodCallExpression>();
    BUG_CHECK(methodCall, "Invalid format of %1% for action_run", member->expr);
    const auto* tableCall = methodCall->method->to<IR::Member>();
    BUG_CHECK(tableCall, "Invalid format of %1% for action_run", methodCall->method);
    const auto* table = state.getTableType(methodCall->method->to<IR::Member>());
    CHECK_NULL(table);
    auto actionVar = TableStepper::getTableActionVar(table);
    IR::Vector<IR::SwitchCase> newCases;
    std::map<cstring, int> actionsIds;
    for (size_t index = 0; index < table->getActionList()->size(); index++) {
        actionsIds.emplace(table->getActionList()->actionList.at(index)->getName().toString(),
                           index);
    }
    for (const auto* switchCase : switchStatement->cases) {
        auto* newSwitchCase = switchCase->clone();
        // Do not replace default expression labels.
        if (!newSwitchCase->label->is<IR::DefaultExpression>()) {
            newSwitchCase->label =
                IRUtils::getConstant(actionVar->type, actionsIds[switchCase->label->toString()]);
        }
        newCases.push_back(newSwitchCase);
    }
    auto* newSwitch = switchStatement->clone();
    newSwitch->cases = newCases;
    return newSwitch;
}

bool CmdStepper::preorder(const IR::SwitchStatement* switchStatement) {
    logStep(switchStatement);

    if (!SymbolicEnv::isSymbolicValue(switchStatement->expression)) {
        // Evaluate the keyset in the first select case.
        return stepToSubexpr(
            switchStatement->expression, result, state,
            [switchStatement, this](const Continuation::Parameter* v) {
                if (!switchStatement->expression->type->is<IR::Type_ActionEnum>()) {
                    BUG("Only switch statements with action_run as expression are supported.");
                }
                // If the switch statement has table action as expression, replace
                // the case labels with indices.
                auto* newSwitch = replaceSwitchLabels(switchStatement);
                newSwitch->expression = v->param;
                return newSwitch;
            });
    }
    // After we have executed, we simple pick the index that matches with the returned constant.
    auto* nextState = new ExecutionState(state);
    std::vector<Continuation::Command> replacements;
    // If the switch expression is tainted, we can not predict which case will be chosen. We taint
    // the program counter and execute all of the statements.
    if (state.hasTaint(switchStatement->expression)) {
        auto currentTaint = state.getProperty<bool>("inUndefinedState");
        replacements.emplace_back(Continuation::PropertyUpdate("inUndefinedState", true));
        for (const auto* switchCase : switchStatement->cases) {
            if (switchCase->statement != nullptr) {
                replacements.emplace_back(switchCase->statement);
            }
        }
        replacements.emplace_back(Continuation::PropertyUpdate("inUndefinedState", currentTaint));
    } else {
        // Otherwise, we pick the switch statement case in a normal fashion.
        for (const auto* switchCase : switchStatement->cases) {
            // Nothing to do with this statement, fall through to the next case.
            if (switchCase->statement == nullptr) {
                continue;
            }
            if (switchStatement->expression->equiv(*switchCase->label)) {
                replacements.emplace_back(switchCase->statement);
                // If the statement is a block, we do not fall through
                if (switchCase->statement->is<IR::BlockStatement>()) {
                    break;
                }
            }
            // The default label should be last, so break here.
            if (switchCase->label->is<IR::DefaultExpression>()) {
                replacements.emplace_back(switchCase->statement);
                break;
            }
        }
    }

    BUG_CHECK(!replacements.empty(), "Switch statements should have at least one case (default).");
    nextState->replaceTopBody(&replacements);
    result->emplace_back(nextState);

    return false;
}

}  // namespace P4Testgen

}  // namespace P4Tools
