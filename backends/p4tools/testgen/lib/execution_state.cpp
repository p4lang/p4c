#include "backends/p4tools/testgen/lib/execution_state.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <stack>
#include <utility>
#include <vector>

#include <boost/variant/variant.hpp>

#include "backends/p4tools/common/compiler/hs_index_simplify.h"
#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/taint.h"
#include "lib/log.h"
#include "lib/null.h"

#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

const IR::Member ExecutionState::inputPacketLabel =
    IR::Member(new IR::PathExpression("*"), "inputPacket");

const IR::Member ExecutionState::packetBufferLabel =
    IR::Member(new IR::PathExpression("*"), "packetBuffer");

const IR::Member ExecutionState::emitBufferLabel =
    IR::Member(new IR::PathExpression("*"), "emitBuffer");

const IR::Member ExecutionState::payloadLabel = IR::Member(new IR::PathExpression("*"), "payload");

// The methods in P4's packet_in uses 32-bit values. Conform with this to make it easier to produce
// well-typed expressions when manipulating the parser cursor.
const IR::Type_Bits ExecutionState::packetSizeVarType = IR::Type_Bits(32, false);

ExecutionState::ExecutionState(const IR::P4Program* program)
    : namespaces(NamespaceContext::Empty->push(program)),
      body({program}),
      stack(*new std::stack<gsl::not_null<const StackFrame*>>()) {
    // Insert the default zombies of the execution state.
    // This also makes the zombies present in the state explicit.
    allocatedZombies.insert(getInputPacketSizeVar());
    env.set(&inputPacketLabel, IRUtils::getConstant(IRUtils::getBitType(0), 0));
    env.set(&packetBufferLabel, IRUtils::getConstant(IRUtils::getBitType(0), 0));
    // We also add the taint property and set it to false.
    setProperty("inUndefinedState", false);
    // Drop is initialized to false, too.
    setProperty("drop", false);
}

ExecutionState::ExecutionState(Continuation::Body body)
    : namespaces(NamespaceContext::Empty),
      body(std::move(body)),
      stack(*new std::stack<gsl::not_null<const StackFrame*>>()) {
    // Insert the default zombies of the execution state.
    // This also makes the zombies present in the state explicit.
    allocatedZombies.insert(getInputPacketSizeVar());
    // We also add the taint property and set it to false.
    setProperty("inUndefinedState", false);
    // Drop is initialized to false, too.
    setProperty("drop", false);
}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

bool ExecutionState::isTerminal() const { return body.empty() && stack.empty(); }

boost::optional<const Continuation::Command> ExecutionState::getNextCmd() const {
    if (body.empty()) {
        return boost::none;
    }
    return body.next();
}

const IR::Expression* ExecutionState::get(const StateVariable& var) const {
    const auto* expr = env.get(var);
    if (var->expr->type->is<IR::Type_Header>() && var->member != IRUtils::Valid) {
        // If we are setting the member of a header, we need to check whether the
        // header is valid.
        // If the header is invalid, the get returns a tainted expression.
        // The member could have any value.
        // TODO: This is a convoluted check. We should be using runtime objects instead of flat
        // assignments.
        auto validity = IRUtils::getHeaderValidity(var->expr);
        const auto* validVar = env.get(validity);
        // The validity bit could already be tainted or the validity bit is false.
        // Both outcomes lead to a tainted write.
        bool isTainted = validVar->is<IR::TaintExpression>();
        if (const auto* validBool = validVar->to<IR::BoolLiteral>()) {
            isTainted = !validBool->value;
        }
        if (isTainted) {
            return IRUtils::getTaintExpression(expr->type);
        }
    }
    return expr;
}

void ExecutionState::markVisited(const IR::Statement* stmt) { visitedStatements.push_back(stmt); }

const std::vector<const IR::Statement*>& ExecutionState::getVisited() const {
    return visitedStatements;
}

bool ExecutionState::hasTaint(const IR::Expression* expr) const {
    return Taint::hasTaint(env.getInternalMap(), expr);
}

bool ExecutionState::exists(const StateVariable& var) const { return env.exists(var); }

void ExecutionState::set(const StateVariable& var, const IR::Expression* value) {
    if (getProperty<bool>("inUndefinedState")) {
        // If we are in an undefined state, the variable we set is tainted.
        value = IRUtils::getTaintExpression(value->type);
    }
    env.set(var, value);
}

