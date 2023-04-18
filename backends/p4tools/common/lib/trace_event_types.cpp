#include "backends/p4tools/common/lib/trace_event_types.h"

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "ir/vector.h"
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
 *   Expression
 * ============================================================================================= */

Expression::Expression(const IR::Expression *value, cstring label) : Generic(label), value(value) {}

const Expression *Expression::subst(const SymbolicEnv &env) const {
    return new Expression(env.subst(value), label);
}

const Expression *Expression::apply(Transform &visitor) const {
    return new Expression(value->apply(visitor), label);
}

void Expression::complete(Model *model) const { model->complete(value); }

const Expression *Expression::evaluate(const Model &model) const {
    if (Taint::hasTaint(model, value)) {
        return new Expression(&Taint::TAINTED_STRING_LITERAL, label);
    }
    return new Expression(model.evaluate(value), label);
}

void Expression::print(std::ostream &os) const {
    Generic::print(os);
    os << " = " << formatHexExpr(value, true);
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

void IfStatementCondition::complete(Model *model) const { model->complete(postEvalCond); }

const IfStatementCondition *IfStatementCondition::evaluate(const Model &model) const {
    const IR::Literal *evaluatedPostVal = nullptr;
    if (Taint::hasTaint(model, postEvalCond)) {
        evaluatedPostVal = &Taint::TAINTED_STRING_LITERAL;
    } else {
        evaluatedPostVal = model.evaluate(postEvalCond);
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
        os << "[If Statement]: " << preEvalCond->getSourceInfo().toBriefSourceFragment();
    } else {
        os << "[Internal If Statement]: " << preEvalCond;
    }
    os << " -> " << preEvalCond;
    const auto *boolResult = postEvalCond->checkedTo<IR::BoolLiteral>()->value ? "true" : "false";
    os << " -> " << boolResult;
}

/* =============================================================================================
 *   ExtractSuccess
 * ============================================================================================= */

ExtractSuccess::ExtractSuccess(
    const IR::Expression *extractedHeader, int offset, const IR::Expression *condition,
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> fields)
    : extractedHeader(extractedHeader),
      offset(offset),
      condition(condition),
      fields(std::move(fields)) {}

const ExtractSuccess *ExtractSuccess::subst(const SymbolicEnv &env) const {
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        applyFields.emplace_back(field.first, env.subst(field.second));
    }
    return new ExtractSuccess(extractedHeader, offset, condition, applyFields);
}

const ExtractSuccess *ExtractSuccess::apply(Transform &visitor) const {
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        applyFields.emplace_back(field.first, field.second->apply(visitor));
    }
    return new ExtractSuccess(extractedHeader, offset, condition, applyFields);
}

void ExtractSuccess::complete(Model *model) const {
    for (const auto &field : fields) {
        model->complete(field.second);
    }
}

const ExtractSuccess *ExtractSuccess::evaluate(const Model &model) const {
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        if (Taint::hasTaint(model, field.second)) {
            applyFields.emplace_back(field.first, &Taint::TAINTED_STRING_LITERAL);
        } else {
            applyFields.emplace_back(field.first, model.evaluate(field.second));
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
        os << field.first->toString() << " = " << formatHexExpr(field.second, true);
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

Emit::Emit(const IR::Expression *emitHeader,
           std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> fields)
    : emitHeader(emitHeader), fields(std::move(fields)) {}

const Emit *Emit::subst(const SymbolicEnv &env) const {
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        applyFields.emplace_back(field.first, env.subst(field.second));
    }
    return new Emit(emitHeader, applyFields);
}

const Emit *Emit::apply(Transform &visitor) const {
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        applyFields.emplace_back(field.first, field.second->apply(visitor));
    }
    return new Emit(emitHeader, applyFields);
}

void Emit::complete(Model *model) const {
    for (const auto &field : fields) {
        model->complete(field.second);
    }
}

const Emit *Emit::evaluate(const Model &model) const {
    std::vector<std::pair<const IR::StateVariable *, const IR::Expression *>> applyFields;
    applyFields.reserve(fields.size());
    for (const auto &field : fields) {
        if (Taint::hasTaint(model, field.second)) {
            applyFields.emplace_back(field.first, &Taint::TAINTED_STRING_LITERAL);
        } else {
            applyFields.emplace_back(field.first, model.evaluate(field.second));
        }
    }
    return new Emit(emitHeader, applyFields);
}

void Emit::print(std::ostream &os) const {
    os << "[Emit] " << emitHeader->toString() << " -> ";
    for (const auto &field : fields) {
        os << field.first->toString() << " = " << formatHexExpr(field.second, true);
        if (field != fields.back()) {
            os << " | ";
        }
    }
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

void Packet::complete(Model *model) const { model->complete(packetValue); }

const Packet *Packet::evaluate(const Model &model) const {
    if (Taint::hasTaint(model, packetValue)) {
        return new Packet(direction, &Taint::TAINTED_STRING_LITERAL);
    }
    return new Packet(direction, model.evaluate(packetValue));
}

void Packet::print(std::ostream &os) const {
    os << "[Packet " << direction << "] " << formatHexExpr(packetValue, true);
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

}  // namespace P4Tools::TraceEvents
