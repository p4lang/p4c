#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXECUTION_STATE_H_

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <stack>
#include <utility>
#include <variant>
#include <vector>

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/core/abstract_execution_state.h"
#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"

namespace P4Tools::P4Testgen {

/// Represents state of execution after having reached a program point.
class ExecutionState : public AbstractExecutionState {
    friend class Test::SmallStepTest;

 public:
    class StackFrame {
     public:
        using ExceptionHandlers = std::map<Continuation::Exception, Continuation>;

     private:
        Continuation normalContinuation;
        ExceptionHandlers exceptionHandlers;
        const NamespaceContext *namespaces = nullptr;

     public:
        StackFrame(Continuation normalContinuation, const NamespaceContext *namespaces);

        StackFrame(Continuation normalContinuation, ExceptionHandlers exceptionHandlers,
                   const NamespaceContext *namespaces);

        StackFrame(const StackFrame &) = default;
        StackFrame(StackFrame &&) noexcept = default;
        StackFrame &operator=(const StackFrame &) = default;
        StackFrame &operator=(StackFrame &&) = default;
        ~StackFrame() = default;

        /// @returns the top-level continuation of this particular stack frame.
        [[nodiscard]] const Continuation &getContinuation() const;

        /// @returns the exception handlers contained within this stack frame.
        [[nodiscard]] const ExceptionHandlers &getExceptionHandlers() const;

        /// @returns the namespaces contained within this stack frame.
        [[nodiscard]] const NamespaceContext *getNameSpaces() const;
    };

    /// No move semantics because of constant members. We always need to clone a state.
    ExecutionState(ExecutionState &&) = delete;
    ExecutionState &operator=(ExecutionState &&) = delete;
    ~ExecutionState() override = default;

 private:
    /// The program trace for the current program point (i.e., how we got to the current state).
    std::vector<std::reference_wrapper<const TraceEvent>> trace;

    /// Set of visited nodes. Used for code coverage.
    P4::Coverage::CoverageSet visitedNodes;

    /// The remaining body of the current function being executed.
    ///
    /// Invariant: if this is empty, then so is the @stack, and this state is terminal.
    Continuation::Body body;

    /// When execution of the @body completes, the @stack is popped and execution passes to the
    /// normal continuation in the popped frame. Any returned value is given as the argument to
    /// that continuation. If the current @body throws an exception, the stack is popped until we
    /// find a corresponding handler. The handler body becomes the new @body, and the frame below
    /// becomes the top of the stack.
    ///
    // Invariant: if the @body is empty, then so is this, and this state is terminal.
    std::stack<std::reference_wrapper<const StackFrame>> stack;

    /// State properties are bools, integers, or strings that can be set and propagated across
    /// execution state. They are used to influence execution along a particular continuation path.
    /// For example, we can set the "inUndefinedState" property to indicate that all state being
    /// written while this variable is active is tainted. This property must be unset manually to
    /// resume normal operation by setting the property "false". Usually, this is done directly
    /// after the tainted sequence of commands has been executed.
    std::map<cstring, Continuation::PropertyValue> stateProperties;

    // Test objects are classes of variables that influence the execution of test frameworks. They
    // are collected during interpreter execution and consumed by the respective test framework. For
    // example, when visiting a table, the interpreter will generate a table test object per table,
    // which defines control plane match action entries. Once the interpreter has solved for the
    // variables used by these test objects and concretized the values, they can be used to generate
    // a test. Test objects are not constant because they may be manipulated by a target back end.
    std::map<cstring, TestObjectMap> testObjects;

    /// The parserErrorLabel is set by the parser to indicate the variable corresponding to the
    /// parser error that is set by various built-in functions such as verify or extract.
    std::optional<IR::StateVariable> parserErrorLabel = std::nullopt;

    /// This variable tracks how much of the input packet has been parsed.
    /// Typically, this is equivalent to the size of the input packet. However, there may be
    /// functions such as lookahead that peek into the program packet without actually advancing the
    /// cursor. The cursor ensures that parsing remains consistent in these cases.
    int inputPacketCursor = 0;

