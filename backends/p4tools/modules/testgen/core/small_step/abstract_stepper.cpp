#include "backends/p4tools/modules/testgen/core/small_step/abstract_stepper.h"

#include <cstddef>
#include <optional>
#include <ostream>
#include <variant>
#include <vector>

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/variables.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/dump.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

AbstractStepper::AbstractStepper(ExecutionState &state, AbstractSolver &solver,
                                 const ProgramInfo &programInfo)
    : programInfo(programInfo), state(state), solver(solver), result(new std::vector<Branch>()) {}

AbstractStepper::Result AbstractStepper::step(const IR::Node *node) {
    node->apply(*this);
    return result;
}

bool AbstractStepper::preorder(const IR::Node *node) {
    logStep(node);
    dump(node);
    BUG("%1%: Unhandled node type in %2%: %3%", node, getClassName(), node->node_type_name());
}

const ProgramInfo &AbstractStepper::getProgramInfo() const { return programInfo; }

void AbstractStepper::logStep(const IR::Node *node) {
    LOG_FEATURE("small_step", 4,
                "***** " << getClassName() << "(" << node->node_type_name() << ") *****"
                         << std::endl
                         << node << std::endl);
}

void AbstractStepper::checkMemberInvariant(const IR::Node *node) {
    while (true) {
        const auto *member = node->to<IR::Member>();
        BUG_CHECK(member, "Not a member expression: %1%", node);

        node = member->expr;
        if (node->is<IR::PathExpression>()) {
            return;
        }
    }
}

bool AbstractStepper::stepSymbolicValue(const IR::Node *expr) {
    // Pop the topmost continuation and apply the value. This gives us the new body in the next
    // state. The namespace in the topmost continuation frame becomes the namespace in the next
    // state.
    BUG_CHECK(SymbolicEnv::isSymbolicValue(expr), "Not a symbolic value: %1% of type %2%", expr,
              expr->node_type_name());
    state.popContinuation(expr);
    result->emplace_back(state);
    return false;
}

bool AbstractStepper::stepToException(Continuation::Exception exception) {
    state.replaceTopBody(exception);
    result->emplace_back(state);
    return false;
}

bool AbstractStepper::stepToSubexpr(
    const IR::Expression *subexpr, SmallStepEvaluator::Result &result, const ExecutionState &state,
    std::function<const Continuation::Command(const Continuation::Parameter *)> rebuildCmd) {
    // Create a parameter for the continuation we're about to build.
    const auto *v = Continuation::genParameter(subexpr->type, "v", state.getNamespaceContext());

    // Create the continuation itself.
    Continuation::Body kBody(state.getBody());
    kBody.pop();
    kBody.push(rebuildCmd(v));
    Continuation k(v, kBody);

    // Create our new state.
    auto &nextState = state.clone();
    Continuation::Body stateBody({Continuation::Return(subexpr)});
    nextState.replaceBody(stateBody);
    nextState.pushContinuation(*new ExecutionState::StackFrame(k, state.getNamespaceContext()));

    result->emplace_back(nextState);
    return false;
}

bool AbstractStepper::stepToListSubexpr(
    const IR::BaseListExpression *subexpr, SmallStepEvaluator::Result &result,
    const ExecutionState &state,
    std::function<const Continuation::Command(const IR::BaseListExpression *)> rebuildCmd) {
    // Rewrite the list expression to replace the first non-value expression with the continuation
    // parameter.
    // XXX This results in the small-step evaluator evaluating list expressions in quadratic time.
    // XXX Ideally, we'd build a tower of continuations, with each non-value component of the list
    // XXX expression being evaluated in a separate continuation, and piece the final list
    // XXX expression back together in the last continuation. But the evaluation framework isn't
    // XXX currently rich enough to express this: we'd need lambdas in our continuation bodies.
    // XXX
    // XXX Alternatively, we could introduce P4 variables to store the values obtained by each
    // XXX intermediate continuation, but we'd have to be careful about how those variables are
    // XXX named.
    // XXX
    // XXX For now, we do the quadratic-time thing, since it's easiest.

    // Find the first non-value component of the list.
    const auto &components = subexpr->components;
    const IR::Expression *nonValueComponent = nullptr;
    size_t nonValueComponentIdx = 0;
    for (nonValueComponentIdx = 0; nonValueComponentIdx < components.size();
         nonValueComponentIdx++) {
        const auto *curComponent = components.at(nonValueComponentIdx);
        if (!SymbolicEnv::isSymbolicValue(curComponent)) {
            nonValueComponent = curComponent;
            break;
        }
    }

    BUG_CHECK(nonValueComponent != nullptr && nonValueComponentIdx != components.size(),
              "This list expression is a symbolic value, but is not expected to be one: %1%",
              subexpr);

    return stepToSubexpr(
        nonValueComponent, result, state,
        [nonValueComponentIdx, rebuildCmd, subexpr](const Continuation::Parameter *v) {
            auto *result = subexpr->clone();
            result->components[nonValueComponentIdx] = v->param;
            return rebuildCmd(result);
        });
}

