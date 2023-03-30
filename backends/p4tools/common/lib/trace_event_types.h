#ifndef BACKENDS_P4TOOLS_COMMON_LIB_TRACE_EVENT_TYPES_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_TRACE_EVENT_TYPES_H_

#include <iosfwd>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

/// This file defines explicit types of trace events extended from the generic trace class.
namespace P4Tools::TraceEvents {

/* =============================================================================================
 *   Generic
 * ============================================================================================= */

/// A generic event that only takes in a string.
class Generic : public TraceEvent {
 protected:
    // A label that specifies the type of this generic trace event.
    cstring label;
    void print(std::ostream &os) const override;

 public:
    explicit Generic(cstring label);
    ~Generic() override = default;
    Generic(const Generic &) = default;
    Generic(Generic &&) = default;
    Generic &operator=(const Generic &) = default;
    Generic &operator=(Generic &&) = default;
};

/* =============================================================================================
 *   Expression
 * ============================================================================================= */

/// A simple event that stores the provided expression.
class Expression : public Generic {
 private:
    const IR::Expression *value;

 public:
    [[nodiscard]] const Expression *subst(const SymbolicEnv &env) const override;
    const Expression *apply(Transform &visitor) const override;
    void complete(Model *model) const override;
    [[nodiscard]] const Expression *evaluate(const Model &model) const override;

    explicit Expression(const IR::Expression *value, cstring label);
    ~Expression() override = default;
    Expression(const Expression &) = default;
    Expression(Expression &&) = default;
    Expression &operator=(const Expression &) = default;
    Expression &operator=(Expression &&) = default;

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   ListExpression
 * ============================================================================================= */

/// Evaluates a list of expressions all at once.
class ListExpression : public TraceEvent {
 private:
    const IR::ListExpression *value;

 public:
    [[nodiscard]] const ListExpression *subst(const SymbolicEnv &env) const override;
    const ListExpression *apply(Transform &visitor) const override;
    void complete(Model *model) const override;
    [[nodiscard]] const ListExpression *evaluate(const Model &model) const override;

    explicit ListExpression(const IR::ListExpression *value);

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   IfStatementCondition
 * ============================================================================================= */

/// Represents an if statement condition.
class IfStatementCondition : public TraceEvent {
 private:
    const IR::Expression *preEvalCond = nullptr;
    const IR::Expression *postEvalCond;

    void setPreEvalCond(const IR::Expression *cond);

 public:
    [[nodiscard]] const IfStatementCondition *subst(const SymbolicEnv &env) const override;
    const IfStatementCondition *apply(Transform &visitor) const override;
    void complete(Model *model) const override;
    [[nodiscard]] const IfStatementCondition *evaluate(const Model &model) const override;

    explicit IfStatementCondition(const IR::Expression *cond);

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   Extract
 * ============================================================================================= */

/// A field being extracted by a parser.
class Extract : public TraceEvent {
 private:
    const IR::Member *fieldRef;
    const IR::Expression *value;

 public:
    [[nodiscard]] const Extract *subst(const SymbolicEnv &env) const override;
    const Extract *apply(Transform &visitor) const override;
    void complete(Model *model) const override;
    [[nodiscard]] const Extract *evaluate(const Model &model) const override;

    Extract(const Extract &) = default;
    Extract(Extract &&) = default;
    Extract &operator=(const Extract &) = default;
    Extract &operator=(Extract &&) = default;
    Extract(const IR::Member *fieldRef, const IR::Expression *value);
    ~Extract() override = default;

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   ExtractSuccess
 * ============================================================================================= */

/// Denotes a successful extract call. Will provide information which header was successfully
/// extracted and the offset it was extracted at.
class ExtractSuccess : public TraceEvent {
 private:
    /// The label of the extracted header.
    /// The type is IR::Expression because we may also have IR::PathExpression as inputs.
    const IR::Expression *extractedHeader;

    /// The current parser offset on the input packet.
    int offset;

    /// The condition that allowed us to extract this header.
    const IR::Expression *condition;

    /// The amount of bits we have extracted.
    int extractSize;

 public:
    ExtractSuccess(const IR::Expression *extractedHeader, int offset,
                   const IR::Expression *condition, int extractSize);

    /// @returns the extracted header label stored in this class.
    [[nodiscard]] const IR::Expression *getExtractedHeader() const;