    /// List of path constraints - expressions that must all evaluate to true to reach this
    /// execution state.
    std::vector<const IR::Expression *> pathConstraint;

    /// List of branch decisions leading into this state.
    std::vector<uint64_t> selectedBranches;

    /// State that is needed to track reachability of nodes given a query.
    ReachabilityEngineState *reachabilityEngineState = nullptr;

    /// The number of individual packet variables that have been created in this state.
    /// Used to create unique symbolic variables in some cases.
    uint16_t numAllocatedPacketVariables = 0;

    /// Helper function to create a new, unique symbolic packet variable.
    /// Keeps track of the allocated packet variables by incrementing @ref
    /// numAllocatedPacketVariables.
    /// @returns a fresh symbolic variable.
    [[nodiscard]] const IR::SymbolicVariable *createPacketVariable(const IR::Type *type);

    /* =========================================================================================
     *  Accessors
     * ========================================================================================= */
 public:
    /// Determines whether this state represents the end of an execution.
    [[nodiscard]] bool isTerminal() const;

    /// @returns list of paths constraints.
    [[nodiscard]] const std::vector<const IR::Expression *> &getPathConstraint() const;

    /// @returns list of branch decisions leading into this state.
    [[nodiscard]] const std::vector<uint64_t> &getSelectedBranches() const;

    /// Adds path constraint.
    void pushPathConstraint(const IR::Expression *e);

    /// Adds branch decision identifier. Must be called for all branches with more than one
    /// successor. These branch decision identifiers are then used by track branches and
    /// selected (input) branches features.
    void pushBranchDecision(uint64_t);

    /// @returns the next command to be evaluated, if any.
    /// @returns std::nullopt if the current body is empty.
    [[nodiscard]] std::optional<const Continuation::Command> getNextCmd() const;

    /// @returns the symbolic value of the given state variable.
    [[nodiscard]] const IR::Expression *get(const IR::StateVariable &var) const override;

    /// Checks whether the node has been visited in this state.
    void markVisited(const IR::Node *node);

    /// @returns list of all nodes visited before reaching this state.
    [[nodiscard]] const P4::Coverage::CoverageSet &getVisited() const;

    /// Sets the symbolic value of the given state variable to the given value. Constant folding
    /// is done on the given value before updating the symbolic state.
    void set(const IR::StateVariable &var, const IR::Expression *value) override;

    /// @returns the current event trace.
    [[nodiscard]] const std::vector<std::reference_wrapper<const TraceEvent>> &getTrace() const;

    /// @returns the current body.
    [[nodiscard]] const Continuation::Body &getBody() const;

    /// @returns the current stack.
    [[nodiscard]] const std::stack<std::reference_wrapper<const StackFrame>> &getStack() const;

    /// Set the property with @arg propertyName to @arg property.
    void setProperty(cstring propertyName, Continuation::PropertyValue property);

    /// @returns whether the property with @arg propertyName exists.
    [[nodiscard]] bool hasProperty(cstring propertyName) const;

    /// @returns the property saved under @arg propertyName from the stateProperties map. Throws a
    /// BUG, If the specified type does not match or the property is not found.
    template <class T>
    [[nodiscard]] T getProperty(cstring propertyName) const {
        auto iterator = stateProperties.find(propertyName);
        if (iterator != stateProperties.end()) {
            auto val = iterator->second;
            try {
                T resolvedVal = std::get<T>(val);
                return resolvedVal;
            } catch (std::bad_variant_access const &ex) {
                BUG("Expected property value type does not correspond to value type stored in the "
                    "property map.");
            }
        }
        BUG("Property %s not found in configuration map.", propertyName);
    }

    /// Add a test object. Create the category and label, if necessary.
    /// Overrides the object label if it already exists in the object map.
    void addTestObject(cstring category, cstring objectLabel, const TestObject *object);