bool AbstractStepper::stepToStructSubexpr(
    const IR::StructExpression *subexpr, SmallStepEvaluator::Result &result,
    const ExecutionState &state,
    std::function<const Continuation::Command(const IR::StructExpression *)> rebuildCmd) {
    // @see stepToListSubexpr regarding the assessment of complexity of this function.

    // Find the first non-value component of the list.
    const auto &components = subexpr->components;
    const IR::Expression *nonValueComponent = nullptr;
    size_t nonValueComponentIdx = 0;
    for (nonValueComponentIdx = 0; nonValueComponentIdx < components.size();
         nonValueComponentIdx++) {
        const auto *curComponent = components.at(nonValueComponentIdx);
        if (!SymbolicEnv::isSymbolicValue(curComponent->expression)) {
            nonValueComponent = curComponent->expression;
            break;
        }
    }

    BUG_CHECK(nonValueComponent != nullptr && nonValueComponentIdx != components.size(),
              "This list expression is a symbolic value, but is not expected to be one: %1%",
              subexpr);

    return stepToSubexpr(
        nonValueComponent, result, state,
        [nonValueComponentIdx, rebuildCmd, subexpr](const Continuation::Parameter *v) {
            auto *result = subexpr->clone();
            auto *namedClone = result->components[nonValueComponentIdx]->clone();
            namedClone->expression = v->param;
            result->components[nonValueComponentIdx] = namedClone;
            return rebuildCmd(result);
        });
}

bool AbstractStepper::stepGetHeaderValidity(const IR::StateVariable &headerRef) {
    // The top of the body should be a Return command containing a call to getValid on the given
    // header ref. Replace this with the variable representing the header ref's validity.
    if (const auto *headerUnion = headerRef->type->to<IR::Type_HeaderUnion>()) {
        for (const auto *field : headerUnion->fields) {
            auto *fieldRef = new IR::Member(field->type, headerRef, field->name);
            const auto &variable = ToolsVariables::getHeaderValidity(fieldRef);
            BUG_CHECK(state.exists(variable),
                      "At this point, the header validity bit should be initialized.");
            const auto *value = state.getSymbolicEnv().get(variable);
            const auto *res = value->to<IR::BoolLiteral>();
            BUG_CHECK(res, "%1%: expected a boolean", value);
            if (res->value) {
                state.replaceTopBody(
                    Continuation::Return(new IR::BoolLiteral(IR::Type::Boolean::get(), true)));
                result->emplace_back(state);
                return false;
            }
        }
        state.replaceTopBody(
            Continuation::Return(new IR::BoolLiteral(IR::Type::Boolean::get(), false)));
        result->emplace_back(state);
        return false;
    }
    const auto &variable = ToolsVariables::getHeaderValidity(headerRef);
    BUG_CHECK(state.exists(variable),
              "At this point, the header validity bit %1% should be initialized.", variable);
    state.replaceTopBody(Continuation::Return(variable));
    result->emplace_back(state);
    return false;
}

