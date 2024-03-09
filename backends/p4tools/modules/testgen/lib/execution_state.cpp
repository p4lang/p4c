#include "backends/p4tools/modules/testgen/lib/execution_state.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <boost/container/vector.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/variables.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "lib/log.h"
#include "lib/null.h"
#include "lib/ordered_map.h"
#include "lib/source_file.h"

#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

/* =============================================================================================
 *  Constructors
 * ============================================================================================= */

ExecutionState::StackFrame::StackFrame(Continuation normalContinuation,
                                       const NamespaceContext *namespaces)
    : StackFrame(std::move(normalContinuation), {}, namespaces) {}

ExecutionState::StackFrame::StackFrame(Continuation normalContinuation,
                                       ExceptionHandlers exceptionHandlers,
                                       const NamespaceContext *namespaces)
    : normalContinuation(std::move(normalContinuation)),
      exceptionHandlers(std::move(exceptionHandlers)),
      namespaces(namespaces) {}

const Continuation &ExecutionState::StackFrame::getContinuation() const {
    return normalContinuation;
}

const ExecutionState::StackFrame::ExceptionHandlers &
ExecutionState::StackFrame::getExceptionHandlers() const {
    return exceptionHandlers;
}

const NamespaceContext *ExecutionState::StackFrame::getNameSpaces() const { return namespaces; }

ExecutionState::ExecutionState(const IR::P4Program *program)
    : AbstractExecutionState(program),
      body({program}),
      stack(*(new std::stack<std::reference_wrapper<const StackFrame>>())) {
    env.set(&PacketVars::INPUT_PACKET_LABEL, IR::getConstant(IR::getBitType(0), 0));
    env.set(&PacketVars::PACKET_BUFFER_LABEL, IR::getConstant(IR::getBitType(0), 0));
    // We also add the taint property and set it to false.
    setProperty("inUndefinedState", false);
    // Drop is initialized to false, too.
    setProperty("drop", false);
    // If a user-pattern is provided, initialize the reachability engine state.
    if (!TestgenOptions::get().pattern.empty()) {
        reachabilityEngineState = ReachabilityEngineState::getInitial();
    }
    // If assertion mode is enabled, set the assertion property to false.
    if (TestgenOptions::get().assertionModeEnabled) {
        setProperty("assertionTriggered", false);
    }
}

ExecutionState::ExecutionState(Continuation::Body body)
    : body(std::move(body)), stack(*(new std::stack<std::reference_wrapper<const StackFrame>>())) {
    // We also add the taint property and set it to false.
    setProperty("inUndefinedState", false);
    // Drop is initialized to false, too.
    setProperty("drop", false);
    // If a user-pattern is provided, initialize the reachability engine state.
    if (!TestgenOptions::get().pattern.empty()) {
        reachabilityEngineState = ReachabilityEngineState::getInitial();
    }
}

ExecutionState &ExecutionState::create(const IR::P4Program *program) {
    return *new ExecutionState(program);
}

ExecutionState &ExecutionState::clone() const { return *new ExecutionState(*this); }

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

bool ExecutionState::isTerminal() const { return body.empty() && stack.empty(); }

const std::vector<uint64_t> &ExecutionState::getSelectedBranches() const {
    return selectedBranches;
}

const std::vector<const IR::Expression *> &ExecutionState::getPathConstraint() const {
    return pathConstraint;
}

std::optional<const Continuation::Command> ExecutionState::getNextCmd() const {
    if (body.empty()) {
        return std::nullopt;
    }
    return body.next();
}

const IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    auto varType = resolveType(var->type);

    // In some cases, we may reference a complex expression. Convert it to a struct expression.
    if (varType->is<IR::Type_StructLike>() || varType->to<IR::Type_Stack>()) {
        return convertToComplexExpression(var);
    }

    // TODO: This is a convoluted (and expensive?) check because struct members are not directly
    // associated with a header. We should be using runtime objects instead of flat assignments.
    if (const auto *member = var->to<IR::Member>()) {
        auto memberType = resolveType(member->expr->type);
        if (memberType->is<IR::Type_Header>() && member->member != ToolsVariables::VALID) {
            // If we are setting the member of a header, we need to check whether the
            // header is valid.
            // If the header is invalid, the get returns a tainted expression.
            // The member could have any value.
            auto validity = ToolsVariables::getHeaderValidity(member->expr);
            const auto *validVar = env.get(validity);
            // The validity bit could already be tainted or the validity bit is false.
            // Both outcomes lead to a tainted write.
            bool isTainted = validVar->is<IR::TaintExpression>();
            if (const auto *validBool = validVar->to<IR::BoolLiteral>()) {
                isTainted = !validBool->value;
            }
            if (isTainted) {
                return ToolsVariables::getTaintExpression(varType);
            }
        }
    }
    const auto *expr = env.get(var);
    return expr;
}

