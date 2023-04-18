#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXECUTION_STATE_H_

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stack>
#include <utility>
#include <variant>
#include <vector>

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen {

/// Represents state of execution after having reached a program point.
class ExecutionState {
    friend class Test::SmallStepTest;

    /// Specifies the type of the packet size variable.
    /// This is mostly used to generate bit vector constants.
    static const IR::Type_Bits packetSizeVarType;

    /// The name of the input packet. The input packet defines the minimum size of the packet
    /// requires to pass this particular path. Typically, calls such as extract, advance, or
    /// lookahead cause the input packet to grow.
    static const IR::StateVariable inputPacketLabel;

    /// The name of packet buffer. The packet buffer defines the data that can be consumed by the
    /// parser. If the packet buffer is empty, extract/advance/lookahead calls will cause the
    /// minimum packet size to grow. The packet buffer also forms the final output packet.
    static const IR::StateVariable packetBufferLabel;

    /// The name of the emit buffer. Each time, emit is called, the emitted content is appended to
    /// this buffer. Typically, after exiting the control, the emit buffer is appended to the packet
    /// buffer.
    static const IR::StateVariable emitBufferLabel;

    /// Canonical name for the payload. This is used for consistent naming when attaching a payload
    /// to the packet.
    static const IR::StateVariable payloadLabel;

 public:
    class StackFrame {
     public:
        using ExceptionHandlers = std::map<Continuation::Exception, Continuation>;

        Continuation normalContinuation;
        ExceptionHandlers exceptionHandlers;
        const NamespaceContext *namespaces = nullptr;

        StackFrame(Continuation normalContinuation, const NamespaceContext *namespaces)
            : StackFrame(normalContinuation, {}, namespaces) {}

        StackFrame(Continuation normalContinuation, ExceptionHandlers exceptionHandlers,
                   const NamespaceContext *namespaces)
            : normalContinuation(normalContinuation),
              exceptionHandlers(exceptionHandlers),
              namespaces(namespaces) {}
    };

 private:
    /// The namespace context in the IR for the current state. The innermost element is the P4
    /// program, representing the top-level namespace.
    const NamespaceContext *namespaces;

    /// The symbolic environment. Maps program variables to their symbolic values.
    SymbolicEnv env;

    /// The list of zombies that have been created in this state.
    /// These zombies are later fed to the model for completion.
    std::set<IR::StateVariable> allocatedZombies;

    /// The program trace for the current program point (i.e., how we got to the current state).
    std::vector<std::reference_wrapper<const TraceEvent>> trace;

    /// Set of visited statements. Used for code coverage.
    P4::Coverage::CoverageSet visitedStatements;

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
    std::map<cstring, std::map<cstring, const TestObject *>> testObjects;

    /// The parserErrorLabel is set by the parser to indicate the variable corresponding to the
    /// parser error that is set by various built-in functions such as verify or extract.
    const IR::StateVariable *parserErrorLabel = nullptr;

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

    /// State that is needed to track reachability of statements given a query.
    ReachabilityEngineState *reachabilityEngineState = nullptr;

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
    [[nodiscard]] const IR::Expression *get(const IR::StateVariable &var) const;

    /// Checks whether the statement has been visited in this state.
    void markVisited(const IR::Statement *stmt);

    /// @returns list of all statements visited before reaching this state.
    [[nodiscard]] const P4::Coverage::CoverageSet &getVisited() const;

    /// Sets the symbolic value of the given state variable to the given value. Constant folding
    /// is done on the given value before updating the symbolic state.
    void set(const IR::StateVariable &var, const IR::Expression *value);

    /// Checks whether the given variable exists in the symbolic environment of this state.
    [[nodiscard]] bool exists(const IR::StateVariable &var) const;

    /// @see Taint::hasTaint
    bool hasTaint(const IR::Expression *expr) const;

    /// @returns the current symbolic environment.
    [[nodiscard]] const SymbolicEnv &getSymbolicEnv() const;

    /// Produce a formatted output of the current symbolic environment.
    void printSymbolicEnv(std::ostream &out = std::cout) const;

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

    /// @returns the uninterpreted test object using the provided category and object label. If
    /// @param checked is enabled, a BUG is thrown if the object label does not exist.
    /// Also casts the test object to the specified type. If the type does not match, a BUG is
    /// thrown.
    template <class T>
    [[nodiscard]] T *getTestObject(cstring category, cstring objectLabel, bool checked) const {
        const auto *testObject = getTestObject(category, objectLabel, checked);
        return testObject->checkedTo<T>();
    }

    /// @returns the map of uninterpreted test objects for a specific test category. For example,
    /// all the table entries saved under "tableconfigs".
    [[nodiscard]] std::map<cstring, const TestObject *> getTestObjectCategory(
        cstring category) const;

    [[nodiscard]] ReachabilityEngineState *getReachabilityEngineState() const;

    void setReachabilityEngineState(ReachabilityEngineState *newEngineState);

    /* =========================================================================================
     *  Trace events.
     * ========================================================================================= */
 public:
    void add(const TraceEvent &event);

    /* =========================================================================================
     *  Namespaces and declarations
     * ========================================================================================= */
 public:
    /// Looks up a declaration from a path. A BUG occurs if no declaration is found.
    [[nodiscard]] const IR::IDeclaration *findDecl(const IR::Path *path) const;

    /// Looks up a declaration from a path expression. A BUG occurs if no declaration is found.
    [[nodiscard]] const IR::IDeclaration *findDecl(const IR::PathExpression *pathExpr) const;

    /// Resolves a Type in the current environment.
    [[nodiscard]] const IR::Type *resolveType(const IR::Type *type) const;

    /// @returns the current namespace context.
    [[nodiscard]] const NamespaceContext *getNamespaceContext() const;

    /// Replaces the namespace context in the current state with the given context.
    void setNamespaceContext(const NamespaceContext *namespaces);

    /// Enters a namespace of declarations.
    void pushNamespace(const IR::INamespace *ns);

    /* =========================================================================================
     *  Body operations
     * ========================================================================================= */
 public:
    /// Replaces the top element of @body with @cmd.
    void replaceTopBody(const Continuation::Command cmd);

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
 public:
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
 public:
    /// @returns the bit type of the parser cursor.
    [[nodiscard]] static const IR::Type_Bits *getPacketSizeVarType();

    /// @returns the symbolic constant representing the length of the input to the current parser,
    /// in bits.
    static const IR::StateVariable *getInputPacketSizeVar();

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
    /// will produce zombie constants that are appended to the input packet. This means we generate
    /// packet content as needed. The returned slice is optional in case one just needs to advance.
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

    /// @returns the label associated with the payload and sets the type according to the @param.
    /// TODO: Consider moving this to a separate utility class?
    [[nodiscard]] static const IR::StateVariable *getPayloadLabel(const IR::Type *t);

    /// Set the parser error label to the @param parserError.
    void setParserErrorLabel(const IR::StateVariable *parserError);

    /// @returns the current parser error label. Throws a BUG if the label is a nullptr.
    [[nodiscard]] const IR::StateVariable *getCurrentParserErrorLabel() const;

    /* =========================================================================================
     *  Variables and symbolic constants.
     * =========================================================================================
     *  These mirror what's available in IRUtils, but automatically fill in the incarnation number,
     *  based on how many times newParser() has been called.
     */
 public:
    /// @returns the zombies that were allocated in this state
    [[nodiscard]] const std::set<IR::StateVariable> &getZombies() const;

    /// @see Utils::getZombieConst.
    /// We also place the zombies in the set of allocated zombies of this state.
    [[nodiscard]] const IR::StateVariable *createZombieConst(const IR::Type *type, cstring name,
                                                             uint64_t instanceID = 0);

    /* =========================================================================================
     *  General utilities involving ExecutionState.
     * ========================================================================================= */
 public:
    /// Takes an input struct type @ts and a prefix @parent and appends each field of the struct
    /// type to the provided vector @flatFields. The result is a vector of all in the bit and bool
    /// members in canonical representation (e.g., {"prefix.h.ethernet.dst_address",
    /// "prefix.h.ethernet.src_address", ...}).
    /// If @arg validVector is provided, this function also collects the validity bits of the
    /// headers.
    [[nodiscard]] std::vector<const IR::StateVariable *> getFlatFields(
        const IR::Expression *parent, const IR::Type_StructLike *ts,
        std::vector<const IR::StateVariable *> *validVector = nullptr) const;

    /// Gets table type from a member.
    /// @returns nullptr is member type is not a IR::P4Table.
    [[nodiscard]] const IR::P4Table *getTableType(const IR::Expression *expression) const;

    /// Gets action type from an expression.
    [[nodiscard]] const IR::P4Action *getActionDecl(const IR::Expression *expression) const;

    /// @returns a translation of the path expression into a member.
    /// This function looks up the path expression in the namespace and tries to find the
    /// corresponding declaration. It then converts the name of the declaration into a zombie
    /// constant and returns. This is necessary because we sometimes
    /// get flat declarations without members (e.g., bit<8> tmp;)
    [[nodiscard]] const IR::StateVariable *convertPathExpr(const IR::PathExpression *path) const;

    /// Allocate a new execution state object with the same state as this object.
    /// Returns a reference, not a pointer.
    [[nodiscard]] ExecutionState &clone() const;

    /// Create a new execution state object from the input program.
    /// Returns a reference not a pointer.
    [[nodiscard]] static ExecutionState &create(const IR::P4Program *program);

    /* =========================================================================================
     *  Constructors
     * ========================================================================================= */
 private:
    /// Create an initial execution state with @param body for testing.
    explicit ExecutionState(Continuation::Body body);

    // Execution state should always be allocated through explicit operators.
    /// Creates an initial execution state for the given program.
    explicit ExecutionState(const IR::P4Program *program);
    ExecutionState(const ExecutionState &) = default;
    ExecutionState &operator=(const ExecutionState &) = default;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXECUTION_STATE_H_ */