void AbstractStepper::setHeaderValidity(const IR::StateVariable &headerRef, bool validity,
                                        ExecutionState &nextState) {
    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(headerRef);
    nextState.set(headerRefValidity, IR::getBoolLiteral(validity));

    // In some cases, the header may be `part of a union.
    if (validity) {
        const auto *headerBaseMember = headerRef.ref->to<IR::Member>();
        if (headerBaseMember == nullptr) {
            return;
        }
        const auto *headerBase = headerBaseMember->expr;
        // In the case of header unions, we need to set all other union members invalid.
        if (const auto *hdrUnion = headerBase->type->to<IR::Type_HeaderUnion>()) {
            for (const auto *field : hdrUnion->fields) {
                auto *member = new IR::Member(field->type, headerBase, field->name);
                // Ignore the member we are setting to valid.
                if (headerRef->equiv(*member)) {
                    continue;
                }
                // Set all other members to invalid.
                setHeaderValidity(member, false, nextState);
            }
        }
        return;
    }
    std::vector<IR::StateVariable> validityVector;
    auto fieldsVector = nextState.getFlatFields(headerRef, &validityVector);
    // The header is going to be invalid. Set all fields to taint constants.
    // TODO: Should we make this target specific? Some targets set the header fields to 0.
    for (const auto &field : fieldsVector) {
        nextState.set(field, programInfo.createTargetUninitialized(field->type, true));
    }
}

bool AbstractStepper::stepSetHeaderValidity(const IR::StateVariable &headerRef, bool validity) {
    // The top of the body should be a Return command containing a call to setValid or setInvalid
    // on the given header ref. Update the symbolic environment to reflect the changed validity
    // bit, and replace the command with an expressionless Return.
    setHeaderValidity(headerRef, validity, state);
    state.replaceTopBody(Continuation::Return());
    result->emplace_back(state);
    return false;
}

const IR::MethodCallStatement *generateStacksetValid(const IR::Expression *stackRef, int index,
                                                     bool isValid) {
    const auto *arrayIndex = HSIndexToMember::produceStackIndex(
        stackRef->type->checkedTo<IR::Type_Stack>()->elementType, stackRef, index);
    auto name = (isValid) ? IR::Type_Header::setValid : IR::Type_Header::setInvalid;
    return new IR::MethodCallStatement(new IR::MethodCallExpression(
        new IR::Type_Void(),
        new IR::Member(new IR::Type_Method(new IR::Type_Void(), new IR::ParameterList(), name),
                       arrayIndex, name)));
}

void generateStackAssigmentStatement(ExecutionState &nextState,
                                     std::vector<Continuation::Command> &replacements,
                                     const IR::Expression *stackRef, int leftIndex,
                                     int rightIndex) {
    const auto *elemType = stackRef->type->checkedTo<IR::Type_Stack>()->elementType;
    const auto *leftArIndex = HSIndexToMember::produceStackIndex(elemType, stackRef, leftIndex);
    const auto *rightArrIndex = HSIndexToMember::produceStackIndex(elemType, stackRef, rightIndex);

    // Check right header validity.
    const auto *value =
        nextState.getSymbolicEnv().get(ToolsVariables::getHeaderValidity(rightArrIndex));
    if (!value->checkedTo<IR::BoolLiteral>()->value) {
        replacements.emplace_back(generateStacksetValid(stackRef, leftIndex, false));
        return;
    }
    replacements.emplace_back(generateStacksetValid(stackRef, leftIndex, true));

    // Unfold fields.
    auto leftVector = nextState.getFlatFields(leftArIndex);
    auto rightVector = nextState.getFlatFields(rightArrIndex);
    for (size_t i = 0; i < leftVector.size(); i++) {
        replacements.emplace_back(new IR::AssignmentStatement(leftVector[i], rightVector[i]));
    }
}