    /// @returns the offset stored in this class.
    [[nodiscard]] int getOffset() const;

    ExtractSuccess(const ExtractSuccess &) = default;
    ExtractSuccess(ExtractSuccess &&) = default;
    ExtractSuccess &operator=(const ExtractSuccess &) = default;
    ExtractSuccess &operator=(ExtractSuccess &&) = default;
    ~ExtractSuccess() override = default;

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   ExtractFailure
 * ============================================================================================= */

/// Denotes a successful extract call. Will provide information which header was successfully
/// extracted and the offset it was extracted at.
class ExtractFailure : public TraceEvent {
 private:
    /// The label of the extracted header.
    /// The type is IR::Expression because we may also have IR::PathExpression as inputs.
    const IR::Expression *extractedHeader;

    /// The current parser offset on the input packet.
    int offset;

    /// The condition that forbade us to extract this header.
    const IR::Expression *condition;

    /// The amount of bits we tried to extract.
    int extractSize;

 public:
    ExtractFailure(const IR::Expression *extractedHeader, int offset,
                   const IR::Expression *condition, int extractSize);

    ExtractFailure(const ExtractFailure &) = default;
    ExtractFailure(ExtractFailure &&) = default;
    ExtractFailure &operator=(const ExtractFailure &) = default;
    ExtractFailure &operator=(ExtractFailure &&) = default;
    ~ExtractFailure() override = default;

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   Emit
 * ============================================================================================= */

/// A field being emitted by a deparser.
class Emit : public TraceEvent {
 private:
    const IR::Member *fieldRef;
    const IR::Expression *value;

 public:
    [[nodiscard]] const Emit *subst(const SymbolicEnv &env) const override;
    const Emit *apply(Transform &visitor) const override;
    void complete(Model *model) const override;
    [[nodiscard]] const Emit *evaluate(const Model &model) const override;

    Emit(const IR::Member *fieldRef, const IR::Expression *value);
    ~Emit() override = default;
    Emit(const Emit &) = default;
    Emit(Emit &&) = default;
    Emit &operator=(const Emit &) = default;
    Emit &operator=(Emit &&) = default;

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   Packet
 * ============================================================================================= */

/// A packet that is either presented to a parser or emitted from a deparser.
class Packet : public TraceEvent {
 public:
    enum class Direction {
        /// Marks a packet that is presented to a parser.
        In,

        /// Marks a packet that is emitted from a deparser.
        Out,
    };

    const Packet *apply(Transform &visitor) const override;

    void complete(Model *model) const override;

    [[nodiscard]] const Packet *evaluate(const Model &model) const override;

    [[nodiscard]] const Packet *subst(const SymbolicEnv &env) const override;

    Packet(const Direction &direction, const IR::Expression *packetValue);
    ~Packet() override = default;
    Packet(const Packet &) = default;
    Packet(Packet &&) = default;
    Packet &operator=(const Packet &) = default;
    Packet &operator=(Packet &&) = default;

 private:
    Direction direction;
    const IR::Expression *packetValue;

 protected:
    void print(std::ostream &os) const override;
};

std::ostream &operator<<(std::ostream &os, const Packet::Direction &direction);

/* =============================================================================================
 *   ParserStart
 * ============================================================================================= */

/// Marks the start of a parser.
class ParserStart : public TraceEvent {
 private:
    const IR::P4Parser *parser;

 public:
    explicit ParserStart(const IR::P4Parser *parser);
    ~ParserStart() override = default;
    ParserStart(const ParserStart &) = default;
    ParserStart(ParserStart &&) = default;
    ParserStart &operator=(const ParserStart &) = default;
    ParserStart &operator=(ParserStart &&) = default;

 protected:
    void print(std::ostream &os) const override;
};

/* =============================================================================================
 *   ParserState
 * ============================================================================================= */

/// Marks the entry into a parser state.
class ParserState : public TraceEvent {
 private:
    const IR::ParserState *state;

 public:
    explicit ParserState(const IR::ParserState *state);
    ~ParserState() override = default;
    ParserState(const ParserState &) = default;
    ParserState(ParserState &&) = default;
    ParserState &operator=(const ParserState &) = default;
    ParserState &operator=(ParserState &&) = default;

 protected:
    void print(std::ostream &os) const override;
};

}  // namespace P4Tools::TraceEvents

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_TRACE_EVENT_TYPES_H_ */