    /// @returns the uninterpreted test object using the provided category and object label. If
    /// @param checked is enabled, a BUG is thrown if the object label does not exist.
    [[nodiscard]] const TestObject *getTestObject(cstring category, cstring objectLabel,
                                                  bool checked) const;

    /// Remove a test object from a category.
    void deleteTestObject(cstring category, cstring objectLabel);

    /// Remove a test category entirely.
    void deleteTestObjectCategory(cstring category);

    /// @returns the uninterpreted test object using the provided category and object label. If
    /// @param checked is enabled, a BUG is thrown if the object label does not exist.
    /// Also casts the test object to the specified type. If the type does not match, a BUG is
    /// thrown.
    template <class T>
    [[nodiscard]] const T *getTestObject(cstring category, cstring objectLabel) const {
        const auto *testObject = getTestObject(category, objectLabel, true);
        return testObject->checkedTo<T>();
    }

    /// @returns the map of uninterpreted test objects for a specific test category. For example,
    /// all the table entries saved under "tableconfigs".
    [[nodiscard]] TestObjectMap getTestObjectCategory(cstring category) const;

    /// Get the current state of the reachability engine.
    [[nodiscard]] ReachabilityEngineState *getReachabilityEngineState() const;

    /// Update the reachability engine state.
    void setReachabilityEngineState(ReachabilityEngineState *newEngineState);

    /* =========================================================================================
     *  Trace events.
     * ========================================================================================= */
    /// Add a new trace event to the state.
    void add(const TraceEvent &event);

    /* =========================================================================================
     *  Body operations
     * ========================================================================================= */
    /// Replaces the top element of @body with @cmd.
    void replaceTopBody(const Continuation::Command &cmd);

    /// Replaces the topmost command in @body with the contents of @cmds. A BUG occurs if @body
    /// is empty or @cmds is empty.
    void replaceTopBody(const std::vector<Continuation::Command> *cmds);

    /// Replaces the topmost command in @body with the contents of @cmds. A BUG occurs if @body
    /// is empty or @cmds is empty.
    void replaceTopBody(std::initializer_list<Continuation::Command> cmds);

    /// A convenience method for the version that takes a vector of commands. Replaces the topmost
    /// command in @body with the contents of @nodes. A BUG occurs if @body is empty or if @nodes
    /// is empty.
    //
    // Needs to be implemented here because of limitations with templates.
    template <class N>
    void replaceTopBody(const std::vector<const N *> *nodes) {
        BUG_CHECK(!nodes->empty(), "Replaced top of execution stack with empty list");
        body.pop();
        for (auto it = nodes->rbegin(); it != nodes->rend(); ++it) {
            BUG_CHECK(*it, "Evaluation produced a null node.");
            body.push(*it);
        }
    }

    /// Pops the top element of @body
    void popBody();

    /// Replaces @body with the given argument.
    void replaceBody(const Continuation::Body &body);

    /* =========================================================================================
     *  Continuation-stack operations
     * ========================================================================================= */
    /// Pushes a new frame onto the continuation stack.
    void pushContinuation(const StackFrame &frame);

    /// Pushes the current body and namespace context as a new frame on the continuation stack. The
    /// new frame will have a parameterless continuation, and will use the given set of exception
    /// handlers.
    void pushCurrentContinuation(StackFrame::ExceptionHandlers handlers);

    /// Pushes the current body and namespace context as a new frame on the continuation stack. The
    /// new frame will use the given set of exception handlers. If @parameterType_opt is given,
    /// then the continuation in the new frame will have a function parameter, with the given type,
    /// that is ignored by the body.
    void pushCurrentContinuation(std::optional<const IR::Type *> parameterType_opt = std::nullopt,
                                 StackFrame::ExceptionHandlers = {});

