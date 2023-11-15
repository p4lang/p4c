#ifndef BACKENDS_P4TOOLS_COMMON_CORE_ABSTRACT_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_COMMON_CORE_ABSTRACT_EXECUTION_STATE_H_

#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "ir/ir.h"

namespace P4Tools {

/// Represents state of execution after having reached a program point.
class AbstractExecutionState {
 protected:
    /// The namespace context in the IR for the current state. The innermost element is the P4
    /// program, representing the top-level namespace.
    const NamespaceContext *namespaces;

    /// The symbolic environment. Maps program variables to their symbolic values.
    SymbolicEnv env;

    /* =========================================================================================
     *  Constructors
     * ========================================================================================= */
    /// Execution state needs to be explicitly copied using the @ref clone call..
    AbstractExecutionState(const AbstractExecutionState &) = default;

    /// Used for debugging and testing.
    AbstractExecutionState();

    /// Do not accidentally copy-assign the execution state.
    AbstractExecutionState &operator=(const AbstractExecutionState &) = default;

 public:
    /// Creates an initial execution state for the given program.
    explicit AbstractExecutionState(const IR::P4Program *program);
    virtual ~AbstractExecutionState() = default;
    AbstractExecutionState &operator=(AbstractExecutionState &&) = delete;

    /// Allocate a new execution state object with the same state as this object.
    /// Returns a reference, not a pointer.
    [[nodiscard]] virtual AbstractExecutionState &clone() const = 0;

    AbstractExecutionState(AbstractExecutionState &&) = default;

    /* =========================================================================================
     *  Accessors
     * ========================================================================================= */
 public:
    /// @returns the value associated with the given state variable.
    [[nodiscard]] virtual const IR::Expression *get(const IR::StateVariable &var) const = 0;

    /// Sets the symbolic value of the given state variable to the given value. Constant folding
    /// is done on the given value before updating the symbolic state.
    virtual void set(const IR::StateVariable &var, const IR::Expression *value) = 0;

    /// Checks whether the given variable exists in the symbolic environment of this state.
    [[nodiscard]] bool exists(const IR::StateVariable &var) const;

    /// @returns the current symbolic environment.
    [[nodiscard]] const SymbolicEnv &getSymbolicEnv() const;

    /// Produce a formatted output of the current symbolic environment.
    void printSymbolicEnv(std::ostream &out = std::cout) const;

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

    /// Exists a namespace of declarations.
    void popNamespace();

    /* =========================================================================================
     *  General utilities involving ExecutionState.
     * ========================================================================================= */
 protected:
    /// Convert the input reference into a complex expression such as a HeaderExpression,
    /// StructExpression, or HeaderStackExpression. @returns nullptr if the expression is not
    /// complex.
    const IR::Expression *convertToComplexExpression(const IR::StateVariable &parent) const;

    /// Takes in a complex expression as a StructExpression, ListExpression, or
    /// HeaderStackExpression, flattens it into a vector and @returns this vector. In parallel this
    /// function fills a vector of the validity variables of any Type_Header encountered in this
    /// expression.
    static std::vector<const IR::Expression *> flattenComplexExpression(
        const IR::Expression *inputExpression, std::vector<const IR::Expression *> &flatValids);

 public:
    /// Takes an input struct type @ts and a prefix @parent and returns a vector of references to
    /// members of the struct. The vector contains all the Type_Base (bit and bool) members in
    /// canonical representation (e.g.,
    /// {"prefix.h.ethernet.dst_address", "prefix.h.ethernet.src_address", ...}). If @arg
    /// validVector is provided, this function also collects the validity bits of the headers.
    [[nodiscard]] std::vector<IR::StateVariable> getFlatFields(
        const IR::StateVariable &parent,
        std::vector<IR::StateVariable> *validVector = nullptr) const;

    /// Initialize all the members of a struct-like object by calling the initialization function of
    /// the active target. Headers validity is set to "false".
    void initializeStructLike(const Target &target, const IR::StateVariable &targetVar,
                              bool forceTaint);

    /// Assign a struct-like expression to @param left. Unrolls @param right into a series of
    /// individual assignments.
    void assignStructLike(const IR::StateVariable &left, const IR::Expression *right);

    /// Set the members of struct-like @target with the values of struct-like @source.
    void setStructLike(const IR::StateVariable &targetVar, const IR::StateVariable &sourceVar);

    /// Initialize a Declaration_Variable to its default value.
    /// Does not expect an initializer.
    void declareVariable(const Target &target, const IR::Declaration_Variable &declVar);

    /// Copy the values referenced by @param @externalParamName into the values references by @param
    /// @internalParam. Any parameter with the direction "out" is de-initialized.
    void copyIn(const Target &target, const IR::Parameter *internalParam,
                cstring externalParamName);

    /// Copy the values referenced by @param @internalParam into the values references by @param
    /// @externalParamName. Only parameters with the direction out or inout are copied.
    void copyOut(const IR::Parameter *internalParam, cstring externalParamName);

    /// Initialize a set of parameters contained in @param @blockParams to their default values. The
    /// default values are specified by the target.
    void initializeBlockParams(const Target &target, const IR::Type_Declaration *typeDecl,
                               const std::vector<cstring> *blockParams);

    /// Looks up the @param member in the environment of @param state. Returns nullptr if the member
    /// is not a table type.
    const IR::P4Table *findTable(const IR::Member *member) const;

    // Try to extract and @return the IR::P4Action from the action call.
    /// This is done by looking up the reference in the execution state.
    /// Throws a BUG, if the action does not exist.
    [[nodiscard]] const IR::P4Action *getP4Action(const IR::MethodCallExpression *actionExpr) const;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_CORE_ABSTRACT_EXECUTION_STATE_H_ */