const SymbolicEnv& ExecutionState::getSymbolicEnv() const { return env; }

void ExecutionState::printSymbolicEnv(std::ostream& out) const {
    // TODO(fruffy): How do we do logging here?
    out << "##### Symbolic Environment Begin #####" << std::endl;
    for (const auto& env_var : env.getInternalMap()) {
        const auto var = env_var.first;
        const auto* val = env_var.second;
        out << "Variable: " << var->toString() << " Value: " << val << std::endl;
    }
    out << "##### Symbolic Environment End #####" << std::endl;
}

const std::vector<gsl::not_null<const TraceEvent*>>& ExecutionState::getTrace() const {
    return trace;
}

const Continuation::Body& ExecutionState::getBody() const { return body; }

const std::stack<gsl::not_null<const ExecutionState::StackFrame*>>& ExecutionState::getStack()
    const {
    return stack;
}

void ExecutionState::setProperty(cstring propertyName, Continuation::PropertyValue property) {
    stateProperties[propertyName] = std::move(property);
}

bool ExecutionState::hasProperty(cstring propertyName) const {
    return stateProperties.count(propertyName) > 0;
}

void ExecutionState::addTestObject(cstring category, cstring objectLabel,
                                   const TestObject* object) {
    testObjects[category][objectLabel] = object;
}

