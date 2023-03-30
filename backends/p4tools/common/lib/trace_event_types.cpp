#include "backends/p4tools/common/lib/trace_event_types.h"

#include <ostream>
#include <string>
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
    os << " = " << formatBinOrHexExpr(value, true);
}

/* =============================================================================================
 *   ListExpression
 * ============================================================================================= */

ListExpression::ListExpression(const IR::ListExpression *value) : value(value) {}

const ListExpression *ListExpression::subst(const SymbolicEnv &env) const {
    return new ListExpression(env.subst(value)->checkedTo<IR::ListExpression>());
}

const ListExpression *ListExpression::apply(Transform &visitor) const {
    return new ListExpression(value->apply(visitor)->checkedTo<IR::ListExpression>());
}

void ListExpression::complete(Model *model) const { model->complete(value); }

const ListExpression *ListExpression::evaluate(const Model &model) const {
    IR::Vector<IR::Expression> evaluatedExpressions;
    for (const auto *component : value->components) {
        BUG_CHECK(!component->is<IR::ListExpression>(),
                  "Nested ListExpressions are currently not supported.");
        if (Taint::hasTaint(model, component)) {
            evaluatedExpressions.push_back(&Taint::TAINTED_STRING_LITERAL);
        } else {
            evaluatedExpressions.push_back(model.evaluate(component));
        }
    }
    return new ListExpression(new IR::ListExpression(evaluatedExpressions));
}

void ListExpression::print(std::ostream &os) const {
    os << "[Expression]: ";
    os << " = {";
    for (const auto *component : value->components) {
        os << formatBinOrHexExpr(component, true);
        if (component != value->components.back()) {
            os << ", ";
        }
    }
    os << "}";
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
    const auto &srcInfo = postEvalCond->getSourceInfo();
    if (srcInfo.isValid()) {
        os << "[If Statement]: " << postEvalCond->getSourceInfo().toBriefSourceFragment();
    } else {
        os << "[Internal If Statement]: " << preEvalCond;
    }
    os << " -> " << preEvalCond;
    const auto *boolResult = postEvalCond->checkedTo<IR::BoolLiteral>()->value ? "true" : "false";
    os << " -> " << boolResult;
}

/* =============================================================================================
 *   Extract
 * ============================================================================================= */

Extract::Extract(const IR::Member *fieldRef, const IR::Expression *value)
    : fieldRef(fieldRef), value(value) {}

const Extract *Extract::subst(const SymbolicEnv &env) const {
    return new Extract(fieldRef, env.subst(value));
}

const Extract *Extract::apply(Transform &visitor) const {
    return new Extract(fieldRef, value->apply(visitor));
}

void Extract::complete(Model *model) const { model->complete(value); }

const Extract *Extract::evaluate(const Model &model) const {
    if (Taint::hasTaint(model, value)) {
        return new Extract(fieldRef, &Taint::TAINTED_STRING_LITERAL);
    }
    return new Extract(fieldRef, model.evaluate(value));
}

void Extract::print(std::ostream &os) const {
    // Using toString produces cleaner output.
    os << "[Extract] " << fieldRef->toString() << " = " << formatBinOrHexExpr(value, true);
}

/* =============================================================================================
 *   ExtractSuccess
 * ============================================================================================= */

ExtractSuccess::ExtractSuccess(const IR::Expression *extractedHeader, int offset,
                               const IR::Expression *condition, int extractSize)
    : extractedHeader(extractedHeader),
      offset(offset),
      condition(condition),
      extractSize(extractSize) {}

int ExtractSuccess::getOffset() const { return offset; }

const IR::Expression *ExtractSuccess::getExtractedHeader() const { return extractedHeader; }

void ExtractSuccess::print(std::ostream &os) const {
    // Using toString produces cleaner output.
    os << "[ExtractSuccess] " << extractedHeader->toString() << "@" << offset
       << " | Condition: " << condition << " | Extract Size: " << extractSize;
}

/* =============================================================================================
 *   ExtractFailure
 * ============================================================================================= */

ExtractFailure::ExtractFailure(const IR::Expression *extractedHeader, int offset,
                               const IR::Expression *condition, int extractSize)
    : extractedHeader(extractedHeader),
      offset(offset),
      condition(condition),
      extractSize(extractSize) {}

void ExtractFailure::print(std::ostream &os) const {
    // Using toString produces cleaner output.
    os << "[ExtractFailure] " << extractedHeader->toString() << "@" << offset
       << " | Condition: " << condition << " | Extract Size: " << extractSize;
}

/* =============================================================================================
 *   Emit
 * ============================================================================================= */

Emit::Emit(const IR::Member *fieldRef, const IR::Expression *value)
    : fieldRef(fieldRef), value(value) {}

const Emit *Emit::subst(const SymbolicEnv &env) const {
    return new Emit(fieldRef, env.subst(value));
}

const Emit *Emit::apply(Transform &visitor) const {
    return new Emit(fieldRef, value->apply(visitor));
}

void Emit::complete(Model *model) const { model->complete(value); }

const Emit *Emit::evaluate(const Model &model) const {
    if (Taint::hasTaint(model, value)) {
        return new Emit(fieldRef, &Taint::TAINTED_STRING_LITERAL);
    }
    return new Emit(fieldRef, model.evaluate(value));
}

void Emit::print(std::ostream &os) const {
    os << "[Emit] " << fieldRef->toString() << " = " << formatBinOrHexExpr(value, true);
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
    os << "[Packet " << direction << "] " << formatBinOrHexExpr(packetValue, true);
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