    /// Pops the topmost frame in the stack of continuations. From the popped frame, the
    /// continuation (after β reduction) becomes the current body, and the namespace context
    /// becomes the current namespace context. A BUG occurs if the continuation stack is empty.
    ///
    /// Expressions in the metalanguage include P4 non-expressions. Because of this, the argument
    /// (if provided) does not necessarily need to be an instance of IR::Expression.
    void popContinuation(std::optional<const IR::Node *> argument_opt = std::nullopt);

    /// Invokes first handler for e found on the stack
    void handleException(Continuation::Exception e);

    /* =========================================================================================
     *  Packet manipulation
     * ========================================================================================= */
    /// @returns the symbolic constant representing the length of the input to the current parser,
    /// in bits.
    static const IR::SymbolicVariable *getInputPacketSizeVar();

    /// @returns the maximum length, in bits, of the packet in the current packet buffer. This is
    /// the network's MTU.
    [[nodiscard]] static int getMaxPacketLength();

    /// @returns the current version of the packet that is intended as input.
    [[nodiscard]] const IR::Expression *getInputPacket() const;

    /// @returns the current size of the input packet.
    [[nodiscard]] int getInputPacketSize() const;

    /// Append data to the input packet.
    void appendToInputPacket(const IR::Expression *expr);

    /// Prepend data to the input packet.
    void prependToInputPacket(const IR::Expression *expr);

    /// @returns how much of the current input packet has been parsed.
    [[nodiscard]] int getInputPacketCursor() const;

    /// @returns the current packet buffer.
    [[nodiscard]] const IR::Expression *getPacketBuffer() const;

    /// @returns the current size of the packet buffer.
    [[nodiscard]] int getPacketBufferSize() const;

    /// Consumes and @returns a slice from the available packet buffer. If the buffer is empty, this
    /// will produce variables constants that are appended to the input packet. This means we
    /// generate packet content as needed. The returned slice is optional in case one just needs to
    /// advance.
    const IR::Expression *slicePacketBuffer(int amount);

    /// Peeks ahead into the packet buffer. Works similarly to slicePacketBuffer but does NOT
    /// advance the parser cursor or removes content from the packet buffer. However, because
    /// functions such as lookahead may still produce a parser error, this function can also enlarge
    /// the minimum input packet required.
    [[nodiscard]] const IR::Expression *peekPacketBuffer(int amount);

    /// Append data to the packet buffer.
    void appendToPacketBuffer(const IR::Expression *expr);

    /// Prepend data to the packet buffer.
    void prependToPacketBuffer(const IR::Expression *expr);

    /// Reset the packet buffer to zero.
    void resetPacketBuffer();

    /// @returns the current emit buffer
    [[nodiscard]] const IR::Expression *getEmitBuffer() const;

    /// Reset the emit buffer to zero.
    void resetEmitBuffer();

    /// Append data to the emit buffer.
    void appendToEmitBuffer(const IR::Expression *expr);

    /// Set the parser error label to the @param parserError.
    void setParserErrorLabel(const IR::StateVariable &parserError);

    /// @returns the current parser error label. Throws a BUG if the label is a nullptr.
    [[nodiscard]] const IR::StateVariable &getCurrentParserErrorLabel() const;

    /* =========================================================================================
     *  Constructors
     * ========================================================================================= */
    /// Allocate a new execution state object with the same state as this object.
    /// Returns a reference, not a pointer.
    [[nodiscard]] ExecutionState &clone() const override;

    /// Create a new execution state object from the input program.
    /// Returns a reference not a pointer.
    [[nodiscard]] static ExecutionState &create(const IR::P4Program *program);

 private:
    /// Create an initial execution state with @param body for testing.
    explicit ExecutionState(Continuation::Body body);

    // Execution state should always be allocated through explicit operators.
    /// Creates an initial execution state for the given program.
    explicit ExecutionState(const IR::P4Program *program);
    ExecutionState(const ExecutionState &) = default;

    /// Do not accidentally copy-assign the execution state.
    ExecutionState &operator=(const ExecutionState &) = default;
};

using ExecutionStateReference = std::reference_wrapper<ExecutionState>;

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXECUTION_STATE_H_ */
