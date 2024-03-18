#include "backends/p4tools/common/lib/trace_event_types.h"

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "lib/source_file.h"

namespace P4Tools::TraceEvents {

/* =============================================================================================
 *   Generic
 * ============================================================================================= */

Generic::Generic(cstring label) : label(label) {}

void Generic::print(std::ostream &os) const { os << "[" << label << "]"; }

/* =============================================================================================
 *   GenericDescription
 * ============================================================================================= */

GenericDescription::GenericDescription(cstring label, cstring description)
    : Generic(label), description(description) {}

void GenericDescription::print(std::ostream &os) const {
    Generic::print(os);
    os << ": " << description;
}
/* =============================================================================================
 *   Expression
 * ============================================================================================= */

Expression::Expression(const IR::Expression *value, cstring label) : Generic(label), value(value) {}

const Expression *Expression::subst(const SymbolicEnv &env) const {
    return new Expression(env.subst(value), label);
}

const Expression *Expression::apply(Transform &visitor) const {
    return new Expression(value->apply(visitor), label);
}

const Expression *Expression::evaluate(const Model &model, bool doComplete) const {
    const IR::Expression *evaluatedValue = nullptr;
    if (const auto *structExpr = value->to<IR::StructExpression>()) {
        evaluatedValue = model.evaluateStructExpr(structExpr, doComplete);
    } else if (const auto *listExpr = value->to<IR::BaseListExpression>()) {
        evaluatedValue = model.evaluateListExpr(listExpr, doComplete);
    } else if (Taint::hasTaint(value)) {
        evaluatedValue = &Taint::TAINTED_STRING_LITERAL;
    } else {
        evaluatedValue = model.evaluate(value, doComplete);
    }
    return new Expression(evaluatedValue, label);
}

void Expression::print(std::ostream &os) const {
    // Convert the assignment to a string and strip any new lines.
    // TODO: Maybe there is a better way to format newlines?
    std::stringstream exprStream;
    value->dbprint(exprStream);
    auto expString = exprStream.str();
    expString.erase(std::remove(expString.begin(), expString.end(), '\n'), expString.cend());
    Generic::print(os);
    os << ": " << expString;
}

/* =============================================================================================
 *   MethodCall
 * ============================================================================================= */

MethodCall::MethodCall(const IR::MethodCallExpression *call) : call(call) {}

void MethodCall::print(std::ostream &os) const {
    const auto &srcInfo = call->getSourceInfo();
    // Convert the method call to a string and strip any new lines.
    std::stringstream callStream;
    call->dbprint(callStream);
    auto callString = callStream.str();
    callString.erase(std::remove(callString.begin(), callString.end(), '\n'), callString.cend());
    if (srcInfo.isValid()) {
        os << "[MethodCall]: " << callString;
    } else {
        os << "[P4Testgen MethodCall]: " << callString;
    }
}

/* =============================================================================================
 *   IfStatementCondition
 * ============================================================================================= */

IfStatementCondition::IfStatementCondition(const IR::Expression *cond) : postEvalCond(cond) {}

void IfStatementCondition::setPreEvalCond(const IR::Expression *cond) { preEvalCond = cond; }

const IfStatementCondition *IfStatementCondition::subst(const SymbolicEnv &env) const {
    auto *traceEvent = new IfStatementCondition(env.subst(postEvalCond));
    traceEvent->setPreEvalCond(postEvalCond);
    return traceEvent;
}

const IfStatementCondition *IfStatementCondition::apply(Transform &visitor) const {
    auto *traceEvent = new IfStatementCondition(postEvalCond->apply(visitor));
    traceEvent->setPreEvalCond(postEvalCond);
    return traceEvent;
}

const IfStatementCondition *IfStatementCondition::evaluate(const Model &model,
                                                           bool doComplete) const {
    const IR::Literal *evaluatedPostVal = nullptr;
    if (Taint::hasTaint(postEvalCond)) {
        evaluatedPostVal = &Taint::TAINTED_STRING_LITERAL;
    } else {
        evaluatedPostVal = model.evaluate(postEvalCond, doComplete);
    }
    auto *traceEvent = new IfStatementCondition(evaluatedPostVal);
    traceEvent->setPreEvalCond(postEvalCond);
    return traceEvent;
}

void IfStatementCondition::print(std::ostream &os) const {
    if (postEvalCond->is<IR::StringLiteral>()) {
        os << "[Tainted If Statement]: " << postEvalCond->getSourceInfo().toBriefSourceFragment();
        return;
    }
    CHECK_NULL(preEvalCond);
    const auto &srcInfo = preEvalCond->getSourceInfo();
    if (srcInfo.isValid()) {
        os << "[If Statement]: " << srcInfo.toBriefSourceFragment();
    } else {
        os << "[P4Testgen If Statement]: " << preEvalCond;
    }
    os << " Condition: " << preEvalCond;
    const auto *boolResult = postEvalCond->checkedTo<IR::BoolLiteral>()->value ? "true" : "false";
    os << " Result: " << boolResult;
}

/* =============================================================================================
 *   AssignmentStatement
 * ============================================================================================= */

AssignmentStatement::AssignmentStatement(const IR::AssignmentStatement &stmt) : stmt(stmt) {}

const AssignmentStatement *AssignmentStatement::subst(const SymbolicEnv &env) const {
    const auto *right = env.subst(stmt.right);
    auto *traceEvent =
        new AssignmentStatement(*new IR::AssignmentStatement(stmt.srcInfo, stmt.left, right));
    return traceEvent;
}

const AssignmentStatement *AssignmentStatement::apply(Transform &visitor) const {
    const auto *right = stmt.right->apply(visitor);
    auto *traceEvent =
        new AssignmentStatement(*new IR::AssignmentStatement(stmt.srcInfo, stmt.left, right));
    return traceEvent;
}

const AssignmentStatement *AssignmentStatement::evaluate(const Model &model,
                                                         bool doComplete) const {
    const IR::Expression *right = nullptr;
    if (const auto *structExpr = stmt.right->to<IR::StructExpression>()) {
        right = model.evaluateStructExpr(structExpr, doComplete);
    } else if (const auto *listExpr = stmt.right->to<IR::BaseListExpression>()) {
        right = model.evaluateListExpr(listExpr, doComplete);
    } else if (Taint::hasTaint(stmt.right)) {
        right = &Taint::TAINTED_STRING_LITERAL;
    } else {
        right = model.evaluate(stmt.right, doComplete);
    }

    auto *traceEvent =
        new AssignmentStatement(*new IR::AssignmentStatement(stmt.srcInfo, stmt.left, right));
    return traceEvent;
}

void AssignmentStatement::print(std::ostream &os) const {
    const auto &srcInfo = stmt.getSourceInfo();
    // Convert the assignment to a string and strip any new lines.
    // TODO: Maybe there is a better way to format newlines?
    std::stringstream assignStream;
    stmt.dbprint(assignStream);
    auto assignString = assignStream.str();
    assignString.erase(std::remove(assignString.begin(), assignString.end(), '\n'),
                       assignString.cend());
    if (srcInfo.isValid()) {
        auto fragment = srcInfo.toSourceFragment(false);
        fragment = fragment.trim();
        os << "[AssignmentStatement]: " << fragment << "| Computed: " << assignString;
    } else {
        os << "[P4Testgen AssignmentStatement]: " << assignString;
    }
}

/* =============================================================================================
 *   ExtractSuccess
 * ============================================================================================= */

ExtractSuccess::ExtractSuccess(
    const IR::Expression *extractedHeader, int offset, const IR::Expression *condition,
    std::vector<std::pair<IR::StateVariable, const IR::Expression *>> fields)
    : extractedHeader(extractedHeader),
      offset(offset),
      condition(condition),
      fields(std::move(fields)) {}

const ExtractSuccess *ExtractSuccess::subst(const SymbolicEnv &env) const {
    std::vector<std::pair<IR::StateVariable, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        applyFields.emplace_back(field.first, env.subst(field.second));
    }
    return new ExtractSuccess(extractedHeader, offset, condition, applyFields);
}