void ExecutionState::markVisited(const IR::Node *node) {
    // Only track statements, which have a valid source position in the P4 program.
    if (!node->getSourceInfo().isValid()) {
        return;
    }
    auto &coverageOptions = TestgenOptions::get().coverageOptions;
    // Do not add statements, if coverStatements is not toggled.
    if (node->is<IR::Statement>() && !coverageOptions.coverStatements) {
        return;
    }
    // Do not add table entries, if coverTableEntries is not toggled.
    if (node->is<IR::Entry>() && !coverageOptions.coverTableEntries) {
        return;
    }
    // Do not add actions, if coverActions is not toggled.
    if (node->is<IR::P4Action>() && !coverageOptions.coverActions) {
        return;
    }
    visitedNodes.emplace(node);
}

const P4::Coverage::CoverageSet &ExecutionState::getVisited() const { return visitedNodes; }

/// Compare types, considering Extracted_Varbit and bits equal if the (real/extracted) sizes are
/// equal. This is because the packet expression can be something like 0 ++
/// (Extracted_Varbit<N>)pkt_var. This expression is typed as bit<N>, but the optimizer removes the
/// 0 ++ and makes it into Extracted_Varbit type.
/// TODO: Maybe there is a better way to handle varbit that could allow us to avoid this.
static bool typeEquivSansVarbit(const IR::Type *a, const IR::Type *b) {
    if (a->equiv(*b)) {
        return true;
    }
    const auto *abit = a->to<IR::Type_Bits>();
    const auto *avar = a->to<IR::Extracted_Varbits>();
    const auto *bbit = b->to<IR::Type_Bits>();
    const auto *bvar = b->to<IR::Extracted_Varbits>();
    return (abit && bvar && abit->width_bits() == bvar->width_bits()) ||
           (avar && bbit && avar->width_bits() == bbit->width_bits());
}

void ExecutionState::set(const IR::StateVariable &var, const IR::Expression *value) {
    const auto *type = value->type;
    BUG_CHECK(type && !type->is<IR::Type_Unknown>(), "Cannot set value with unspecified type: %1%",
              value);
    if (getProperty<bool>("inUndefinedState")) {
        // If we are in an undefined state, the variable we set is tainted.
        value = ToolsVariables::getTaintExpression(type);
    } else {
        value = P4::optimizeExpression(value);
        BUG_CHECK(value->type && !value->type->is<IR::Type_Unknown>(),
                  "The P4 expression optimizer stripped a type of %1% (was %2%)", value, type);
        BUG_CHECK(typeEquivSansVarbit(type, value->type),
                  "The P4 expression optimizer had changed type of %1% (%2% -> %3%)", value, type,
                  value->type);
    }
    env.set(var, value);
}

const std::vector<std::reference_wrapper<const TraceEvent>> &ExecutionState::getTrace() const {
    return trace;
}

const Continuation::Body &ExecutionState::getBody() const { return body; }

const std::stack<std::reference_wrapper<const ExecutionState::StackFrame>> &
ExecutionState::getStack() const {
    return stack;
}

void ExecutionState::setProperty(cstring propertyName, Continuation::PropertyValue property) {
    stateProperties[propertyName] = property;
}

bool ExecutionState::hasProperty(cstring propertyName) const {
    return stateProperties.count(propertyName) > 0;
}

void ExecutionState::addTestObject(cstring category, cstring objectLabel,
                                   const TestObject *object) {
    testObjects[category][objectLabel] = object;
}

