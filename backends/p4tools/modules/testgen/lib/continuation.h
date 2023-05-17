#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_CONTINUATION_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_CONTINUATION_H_

#include <cstdint>
#include <deque>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/cstring.h"

namespace Test {
class SmallStepTest;
}  // namespace Test

namespace P4Tools::P4Testgen {

/// A continuation is a function that optionally takes an argument and executes a sequence of
/// commands.
class Continuation {
 public:
    /// Enumerates the exceptions that can be thrown during symbolic execution.
    //
    // TODO: Add more as needed. At least, we'd probably want to use this to model parser
    // exceptions.
    enum class Exception {
        /// Thrown when an exit statement is encountered.
        Exit,
        /// Thrown when a select expression fails to match. This models P4's error.NoMatch.
        NoMatch,
        /// Thrown when the parser reaches the reject state
        Reject,
        /// This is an internal interpreter exception to express drop semantics.
        Drop,
        /// Thrown on premature packet end. Models P4's error.PacketTooShort.
        PacketTooShort,
        /// Thrown when the target terminates.
        Abort,
    };

    /// A helper function to print the string values of the exception enum. Unfortunately, this
    /// is a little clunky to use an maintain because we have to add new enum fields manually.
    /// TODO: Find a better implementation to print enums. Also use constexpr instead.
    friend std::ostream &operator<<(std::ostream &out, const Exception value) {
        static std::map<Exception, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(Exception::Exit);
            INSERT_ELEMENT(Exception::NoMatch);
            INSERT_ELEMENT(Exception::Reject);
            INSERT_ELEMENT(Exception::Drop);
            INSERT_ELEMENT(Exception::Abort);
#undef INSERT_ELEMENT
        }

        return out << strings[value];
    }

    /// Represents a command that invokes the topmost continuation on the continuation stack. If a
    /// node is provided, it is first evaluated as an expression, and the result is used as an
    /// argument for the invocation. Note that the node doesn't necessarily need to be an instance
    /// of IR::Expression; other IR nodes may be treated as expressions in the metalanguage, even
    /// though they are not P4 expressions.
    struct Return {
        std::optional<const IR::Node *> expr;

        /// Delegates to IR equality.
        bool operator==(const Return &other) const;

        Return() : expr(std::nullopt) {}
        explicit Return(const IR::Node *expr);
    };

    /// Alias for various property types that can be set. We restrict these to keep the feature
    /// simple.
    using PropertyValue = std::variant<cstring, uint64_t, int64_t, bool, const IR::Expression *>;

    struct PropertyUpdate {
        /// The name of the property that is being set.
        cstring propertyName;
        /// The value of the property.
        PropertyValue property;

        /// Delegates to the equality of the property.
        bool operator==(const PropertyUpdate &other) const;

        explicit PropertyUpdate(cstring propertyName, PropertyValue property);
    };

    /// Guards are commands that can be inserted into the continuation stack to ensure that the
    /// interpreter does not violate a given invariant. If the condition evaluates to false, the
    /// path is invalid and the symbolic executor will not traverse it.
    /// For example, specific port values are not supported in a hardware target guards can enforce
    /// that only the appropriate port variables are chosen. Any other possible path is discarded.
    struct Guard {
        const IR::Expression *cond;

        /// Delegates to IR equality.
        bool operator==(const Guard &other) const;

        explicit Guard(const IR::Expression *cond);
    };

    using Command = std::variant<
        /// Executes a statement-like IR node.
        const IR::Node *,
        /// Registers a trace event.
        const TraceEvent *,
        /// Invokes the topmost continuation on the continuation stack. Execution halts if the
        /// stack is empty. It is a BUG to return a value with an empty continuation stack.
        Return,
        /// Throws an exception.
        Exception,
        /// Updates a property in the state.
        PropertyUpdate,
        /// Ensures that the given branch proceeds in a valid state.
        Guard>;

    /// A continuation body is a list of commands.
    class Body {
        friend class Continuation;
        friend class Test::SmallStepTest;

        std::deque<Command> cmds;

     public:
        /// Determines whether this body is empty.
        bool empty() const;

        /// Returns the next command to be executed. This is the top of the command stack. A BUG
        /// occurs if this body is @empty.
        const Command next() const;

        /// Pushes the given command onto the command stack.
        void push(Command cmd);

        /// Pops the next element off the command stack. A BUG occurs if the command stack is
        /// empty.
        void pop();

        /// Removes all commands in this body.
        void clear();

        bool operator==(const Body &) const;

        /// Allows the command stack to be initialized with a list initializer.
        Body(std::initializer_list<Command> cmds);

        /// Allow deque initialization with a vector. We iterate and push through the vector in
        /// reverse.
        explicit Body(const std::vector<Command> &cmds);

     private:
        Body() = default;
    };

    /// A wrapper to enforce that continuations are created with parameters obtained from
    /// @genParameter.
    class Parameter {
        friend Continuation;

     public:
        const IR::PathExpression *param;

     private:
        explicit Parameter(const IR::PathExpression *param) : param(param) {}
    };

    /// Represents the continuation's parameter.
    //
    // Invariant: this parameter is uniquely named.
    std::optional<const IR::PathExpression *> parameterOpt;
    Body body;

    /// @returns a body that is equivalent to applying this continuation to the given value (or
    /// unit, if no value is provided). A BUG occurs if parameterOpt is std::nullopt but value_opt
    /// is not, or vice versa.
    ///
    /// Expressions in the metalanguage include P4 non-expressions. Because of this, the value (if
    /// provided) does not necessarily need to be an instance of IR::Expression.
    Body apply(std::optional<const IR::Node *> value_opt) const;

    /// @returns a parameter for use in building continuations. The parameter will be fresh in the
    /// given @ctx.
    static const Parameter *genParameter(const IR::Type *type, cstring name,
                                         const NamespaceContext *ctx);

    /// Creates a parameterless continuation.
    explicit Continuation(Body body) : Continuation(std::nullopt, std::move(body)) {}

    /// Creates a continuation. The continuation will have the given parameter, if one is provided;
    /// otherwise, the continuation will have no parameters.
    Continuation(std::optional<const Parameter *> parameterOpt, Body body)
        : parameterOpt(parameterOpt ? std::make_optional((*parameterOpt)->param) : std::nullopt),
          body(std::move(body)) {}
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_CONTINUATION_H_ */