bool AbstractStepper::stepStackPushPopFront(const IR::Expression *stackRef,
                                            const IR::Vector<IR::Argument> *args, bool isPush) {
    const auto *stackType = stackRef->type->checkedTo<IR::Type_Stack>();
    auto sz = static_cast<int>(stackType->getSize());
    BUG_CHECK(args->size() == 1, "Invalid size of arguments for %1%", stackRef);
    auto count = args->at(0)->expression->checkedTo<IR::Constant>()->asInt();
    std::vector<Continuation::Command> replacements;
    auto &nextState = state.clone();
    if (isPush) {
        for (int i = sz - 1; i >= 0; i -= 1) {
            if (i >= count) {
                generateStackAssigmentStatement(nextState, replacements, stackRef, i, i - count);
            } else {
                replacements.emplace_back(generateStacksetValid(stackRef, i, false));
            }
        }
    } else {
        for (int i = 0; i < sz; i++) {
            if (i + count < sz) {
                generateStackAssigmentStatement(nextState, replacements, stackRef, i, i + count);
            } else {
                replacements.emplace_back(generateStacksetValid(stackRef, i, false));
            }
        }
    }
    nextState.replaceTopBody(&replacements);
    result->emplace_back(nextState);
    return false;
}

const IR::Literal *AbstractStepper::evaluateExpression(
    const IR::Expression *expr, std::optional<const IR::Expression *> cond) const {
    BUG_CHECK(solver.isInIncrementalMode(),
              "Currently, expression valuation only supports an incremental solver.");
    auto constraints = state.getPathConstraint();
    expr = state.getSymbolicEnv().subst(expr);
    expr = P4::optimizeExpression(expr);
    // Assert the path constraint to the solver and check whether it is satisfiable.
    if (cond) {
        constraints.push_back(*cond);
    }
    auto solverResult = solver.checkSat(constraints);
    // If the solver can find a solution under the given condition, get the model and return the
    // value.
    const IR::Literal *result = nullptr;
    if (solverResult != std::nullopt && *solverResult) {
        auto model = Model(solver.getSymbolicMapping());
        result = model.evaluate(expr, true);
    }
    return result;
}

void AbstractStepper::setTargetUninitialized(ExecutionState &nextState,
                                             const IR::StateVariable &ref, bool forceTaint) const {
    // Resolve the type of the left-and assignment, if it is a type name.
    const auto *refType = nextState.resolveType(ref->type);
    if (refType->is<const IR::Type_StructLike>()) {
        std::vector<IR::StateVariable> validFields;
        auto fields = nextState.getFlatFields(ref, &validFields);
        // We also need to initialize the validity bits of the headers. These are false.
        for (const auto &validField : validFields) {
            nextState.set(validField, IR::getBoolLiteral(false));
        }
        // For each field in the undefined struct, we create a new symbolic variable.
        // If the variable does not have an initializer we need to create a new variable for it.
        // For now we just use the name directly.
        for (const auto &field : fields) {
            nextState.set(field, programInfo.createTargetUninitialized(field->type, forceTaint));
        }
    } else if (const auto *baseType = refType->to<const IR::Type_Base>()) {
        nextState.set(ref, programInfo.createTargetUninitialized(baseType, forceTaint));
    } else {
        P4C_UNIMPLEMENTED("Unsupported uninitialization type %1%", refType->node_type_name());
    }
}

void AbstractStepper::declareStructLike(ExecutionState &nextState,
                                        const IR::StateVariable &parentExpr,
                                        bool forceTaint) const {
    std::vector<IR::StateVariable> validFields;
    auto fields = nextState.getFlatFields(parentExpr, &validFields);
    // We also need to initialize the validity bits of the headers. These are false.
    for (const auto &validField : validFields) {
        nextState.set(validField, IR::getBoolLiteral(false));
    }
    // For each field in the undefined struct, we create a new symbolic variable.
    // If the variable does not have an initializer we need to create a new variable for it.
    // For now we just use the name directly.
    for (const auto &field : fields) {
        nextState.set(field, programInfo.createTargetUninitialized(field->type, forceTaint));
    }
}

void AbstractStepper::declareBaseType(ExecutionState &nextState, const IR::StateVariable &paramPath,
                                      const IR::Type_Base *baseType) const {
    nextState.set(paramPath, programInfo.createTargetUninitialized(baseType, false));
}

}  // namespace P4Tools::P4Testgen