const TestObject *ExecutionState::getTestObject(cstring category, cstring objectLabel,
                                                bool checked) const {
    auto testObjectCategory = getTestObjectCategory(category);
    auto it = testObjectCategory.find(objectLabel);
    if (it != testObjectCategory.end()) {
        return it->second;
    }
    if (checked) {
        BUG("Unable to find test object with the label %1% in the category %2%. ", objectLabel,
            category);
    }
    return nullptr;
}

TestObjectMap ExecutionState::getTestObjectCategory(cstring category) const {
    auto it = testObjects.find(category);
    if (it != testObjects.end()) {
        return it->second;
    }
    return {};
}

void ExecutionState::deleteTestObject(cstring category, cstring objectLabel) {
    auto it = testObjects.find(category);
    if (it != testObjects.end()) {
        it->second.erase(objectLabel);
    }
}

void ExecutionState::deleteTestObjectCategory(cstring category) { testObjects.erase(category); }

void ExecutionState::setReachabilityEngineState(ReachabilityEngineState *newEngineState) {
    reachabilityEngineState = newEngineState;
}

ReachabilityEngineState *ExecutionState::getReachabilityEngineState() const {
    return reachabilityEngineState;
}

/* =============================================================================================
 *  Trace events.
 * ============================================================================================= */

void ExecutionState::add(const TraceEvent &event) { trace.emplace_back(event); }

void ExecutionState::popBody() { body.pop(); }

void ExecutionState::replaceBody(const Continuation::Body &body) { this->body = body; }

void ExecutionState::pushContinuation(const StackFrame &frame) { stack.push(frame); }

void ExecutionState::pushCurrentContinuation(StackFrame::ExceptionHandlers handlers) {
    pushCurrentContinuation(std::nullopt, std::move(handlers));
}

void ExecutionState::pushCurrentContinuation(std::optional<const IR::Type *> parameterType_opt,
                                             StackFrame::ExceptionHandlers handlers) {
    // If we were given a void parameter type, treat that the same as std::nullopt.
    if (parameterType_opt && IR::Type::Void::get()->equiv(**parameterType_opt)) {
        parameterType_opt = std::nullopt;
    }

    // Create the optional parameter.
    std::optional<const Continuation::Parameter *> parameterOpt = std::nullopt;
    if (parameterType_opt) {
        const auto *parameter =
            Continuation::genParameter(*parameterType_opt, "_", getNamespaceContext());
        parameterOpt = parameter;
    }

    // Actually push the current continuation.
    Continuation k(parameterOpt, body);
    const auto *frame = new StackFrame(k, std::move(handlers), namespaces);
    pushContinuation(*frame);
    body.clear();
}

void ExecutionState::popContinuation(std::optional<const IR::Node *> argument_opt) {
    BUG_CHECK(!stack.empty(), "Popped an empty continuation stack");
    auto frame = stack.top();
    stack.pop();

    auto newBody = frame.get().getContinuation().apply(argument_opt);
    replaceBody(newBody);
    setNamespaceContext(frame.get().getNameSpaces());
}

void ExecutionState::handleException(Continuation::Exception e) {
    while (!stack.empty()) {
        auto frame = stack.top();
        if (frame.get().getExceptionHandlers().count(e) > 0) {
            auto k = frame.get().getExceptionHandlers().at(e);
            auto newBody = k.apply(std::nullopt);
            replaceBody(newBody);
            setNamespaceContext(frame.get().getNameSpaces());
            return;
        }
        stack.pop();
    }
    BUG("Did not find exception handler for %s.", e);
}

/* =============================================================================================
 *  Body operations
 * ============================================================================================= */

void ExecutionState::replaceTopBody(const Continuation::Command &cmd) { replaceTopBody({cmd}); }

void ExecutionState::replaceTopBody(const std::vector<Continuation::Command> *cmds) {
    BUG_CHECK(!cmds->empty(), "Replaced top of execution stack with empty list");
    popBody();
    for (auto it = cmds->rbegin(); it != cmds->rend(); ++it) {
        body.push(*it);
    }
}

void ExecutionState::replaceTopBody(std::initializer_list<Continuation::Command> cmds) {
    std::vector<Continuation::Command> cmdList(cmds);
    replaceTopBody(&cmdList);
}

/* =============================================================================================
 *  Packet manipulation
 * ============================================================================================= */

