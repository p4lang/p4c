#include "backends/p4tools/common/lib/trace_events.h"

#include <ostream>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/taint.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/safe_vector.h"
#include "p4tools/common/lib/model.h"
#include "p4tools/common/lib/symbolic_env.h"

namespace P4Tools {

std::ostream& operator<<(std::ostream& os, const TraceEvent& event) {
    event.print(os);
    return os;
}

TraceEvent::TraceEvent(cstring label) : label(label) {}

const TraceEvent* TraceEvent::subst(const SymbolicEnv& /*env*/) const { return this; }

void TraceEvent::complete(Model* /*model*/) const {}

const TraceEvent* TraceEvent::apply(Transform& /*visitor*/) const { return this; }

const TraceEvent* TraceEvent::evaluate(const Model& /*model*/) const { return this; }

/* =============================================================================================
 *   Generic
 * ============================================================================================= */

TraceEvent::Generic::Generic(cstring label) : TraceEvent(label) {}

const TraceEvent::Generic* TraceEvent::Generic::subst(const SymbolicEnv& /*env*/) const {
    return new Generic(label);
}

void TraceEvent::Generic::print(std::ostream& os) const { os << "[Event]: " << label; }

/* =============================================================================================
 *   Expression
 * ============================================================================================= */

TraceEvent::Expression::Expression(const IR::Expression* value, cstring label)
    : TraceEvent(label), value(value) {}

const TraceEvent::Expression* TraceEvent::Expression::subst(const SymbolicEnv& env) const {
    return new Expression(env.subst(value), label);
}

const TraceEvent::Expression* TraceEvent::Expression::apply(Transform& visitor) const {
    return new Expression(value->apply(visitor), label);
}

void TraceEvent::Expression::complete(Model* model) const { model->complete(value); }

const TraceEvent::Expression* TraceEvent::Expression::evaluate(const Model& model) const {
    if (Taint::hasTaint(model, value)) {
        return new Expression(&Taint::TAINTED_STRING_LITERAL, label);
    }
    return new Expression(model.evaluate(value), label);
}

void TraceEvent::Expression::print(std::ostream& os) const {
    os << "[Expression]: ";
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << " = " << formatBinOrHexExpr(value, true);
}

/* =============================================================================================
 *   ListExpression
 * ============================================================================================= */

TraceEvent::ListExpression::ListExpression(const IR::ListExpression* value, cstring label)
    : TraceEvent(label), value(value) {}

const TraceEvent::ListExpression* TraceEvent::ListExpression::subst(const SymbolicEnv& env) const {
    return new ListExpression(env.subst(value)->checkedTo<IR::ListExpression>(), label);
}

const TraceEvent::ListExpression* TraceEvent::ListExpression::apply(Transform& visitor) const {
    return new ListExpression(value->apply(visitor)->checkedTo<IR::ListExpression>(), label);
}

void TraceEvent::ListExpression::complete(Model* model) const { model->complete(value); }

const TraceEvent::ListExpression* TraceEvent::ListExpression::evaluate(const Model& model) const {
    IR::Vector<IR::Expression> evaluatedExpressions;
    for (const auto* component : value->components) {
        BUG_CHECK(!component->is<IR::ListExpression>(),
                  "Nested ListExpressions are currently not supported.");
        if (Taint::hasTaint(model, component)) {
            evaluatedExpressions.push_back(&Taint::TAINTED_STRING_LITERAL);
        } else {
            evaluatedExpressions.push_back(model.evaluate(component));
        }
    }
    return new ListExpression(new IR::ListExpression(evaluatedExpressions), label);
}

void TraceEvent::ListExpression::print(std::ostream& os) const {
    os << "[Expression]: ";
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << " = {";
    for (const auto* component : value->components) {
        os << formatBinOrHexExpr(component, true);
        if (component != value->components.back()) {
            os << ", ";
        }
    }
    os << "}";
}

/* =============================================================================================
 *   PreEvalExpression
 * ============================================================================================= */

TraceEvent::PreEvalExpression::PreEvalExpression(const IR::Expression* val, cstring label)
    : TraceEvent(label), postEvalVal(val) {}

void TraceEvent::PreEvalExpression::setPreEvalVal(const IR::Expression* val) { preEvalVal = val; }

const TraceEvent::PreEvalExpression* TraceEvent::PreEvalExpression::subst(
    const SymbolicEnv& env) const {
    auto* traceEvent = new PreEvalExpression(env.subst(postEvalVal), label);
    traceEvent->setPreEvalVal(postEvalVal);
    return traceEvent;
}

const TraceEvent::PreEvalExpression* TraceEvent::PreEvalExpression::apply(
    Transform& visitor) const {
    auto* traceEvent = new PreEvalExpression(postEvalVal->apply(visitor), label);
    traceEvent->setPreEvalVal(postEvalVal);
    return traceEvent;
}

void TraceEvent::PreEvalExpression::complete(Model* model) const { model->complete(postEvalVal); }

const TraceEvent::PreEvalExpression* TraceEvent::PreEvalExpression::evaluate(
    const Model& model) const {
    const IR::Literal* evaluatedPostVal = nullptr;
    if (Taint::hasTaint(model, postEvalVal)) {
        evaluatedPostVal = &Taint::TAINTED_STRING_LITERAL;
    } else {
        evaluatedPostVal = model.evaluate(postEvalVal);
    }
    auto* traceEvent = new PreEvalExpression(evaluatedPostVal, label);
    traceEvent->setPreEvalVal(postEvalVal);
    return traceEvent;
}

void TraceEvent::PreEvalExpression::print(std::ostream& os) const {
    os << "[Expression]: ";
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    // If the the trace event has recorded the value pre-evaluation, print it.
    if (preEvalVal != nullptr) {
        preEvalVal->dbprint(os);
        os << " = ";
    }
    os << formatBinOrHexExpr(postEvalVal, true);
}

/* =============================================================================================
 *   Extract
 * ============================================================================================= */

TraceEvent::Extract::Extract(const IR::Member* fieldRef, const IR::Expression* value, cstring label)
    : TraceEvent(label), fieldRef(fieldRef), value(value) {}

const TraceEvent::Extract* TraceEvent::Extract::subst(const SymbolicEnv& env) const {
    return new Extract(fieldRef, env.subst(value), label);
}

const TraceEvent::Extract* TraceEvent::Extract::apply(Transform& visitor) const {
    return new Extract(fieldRef, value->apply(visitor), label);
}

void TraceEvent::Extract::complete(Model* model) const { model->complete(value); }

const TraceEvent::Extract* TraceEvent::Extract::evaluate(const Model& model) const {
    if (Taint::hasTaint(model, value)) {
        return new Extract(fieldRef, &Taint::TAINTED_STRING_LITERAL, label);
    }
    return new Extract(fieldRef, model.evaluate(value), label);
}

void TraceEvent::Extract::print(std::ostream& os) const {
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << "[Extract] " << fieldRef << " = " << formatBinOrHexExpr(value, true);
}

/* =============================================================================================
 *   Emit
 * ============================================================================================= */

TraceEvent::Emit::Emit(const IR::Member* fieldRef, const IR::Expression* value, cstring label)
    : TraceEvent(label), fieldRef(fieldRef), value(value) {}

const TraceEvent::Emit* TraceEvent::Emit::subst(const SymbolicEnv& env) const {
    return new Emit(fieldRef, env.subst(value), label);
}

const TraceEvent::Emit* TraceEvent::Emit::apply(Transform& visitor) const {
    return new Emit(fieldRef, value->apply(visitor), label);
}

void TraceEvent::Emit::complete(Model* model) const { model->complete(value); }

const TraceEvent::Emit* TraceEvent::Emit::evaluate(const Model& model) const {
    if (Taint::hasTaint(model, value)) {
        return new Emit(fieldRef, &Taint::TAINTED_STRING_LITERAL, label);
    }
    return new Emit(fieldRef, model.evaluate(value), label);
}

void TraceEvent::Emit::print(std::ostream& os) const {
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << "[Emit] " << fieldRef << " = " << formatBinOrHexExpr(value, true);
}

/* =============================================================================================
 *   Packet
 * ============================================================================================= */

TraceEvent::Packet::Packet(const TraceEvent::Packet::Direction& direction,
                           const IR::Expression* packetValue, cstring label)
    : TraceEvent(label), direction(direction), packetValue(packetValue) {}

const TraceEvent::Packet* TraceEvent::Packet::subst(const SymbolicEnv& env) const {
    return new Packet(direction, env.subst(packetValue), label);
}

const TraceEvent::Packet* TraceEvent::Packet::apply(Transform& visitor) const {
    return new Packet(direction, packetValue->apply(visitor), label);
}

void TraceEvent::Packet::complete(Model* model) const { model->complete(packetValue); }

const TraceEvent::Packet* TraceEvent::Packet::evaluate(const Model& model) const {
    if (Taint::hasTaint(model, packetValue)) {
        return new Packet(direction, &Taint::TAINTED_STRING_LITERAL, label);
    }
    return new Packet(direction, model.evaluate(packetValue), label);
}

void TraceEvent::Packet::print(std::ostream& os) const {
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << "[Packet " << direction << "] " << formatBinOrHexExpr(packetValue, true);
}

std::ostream& operator<<(std::ostream& os, const TraceEvent::Packet::Direction& direction) {
    switch (direction) {
        case TraceEvent::Packet::Direction::In:
            os << "in";
            break;

        case TraceEvent::Packet::Direction::Out:
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

TraceEvent::ParserStart::ParserStart(const IR::P4Parser* parser, cstring label)
    : TraceEvent(label), parser(parser) {}

void TraceEvent::ParserStart::print(std::ostream& os) const {
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << "[Parser] " << parser->name.toString();
}

/* =============================================================================================
 *   ParserState
 * ============================================================================================= */

TraceEvent::ParserState::ParserState(const IR::ParserState* state, cstring label)
    : TraceEvent(label), state(state) {}

void TraceEvent::ParserState::print(std::ostream& os) const {
    if (!label.isNullOrEmpty()) {
        os << label << ": ";
    }
    os << "[State] " << state->name.toString();
}

}  // namespace P4Tools