const TestObject* ExecutionState::getTestObject(cstring category, cstring objectLabel,
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

std::map<cstring, const TestObject*> ExecutionState::getTestObjectCategory(cstring category) const {
    auto it = testObjects.find(category);
    if (it != testObjects.end()) {
        return it->second;
    }
    return {};
}

/* =============================================================================================
 *  Trace events.
 * ============================================================================================= */

void ExecutionState::add(const TraceEvent* event) {
    CHECK_NULL(event);
    trace.emplace_back(event);
}

/* =============================================================================================
 *  Namespaces and declarations
 * ============================================================================================= */

const IR::IDeclaration* ExecutionState::findDecl(const IR::Path* path) const {
    return namespaces->findDecl(path);
}

const IR::IDeclaration* ExecutionState::findDecl(const IR::PathExpression* pathExpr) const {
    return findDecl(pathExpr->path);
}

const IR::Type* ExecutionState::resolveType(const IR::Type* type) const {
    const auto* typeName = type->to<IR::Type_Name>();
    // Nothing to resolve here. Just return.
    if (typeName == nullptr) {
        return type;
    }
    const auto* path = typeName->path;
    const auto* decl = findDecl(path)->to<IR::Type_Declaration>();
    BUG_CHECK(decl, "Not a type: %1%", path);
    return decl;
}

const NamespaceContext* ExecutionState::getNamespaceContext() const { return namespaces; }

void ExecutionState::setNamespaceContext(const NamespaceContext* namespaces) {
    this->namespaces = namespaces;
}

void ExecutionState::pushNamespace(const IR::INamespace* ns) { namespaces = namespaces->push(ns); }

void ExecutionState::popBody() { body.pop(); }

void ExecutionState::replaceBody(const Continuation::Body& body) { this->body = body; }

void ExecutionState::pushContinuation(gsl::not_null<const StackFrame*> frame) { stack.push(frame); }

void ExecutionState::pushCurrentContinuation(StackFrame::ExceptionHandlers handlers) {
    pushCurrentContinuation(boost::none, handlers);
}

void ExecutionState::pushCurrentContinuation(boost::optional<const IR::Type*> parameterType_opt,
                                             StackFrame::ExceptionHandlers handlers) {
    // If we were given a void parameter type, treat that the same as boost::none.
    if (parameterType_opt && IR::Type::Void::get()->equiv(**parameterType_opt)) {
        parameterType_opt = boost::none;
    }

    // Create the optional parameter.
    boost::optional<const Continuation::Parameter*> parameter_opt = boost::none;
    if (parameterType_opt) {
        const auto* parameter =
            Continuation::genParameter(*parameterType_opt, "_", getNamespaceContext());
        parameter_opt = boost::make_optional(parameter);
    }

    // Actually push the current continuation.
    Continuation k(parameter_opt, body);
    const auto* frame = new StackFrame(k, handlers, namespaces);
    pushContinuation(frame);
    body.clear();
}

void ExecutionState::popContinuation(boost::optional<const IR::Node*> argument_opt) {
    BUG_CHECK(!stack.empty(), "Popped an empty continuation stack");
    auto frame = stack.top();
    stack.pop();

    auto newBody = frame->normalContinuation.apply(argument_opt);
    replaceBody(newBody);
    setNamespaceContext(frame->namespaces);
}

void ExecutionState::handleException(Continuation::Exception e) {
    while (!stack.empty()) {
        auto frame = stack.top();
        if (frame->exceptionHandlers.count(e) > 0) {
            auto k = frame->exceptionHandlers.at(e);
            auto newBody = k.apply(boost::none);
            replaceBody(newBody);
            setNamespaceContext(frame->namespaces);
            return;
        }
        stack.pop();
    }
    BUG("Did not find exception handler for %s.", e);
}

/* =============================================================================================
 *  Body operations
 * ============================================================================================= */

void ExecutionState::replaceTopBody(const Continuation::Command cmd) { replaceTopBody({cmd}); }

void ExecutionState::replaceTopBody(const std::vector<Continuation::Command>* cmds) {
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

void ExecutionState::pushPathConstraint(const IR::Expression* e) { pathConstraint.push_back(e); }

void ExecutionState::pushBranchDecision(uint64_t bIdx) { selectedBranches.push_back(bIdx); }

const IR::Type_Bits* ExecutionState::getPacketSizeVarType() { return &packetSizeVarType; }

const StateVariable& ExecutionState::getInputPacketSizeVar() {
    return IRUtils::getZombieConst(getPacketSizeVarType(), 0, "*packetLen_bits");
}

int ExecutionState::getMaxPacketLength_bits() {
    return 8 * (TestgenOptions::get().networkMtu_bytes);
}

const IR::Expression* ExecutionState::getInputPacket() const { return env.get(&inputPacketLabel); }

int ExecutionState::getInputPacketSize() const {
    const auto* inputPkt = getInputPacket();
    return inputPkt->type->width_bits();
}

void ExecutionState::appendToInputPacket(const IR::Expression* expr) {
    const auto* inputPkt = getInputPacket();
    const auto* width =
        IRUtils::getBitType(expr->type->width_bits() + inputPkt->type->width_bits());
    const auto* concat = new IR::Concat(width, inputPkt, expr);
    env.set(&inputPacketLabel, concat);
}

void ExecutionState::prependToInputPacket(const IR::Expression* expr) {
    const auto* inputPkt = getInputPacket();
    const auto* width =
        IRUtils::getBitType(expr->type->width_bits() + inputPkt->type->width_bits());
    const auto* concat = new IR::Concat(width, expr, inputPkt);
    env.set(&inputPacketLabel, concat);
}

int ExecutionState::getInputPacketCursor() const { return inputPacketCursor; }

const IR::Expression* ExecutionState::getPacketBuffer() const {
    return env.get(&packetBufferLabel);
}

int ExecutionState::getPacketBufferSize() const {
    const auto* buffer = getPacketBuffer();
    return buffer->type->width_bits();
}

const IR::Expression* ExecutionState::peekPacketBuffer(int amount) {
    BUG_CHECK(amount > 0, "Peeked amount \"%1%\" should be larger than 0.", amount);

    const auto* buffer = getPacketBuffer();
    auto bufferSize = buffer->type->width_bits();

    auto diff = amount - bufferSize;
    const auto* amountType = IRUtils::getBitType(amount);
    // We are running off the available buffer, we need to generate new packet content.
    if (diff > 0) {
        // We need to enlarge the input packet by the amount we are exceeding the buffer.
        // TODO: How should we perform accounting here?
        const IR::Expression* newVar =
            createZombieConst(IRUtils::getBitType(diff), "pktVar", allocatedZombies.size());
        appendToInputPacket(newVar);
        // If the buffer was not empty, append the data we have consumed to the newly generated
        // content and reset the buffer.
        if (bufferSize > 0) {
            auto* slice = new IR::Slice(buffer, bufferSize - 1, 0);
            slice->type = IRUtils::getBitType(amount);
            newVar = new IR::Concat(amountType, slice, newVar);
            resetPacketBuffer();
        }
        // We have peeked ahead of what is available. We need to add the content we have looked at
        // to our packet buffer, which can later be consumed.
        env.set(&packetBufferLabel, newVar);
        return newVar;
    }
    // The buffer is large enough and we can grab a slice
    auto* slice = new IR::Slice(buffer, bufferSize - 1, bufferSize - amount);
    slice->type = amountType;
    return slice;
}

const IR::Expression* ExecutionState::slicePacketBuffer(int amount) {
    BUG_CHECK(amount > 0, "Sliced amount \"%1%\" should be larger than 0.", amount);

    const auto* buffer = getPacketBuffer();
    auto bufferSize = buffer->type->width_bits();
    const auto* inputPkt = getInputPacket();
    auto inputPktSize = inputPkt->type->width_bits();

    // If the cursor is behind the current available packet, move it ahead by either the amount we
    // are slicing or until we are equal to the current program packet size.
    if (inputPacketCursor < inputPktSize) {
        inputPacketCursor += std::min(amount, inputPktSize - inputPacketCursor);
    }

    // Compute the difference between what we have in the buffer and what we want to slice.
    auto diff = amount - bufferSize;
    const auto* amountType = IRUtils::getBitType(amount);
    // We are running off the available buffer, we need to generate new packet content.
    if (diff > 0) {
        // We need to enlarge the input packet by the amount we are exceeding the buffer.
        // TODO: How should we perform accounting here?
        const IR::Expression* newVar =
            createZombieConst(IRUtils::getBitType(diff), "pktVar", allocatedZombies.size());
        appendToInputPacket(newVar);
        // If the buffer was not empty, append the data we have consumed to the newly generated
        // content and reset the buffer.
        if (bufferSize > 0) {
            auto* slice = new IR::Slice(buffer, bufferSize - 1, 0);
            slice->type = IRUtils::getBitType(amount);
            newVar = new IR::Concat(amountType, slice, newVar);
            resetPacketBuffer();
        }
        // Advance the cursor for bookkeeping.
        inputPacketCursor += diff;
        return newVar;
    }
    // The buffer is large enough and we can grab a slice
    auto* slice = new IR::Slice(buffer, bufferSize - 1, bufferSize - amount);
    slice->type = amountType;
    // If the buffer is larger, update the buffer with its remainder.
    if (diff < 0) {
        auto* remainder = new IR::Slice(buffer, bufferSize - amount - 1, 0);
        remainder->type = IRUtils::getBitType(bufferSize - amount);
        env.set(&packetBufferLabel, remainder);
    }
    // The amount we slice is equal to what is in the buffer. Just set the buffer to zero.
    if (diff == 0) {
        resetPacketBuffer();
    }
    return slice;
}

void ExecutionState::appendToPacketBuffer(const IR::Expression* expr) {
    const auto* buffer = getPacketBuffer();
    const auto* width = IRUtils::getBitType(expr->type->width_bits() + buffer->type->width_bits());
    const auto* concat = new IR::Concat(width, buffer, expr);
    env.set(&packetBufferLabel, concat);
}

void ExecutionState::prependToPacketBuffer(const IR::Expression* expr) {
    const auto* buffer = getPacketBuffer();
    const auto* width = IRUtils::getBitType(expr->type->width_bits() + buffer->type->width_bits());
    const auto* concat = new IR::Concat(width, expr, buffer);
    env.set(&packetBufferLabel, concat);
}

void ExecutionState::resetPacketBuffer() {
    env.set(&packetBufferLabel, IRUtils::getConstant(IRUtils::getBitType(0), 0));
}

const IR::Expression* ExecutionState::getEmitBuffer() const { return env.get(&emitBufferLabel); }

void ExecutionState::resetEmitBuffer() {
    env.set(&emitBufferLabel, IRUtils::getConstant(IRUtils::getBitType(0), 0));
}

void ExecutionState::appendToEmitBuffer(const IR::Expression* expr) {
    const auto* buffer = getEmitBuffer();
    const auto* width = IRUtils::getBitType(expr->type->width_bits() + buffer->type->width_bits());
    const auto* concat = new IR::Concat(width, buffer, expr);
    env.set(&emitBufferLabel, concat);
}

const IR::Member* ExecutionState::getPayloadLabel(const IR::Type* t) {
    // If the type is not null, we construct a member clone with the input type set.
    if (t != nullptr) {
        auto* label = payloadLabel.clone();
        label->type = t;
        return label;
    }
    return &payloadLabel;
}

void ExecutionState::setParserErrorLabel(const IR::Member* parserError) {
    parserErrorLabel = parserError;
}

const IR::Member* ExecutionState::getCurrentParserErrorLabel() const {
    CHECK_NULL(parserErrorLabel);
    return parserErrorLabel;
}

/* =============================================================================================
 *  Variables and symbolic constants
 * ============================================================================================= */

const std::set<StateVariable>& ExecutionState::getZombies() const { return allocatedZombies; }

const StateVariable& ExecutionState::createZombieConst(const IR::Type* type, cstring name,
                                                       uint64_t instanceId) {
    const auto& zombie = IRUtils::getZombieConst(type, instanceId, name);
    const auto& result = allocatedZombies.insert(zombie);
    // The zombie already existed, check its type.
    if (!result.second) {
        BUG_CHECK((*result.first)->type->equiv(*type),
                  "Inconsistent types for zombie variable %1%: previously %2%, but now %3%",
                  zombie->toString(), (*result.first)->type, type);
    }
    return zombie;
}

/* =========================================================================================
 *  General utilities involving ExecutionState.
 * ========================================================================================= */

std::vector<const IR::Member*> ExecutionState::getFlatFields(
    const IR::Expression* parent, const IR::Type_StructLike* ts,
    std::vector<const IR::Member*>* validVector) const {
    std::vector<const IR::Member*> flatFields;
    for (const auto* field : ts->fields) {
        const auto* fieldType = resolveType(field->type);
        if (const auto* ts = fieldType->to<IR::Type_StructLike>()) {
            auto subFields =
                getFlatFields(new IR::Member(fieldType, parent, field->name), ts, validVector);
            flatFields.insert(flatFields.end(), subFields.begin(), subFields.end());
        } else if (const auto* typeStack = fieldType->to<IR::Type_Stack>()) {
            const auto* stackElementsType = resolveType(typeStack->elementType);
            for (size_t arrayIndex = 0; arrayIndex < typeStack->getSize(); arrayIndex++) {
                const auto* newMember = HSIndexToMember::produceStackIndex(
                    stackElementsType, new IR::Member(typeStack, parent, field->name), arrayIndex);
                BUG_CHECK(stackElementsType->is<IR::Type_StructLike>(),
                          "Try to make the flat fields for non Type_StructLike element : %1%",
                          stackElementsType);
                auto subFields = getFlatFields(
                    newMember, stackElementsType->to<IR::Type_StructLike>(), validVector);
                flatFields.insert(flatFields.end(), subFields.begin(), subFields.end());
            }
        } else {
            flatFields.push_back(new IR::Member(fieldType, parent, field->name));
        }
    }
    // If we are dealing with a header we also include the validity bit in the list of
    // fields.
    if (validVector != nullptr && ts->is<IR::Type_Header>()) {
        validVector->push_back(IRUtils::getHeaderValidity(parent));
    }
    return flatFields;
}

const IR::P4Table* ExecutionState::getTableType(const IR::Expression* expression) const {
    if (!expression->is<IR::Member>()) {
        return nullptr;
    }
    const auto* member = expression->to<IR::Member>();
    if (member->member != IR::IApply::applyMethodName) {
        return nullptr;
    }
    if (member->expr->is<IR::PathExpression>()) {
        const auto* declaration = findDecl(member->expr->to<IR::PathExpression>());
        return declaration->to<IR::P4Table>();
    }
    const auto* type = member->expr->type;
    if (const auto* tableType = type->to<IR::Type_Table>()) {
        return tableType->table;
    }
    return nullptr;
}

const IR::P4Action* ExecutionState::getActionDecl(const IR::Expression* expression) const {
    if (const auto* path = expression->to<IR::PathExpression>()) {
        const auto* declaration = findDecl(path);
        BUG_CHECK(declaration, "Can't find declaration %1%", path);
        return declaration->to<IR::P4Action>();
    }
    return expression->to<IR::P4Action>();
}

const StateVariable& ExecutionState::convertPathExpr(const IR::PathExpression* path) const {
    const auto* decl = findDecl(path)->getNode();
    // Local variable.
    if (const auto* declVar = decl->to<IR::Declaration_Variable>()) {
        return IRUtils::getZombieVar(path->type, 0, declVar->name.name);
    }
    if (const auto* declInst = decl->to<IR::Declaration_Instance>()) {
        return IRUtils::getZombieVar(path->type, 0, declInst->name.name);
    }
    if (const auto* param = decl->to<IR::Parameter>()) {
        return IRUtils::getZombieVar(path->type, 0, param->name.name);
    }
    BUG("Unsupported declaration %1% of type %2%.", decl, decl->node_type_name());
}

}  // namespace P4Testgen

}  // namespace P4Tools