void ExecutionState::pushPathConstraint(const IR::Expression *e) { pathConstraint.push_back(e); }

void ExecutionState::pushBranchDecision(uint64_t bIdx) { selectedBranches.push_back(bIdx); }

const IR::SymbolicVariable *ExecutionState::getInputPacketSizeVar() {
    return ToolsVariables::getSymbolicVariable(&PacketVars::PACKET_SIZE_VAR_TYPE,
                                               "*packetLen_bits");
}

int ExecutionState::getMaxPacketLength() { return TestgenOptions::get().maxPktSize; }

const IR::Expression *ExecutionState::getInputPacket() const {
    return env.get(&PacketVars::INPUT_PACKET_LABEL);
}

int ExecutionState::getInputPacketSize() const {
    const auto *inputPkt = getInputPacket();
    return inputPkt->type->width_bits();
}

void ExecutionState::appendToInputPacket(const IR::Expression *expr) {
    const auto *inputPkt = getInputPacket();
    const auto *width = IR::getBitType(expr->type->width_bits() + inputPkt->type->width_bits());
    const auto *concat = new IR::Concat(width, inputPkt, expr);
    env.set(&PacketVars::INPUT_PACKET_LABEL, concat);
}

void ExecutionState::prependToInputPacket(const IR::Expression *expr) {
    const auto *inputPkt = getInputPacket();
    const auto *width = IR::getBitType(expr->type->width_bits() + inputPkt->type->width_bits());
    const auto *concat = new IR::Concat(width, expr, inputPkt);
    env.set(&PacketVars::INPUT_PACKET_LABEL, concat);
}

int ExecutionState::getInputPacketCursor() const { return inputPacketCursor; }

const IR::Expression *ExecutionState::getPacketBuffer() const {
    return env.get(&PacketVars::PACKET_BUFFER_LABEL);
}

int ExecutionState::getPacketBufferSize() const {
    const auto *buffer = getPacketBuffer();
    return buffer->type->width_bits();
}

const IR::Expression *ExecutionState::peekPacketBuffer(int amount) {
    BUG_CHECK(amount > 0, "Peeked amount \"%1%\" should be larger than 0.", amount);

    const auto *buffer = getPacketBuffer();
    auto bufferSize = buffer->type->width_bits();

    auto diff = amount - bufferSize;
    const auto *amountType = IR::getBitType(amount);
    // We are running off the available buffer, we need to generate new packet content.
    if (diff > 0) {
        // We need to enlarge the input packet by the amount we are exceeding the buffer.
        // TODO: How should we perform accounting here?
        const IR::Expression *newVar = createPacketVariable(IR::getBitType(diff));
        appendToInputPacket(newVar);
        // If the buffer was not empty, append the data we have consumed to the newly generated
        // content and reset the buffer.
        if (bufferSize > 0) {
            auto *slice = new IR::Slice(buffer, bufferSize - 1, 0);
            slice->type = IR::getBitType(amount);
            newVar = new IR::Concat(amountType, slice, newVar);
            resetPacketBuffer();
        }
        // We have peeked ahead of what is available. We need to add the content we have looked at
        // to our packet buffer, which can later be consumed.
        env.set(&PacketVars::PACKET_BUFFER_LABEL, newVar);
        return newVar;
    }
    // The buffer is large enough and we can grab a slice
    auto *slice = new IR::Slice(buffer, bufferSize - 1, bufferSize - amount);
    slice->type = amountType;
    return slice;
}

