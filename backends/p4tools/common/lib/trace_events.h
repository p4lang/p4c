#ifndef COMMON_LIB_TRACE_EVENTS_H_
#define COMMON_LIB_TRACE_EVENTS_H_

#include <iosfwd>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools {

/// An event in a trace of the execution of a P4 program.
class TraceEvent {
 private:
    friend std::ostream& operator<<(std::ostream& os, const TraceEvent& event);

 public:
    class Generic;
    class Extract;
    class PreEvalExpression;
    class Emit;
    class Expression;
    class ListExpression;

    class Packet;
    class ParserStart;
    class ParserState;

    explicit TraceEvent(cstring label = "");

    /// Substitutes state variables in the body of this trace event for their symbolic value in the
    /// given symbolic environment. Variables that are unbound by the given environment are left
    /// untouched.
    virtual const TraceEvent* subst(const SymbolicEnv& env) const;

    /// Applies the given IR transform to the expressions in this trace event.
    virtual const TraceEvent* apply(Transform& visitor) const;

    /// Completes a model with the variables used in this trace event.
    virtual void complete(Model* model) const;

    /// Evaluates expressions in the body of this trace event for their concrete value in the given
    /// model. A BUG occurs if there are any variables that are unbound by the given model.
    virtual const TraceEvent* evaluate(const Model& model) const;

 protected:
    // An optional label that can be prefixed to the trace event.
    cstring label;
    /// Prints this trace event to the given ostream.
    virtual void print(std::ostream&) const = 0;
};

std::ostream& operator<<(std::ostream& os, const TraceEvent& event);

/// A generic event that only takes in a string.
class TraceEvent::Generic : public TraceEvent {
 public:
    const Generic* subst(const SymbolicEnv& env) const override;

    explicit Generic(cstring label);

 protected:
    void print(std::ostream& /*out*/) const override;
};

/// A simple event that stores the provided expression.
class TraceEvent::Expression : public TraceEvent {
 private:
    const IR::Expression* value;

 public:
    const Expression* subst(const SymbolicEnv& env) const override;
    const Expression* apply(Transform& visitor) const override;
    void complete(Model* model) const override;
    const Expression* evaluate(const Model& model) const override;

    explicit Expression(const IR::Expression* value, cstring label = "");

 protected:
    void print(std::ostream& /*out*/) const override;
};

/// Evaluates a list of expressions all at once.
class TraceEvent::ListExpression : public TraceEvent {
 private:
    const IR::ListExpression* value;

 public:
    const ListExpression* subst(const SymbolicEnv& env) const override;
    const ListExpression* apply(Transform& visitor) const override;
    void complete(Model* model) const override;
    const ListExpression* evaluate(const Model& model) const override;

    explicit ListExpression(const IR::ListExpression* value, cstring label = "");

 protected:
    void print(std::ostream& /*out*/) const override;
};

/// An event trace that records both the input before evaluation and the evaluated result.
/// This can be useful to understand how a particular expression has been evaluated.
class TraceEvent::PreEvalExpression : public TraceEvent {
 private:
    const IR::Expression* preEvalVal = nullptr;
    const IR::Expression* postEvalVal;

    void setPreEvalVal(const IR::Expression* val);

 public:
    const PreEvalExpression* subst(const SymbolicEnv& env) const override;
    const PreEvalExpression* apply(Transform& visitor) const override;
    void complete(Model* model) const override;
    const PreEvalExpression* evaluate(const Model& model) const override;

    explicit PreEvalExpression(const IR::Expression* val, cstring label = "");

 protected:
    void print(std::ostream& /*out*/) const override;
};

/// A field being extracted by a parser.
class TraceEvent::Extract : public TraceEvent {
 private:
    const IR::Member* fieldRef;
    const IR::Expression* value;

 public:
    const Extract* subst(const SymbolicEnv& env) const override;
    const Extract* apply(Transform& visitor) const override;
    void complete(Model* model) const override;
    const Extract* evaluate(const Model& model) const override;

    Extract(const IR::Member* fieldRef, const IR::Expression* value, cstring label = "");

 protected:
    void print(std::ostream& /*out*/) const override;
};

/// A field being emitted by a deparser.
class TraceEvent::Emit : public TraceEvent {
 private:
    const IR::Member* fieldRef;
    const IR::Expression* value;

 public:
    const Emit* subst(const SymbolicEnv& env) const override;
    const Emit* apply(Transform& visitor) const override;
    void complete(Model* model) const override;
    const Emit* evaluate(const Model& model) const override;

    Emit(const IR::Member* fieldRef, const IR::Expression* value, cstring label = "");

 protected:
    void print(std::ostream& /*out*/) const override;
};

/// A packet that is either presented to a parser or emitted from a deparser.
class TraceEvent::Packet : public TraceEvent {
 public:
    enum class Direction {
        /// Marks a packet that is presented to a parser.
        In,

        /// Marks a packet that is emitted from a deparser.
        Out,
    };

    const Direction direction;
    const IR::Expression* packetValue;

    const Packet* subst(const SymbolicEnv& env) const override;
    const Packet* apply(Transform& visitor) const override;
    void complete(Model* model) const override;
    const Packet* evaluate(const Model& model) const override;

    Packet(const Direction& direction, const IR::Expression* packetValue, cstring label = "");

 protected:
    void print(std::ostream&) const override;
};

std::ostream& operator<<(std::ostream& os, const TraceEvent::Packet::Direction& direction);

/// Marks the start of a parser.
class TraceEvent::ParserStart : public TraceEvent {
 private:
    const IR::P4Parser* parser;

 public:
    explicit ParserStart(const IR::P4Parser* parser, cstring label = "");

 protected:
    void print(std::ostream&) const override;
};

/// Marks the entry into a parser state.
class TraceEvent::ParserState : public TraceEvent {
 private:
    const IR::ParserState* state;

 public:
    explicit ParserState(const IR::ParserState* state, cstring label = "");

 protected:
    void print(std::ostream&) const override;
};

}  // namespace P4Tools

#endif /* COMMON_LIB_TRACE_EVENTS_H_ */