const ExtractSuccess *ExtractSuccess::apply(Transform &visitor) const {
    std::vector<std::pair<IR::StateVariable, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        applyFields.emplace_back(field.first, field.second->apply(visitor));
    }
    return new ExtractSuccess(extractedHeader, offset, condition, applyFields);
}

const ExtractSuccess *ExtractSuccess::evaluate(const Model &model, bool doComplete) const {
    std::vector<std::pair<IR::StateVariable, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        if (Taint::hasTaint(field.second)) {
            applyFields.emplace_back(field.first, &Taint::TAINTED_STRING_LITERAL);
        } else {
            applyFields.emplace_back(field.first, model.evaluate(field.second, doComplete));
        }
    }
    return new ExtractSuccess(extractedHeader, offset, condition, applyFields);
}

void ExtractSuccess::print(std::ostream &os) const {
    // Using toString produces cleaner output.
    os << "[ExtractSuccess] " << extractedHeader->toString() << "@" << offset
       << " | Condition: " << condition
       << " | Extract Size: " << extractedHeader->type->width_bits() << " -> ";
    for (const auto &field : fields) {
        os << field.first->toString() << " = " << formatHexExpr(field.second, {true});
        if (field != fields.back()) {
            os << " | ";
        }
    }
}

int ExtractSuccess::getOffset() const { return offset; }