const IR::Expression *ExecutionState::slicePacketBuffer(int amount) {
    BUG_CHECK(amount > 0, "Sliced amount \"%1%\" should be larger than 0.", amount);

    const auto *buffer = getPacketBuffer();
    auto bufferSize = buffer->type->width_bits();
    const auto *inputPkt = getInputPacket();
    auto inputPktSize = inputPkt->type->width_bits();

    // If the cursor is behind the current available packet, move it ahead by either the amount we
    // are slicing or until we are equal to the current program packet size.
    if (inputPacketCursor < inputPktSize) {
        inputPacketCursor += std::min(amount, inputPktSize - inputPacketCursor);
    }

    // Compute the difference between what we have in the buffer and what we want to slice.
    auto diff = amount - bufferSize;
    const auto *amountType = IR::getBitType(amount);
    // We are running off the available buffer, we need to generate new packet content.
    if (diff > 0) {
        // We need to enlarge the input packet by the amount we are exceeding the buffer.
        // TODO: How should we perform accounting here?
        const IR::Expression *newVar = createPacketVariable(IR::getBitType(diff));
        appendToInputPacket(newVar);
        // If the buffer was not empty, append the data we have consumed to the newly generated
        // content and reset the buffer.
        if (bufferSize > 0) {
            auto *slice = new IR::Slice(buffer, bufferSize - 1, 0);
            slice->type = IR::getBitType(amount);
            newVar = new IR::Concat(amountType, slice, newVar);
            resetPacketBuffer();
        }
        // Advance the cursor for bookkeeping.
        inputPacketCursor += diff;
        return newVar;
    }
    // The buffer is large enough and we can grab a slice
    auto *slice = new IR::Slice(buffer, bufferSize - 1, bufferSize - amount);
    slice->type = amountType;
    // If the buffer is larger, update the buffer with its remainder.
    if (diff < 0) {
        auto *remainder = new IR::Slice(buffer, bufferSize - amount - 1, 0);
        remainder->type = IR::getBitType(bufferSize - amount);
        env.set(&PacketVars::PACKET_BUFFER_LABEL, remainder);
    }
    // The amount we slice is equal to what is in the buffer. Just set the buffer to zero.
    if (diff == 0) {
        resetPacketBuffer();
    }
    return slice;
}

void ExecutionState::appendToPacketBuffer(const IR::Expression *expr) {
    const auto *buffer = getPacketBuffer();
    const auto *width = IR::getBitType(expr->type->width_bits() + buffer->type->width_bits());
    const auto *concat = new IR::Concat(width, buffer, expr);
    env.set(&PacketVars::PACKET_BUFFER_LABEL, concat);
}

void ExecutionState::prependToPacketBuffer(const IR::Expression *expr) {
    const auto *buffer = getPacketBuffer();
    const auto *width = IR::getBitType(expr->type->width_bits() + buffer->type->width_bits());
    const auto *concat = new IR::Concat(width, expr, buffer);
    env.set(&PacketVars::PACKET_BUFFER_LABEL, concat);
}

void ExecutionState::resetPacketBuffer() {
    env.set(&PacketVars::PACKET_BUFFER_LABEL, IR::getConstant(IR::getBitType(0), 0));
}

const IR::Expression *ExecutionState::getEmitBuffer() const {
    return env.get(&PacketVars::EMIT_BUFFER_LABEL);
}

void ExecutionState::resetEmitBuffer() {
    env.set(&PacketVars::EMIT_BUFFER_LABEL, IR::getConstant(IR::getBitType(0), 0));
}

void ExecutionState::appendToEmitBuffer(const IR::Expression *expr) {
    const auto *buffer = getEmitBuffer();
    const auto *width = IR::getBitType(expr->type->width_bits() + buffer->type->width_bits());
    const auto *concat = new IR::Concat(width, buffer, expr);
    env.set(&PacketVars::EMIT_BUFFER_LABEL, concat);
}

void ExecutionState::setParserErrorLabel(const IR::StateVariable &parserError) {
    parserErrorLabel.emplace(parserError);
}

const IR::StateVariable &ExecutionState::getCurrentParserErrorLabel() const {
    BUG_CHECK(parserErrorLabel.has_value(), "Parser error label has not been set.");
    return parserErrorLabel.value();
}

/* =============================================================================================
 *  Variables and symbolic constants
 * ============================================================================================= */

const IR::SymbolicVariable *ExecutionState::createPacketVariable(const IR::Type *type) {
    const auto *variable = ToolsVariables::getSymbolicVariable(
        type, "pktvar_" + std::to_string(numAllocatedPacketVariables));
    numAllocatedPacketVariables++;
    BUG_CHECK(numAllocatedPacketVariables < UINT16_MAX,
              "Allocating too many symbolic packet variables. The symbolic variable counter with "
              "maximum size %1% counter will overflow.",
              UINT16_MAX);
    return variable;
}

}  // namespace P4Tools::P4Testgen