const IR::Expression *ExtractSuccess::getExtractedHeader() const { return extractedHeader; }

/* =============================================================================================
 *   ExtractFailure
 * ============================================================================================= */

ExtractFailure::ExtractFailure(const IR::Expression *extractedHeader, int offset,
                               const IR::Expression *condition)
    : extractedHeader(extractedHeader), offset(offset), condition(condition) {}

void ExtractFailure::print(std::ostream &os) const {
    // Using toString produces cleaner output.
    os << "[ExtractFailure] " << extractedHeader->toString() << "@" << offset
       << " | Condition: " << condition
       << " | Extract Size: " << extractedHeader->type->width_bits();
}

/* =============================================================================================
 *   Emit
 * ============================================================================================= */

Emit::Emit(const IR::HeaderExpression *emitHeader) : emitHeader(emitHeader) {}

const Emit *Emit::subst(const SymbolicEnv &env) const {
    std::vector<std::pair<IR::StateVariable, const IR::Expression *>> applyFields;
    return new Emit(env.subst(emitHeader)->checkedTo<IR::HeaderExpression>());
}

const Emit *Emit::apply(Transform &visitor) const {
    return new Emit(emitHeader->apply(visitor)->checkedTo<IR::HeaderExpression>());
}

const Emit *Emit::evaluate(const Model &model, bool doComplete) const {
    return new Emit(
        model.evaluateStructExpr(emitHeader, doComplete)->checkedTo<IR::HeaderExpression>());
}

void Emit::print(std::ostream &os) const {
    // Convert the header expression to a string and strip any new lines.
    // TODO: Maybe there is a better way to format newlines?
    std::stringstream assignStream;
    emitHeader->dbprint(assignStream);
    auto headerString = assignStream.str();
    headerString.erase(std::remove(headerString.begin(), headerString.end(), '\n'),
                       headerString.cend());
    os << "[Emit]: " << headerString;
}

/* =============================================================================================
 *   Packet
 * ============================================================================================= */

Packet::Packet(const Packet::Direction &direction, const IR::Expression *packetValue)
    : direction(direction), packetValue(packetValue) {}

const Packet *Packet::subst(const SymbolicEnv &env) const {
    return new Packet(direction, env.subst(packetValue));
}

const Packet *Packet::apply(Transform &visitor) const {
    return new Packet(direction, packetValue->apply(visitor));
}

const Packet *Packet::evaluate(const Model &model, bool doComplete) const {
    if (Taint::hasTaint(packetValue)) {
        return new Packet(direction, &Taint::TAINTED_STRING_LITERAL);
    }
    return new Packet(direction, model.evaluate(packetValue, doComplete));
}

void Packet::print(std::ostream &os) const {
    os << "[Packet " << direction << "] " << formatHexExpr(packetValue, {true});
}

std::ostream &operator<<(std::ostream &os, const Packet::Direction &direction) {
    switch (direction) {
        case Packet::Direction::In:
            os << "in";
            break;

        case Packet::Direction::Out:
            os << "out";
            break;

        default:
            BUG("Unknown packet direction");
    }
    return os;
}

/* =============================================================================================
 *   ParserStart
 * ============================================================================================= */

ParserStart::ParserStart(const IR::P4Parser *parser) : parser(parser) {}

void ParserStart::print(std::ostream &os) const { os << "[Parser] " << parser->controlPlaneName(); }

/* =============================================================================================
 *   ParserState
 * ============================================================================================= */

ParserState::ParserState(const IR::ParserState *state) : state(state) {}

void ParserState::print(std::ostream &os) const { os << "[State] " << state->controlPlaneName(); }

const IR::ParserState *ParserState::getParserState() const { return state; }

}  // namespace P4Tools::TraceEvents
