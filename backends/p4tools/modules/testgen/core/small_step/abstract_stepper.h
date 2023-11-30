#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_ABSTRACT_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_ABSTRACT_STEPPER_H_

#include <functional>
#include <optional>
#include <string>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/solver.h"
#include "ir/vector.h"
#include "ir/visitor.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// A framework for implementing small-step operational semantics. Each instance is good for one
/// small-step evaluation.
///
/// Though this inherits from the compiler's visitor framework, it's not really a visitor. It just
/// leverages the framework to obtain multiple dispatch on IR nodes. Implementations should
/// override the @ref preorder methods for those nodes whose evaluation they support and place the
/// result of the evaluation in @ref result.
class AbstractStepper : public Inspector {
 public:
    using Branch = SmallStepEvaluator::Branch;
    using Result = SmallStepEvaluator::Result;

    /// Steps on the given node. This is the main entry point into a stepper.
    ///
    /// The given node is assumed to be derived from the command at the top of the current
    /// continuation body. No checks are done to enforce this assumption.
    Result step(const IR::Node *);

    /// Provides generic handling of unsupported nodes.
    bool preorder(const IR::Node *) override;

    AbstractStepper(ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo);

 protected:
    /// Target-specific information about the P4 program being evaluated.
    const ProgramInfo &programInfo;

    /// The state being evaluated.
    ExecutionState &state;

    /// The solver backing the state being executed.
    AbstractSolver &solver;

    /// The output of the evaluation.
    Result result;

    /// @returns the name of the dynamic type of this object.
    virtual std::string getClassName() = 0;

    virtual const ProgramInfo &getProgramInfo() const;

    /// Helper function for debugging execution of small stepper.
    void logStep(const IR::Node *node);

    /// Checks our assumption that chains of Member expressions terminate in a PathExpression.
    /// Throws a BUG if @a node is not a chain of one or more Member expressions terminating in a
    /// PathExpression.
    /// if @a node is local variable then returns created member for it.
    static void checkMemberInvariant(const IR::Node *node);

    /* *****************************************************************************************
     * Transition helper functions
     * ***************************************************************************************** */

    /// Transition function for a symbolic value. Expressions in the metalanguage include P4
    /// non-expressions. Because of this, the given node does not necessarily need to be an
    /// instance of IR::Expression.
    ///
    /// @returns false
    bool stepSymbolicValue(const IR::Node *);

    /// Transitions to an exception.
    /// @returns false
    bool stepToException(Continuation::Exception);

    /// Transitions to a subexpression of the topmost command in the current continuation body.
    ///
    /// @param rebuildCmd
    ///     Rebuilds the command containing the subexpression by replacing @subexpr with the given
    ///     parameter.
    ///
    /// @returns false
    static bool stepToSubexpr(
        const IR::Expression *subexpr, SmallStepEvaluator::Result &result,
        const ExecutionState &state,
        std::function<const Continuation::Command(const Continuation::Parameter *)> rebuildCmd);

    /// Transitions to a subexpression of the topmost command in the current continuation body. The
    /// subexpression is expected to be a ListExpression. This is a specialized version of
    /// stepToSubExpr that can be used in contexts that explicitly expect a ListExpression instead
    /// of a generic Expression.
    ///
    /// @param rebuildCmd
    ///     Rebuilds the command containing the subexpression by replacing @subexpr with the given
    ///     list expression.
    ///
    /// @returns false
    static bool stepToListSubexpr(
        const IR::BaseListExpression *subexpr, SmallStepEvaluator::Result &result,
        const ExecutionState &state,
        std::function<const Continuation::Command(const IR::BaseListExpression *)> rebuildCmd);

    /// @see stepToListSubexpr.IR::StructExpression differs slightly from IR::BaseListExpression in
    /// that the components are IR::NamedExpression instead of just IR::Expression.  To keep things
    /// simple, and to avoid excessive type casting, these two functions are kept separate.
    static bool stepToStructSubexpr(
        const IR::StructExpression *subexpr, SmallStepEvaluator::Result &result,
        const ExecutionState &state,
        std::function<const Continuation::Command(const IR::StructExpression *)> rebuildCmd);

    /// Transition function for isValid calls.
    ///
    /// @param headerRef the header instance whose validity is being queried. Arguments must be
    ///     either Members or PathExpressions.
    ///
    /// @returns false
    bool stepGetHeaderValidity(const IR::StateVariable &headerRef);

    /// Sets validity for a header if @a expr is a header. if @a expr is a part of a header union
    /// then it sets invalid for other headers in the union. Otherwise it generates an exception.
    void setHeaderValidity(const IR::StateVariable &headerRef, bool validity,
                           ExecutionState &state);

    /// Transition function for setValid and setInvalid calls.
    ///
    /// @param headerRef the header instance being set valid or invalid. This is either a Member or
    ///     a PathExpression.
    /// @param validity the validity state being assigned to the header instance.
    ///
    /// @returns false
    bool stepSetHeaderValidity(const IR::StateVariable &headerRef, bool validity);

    /// Transition function for push_front/pop_front calls.
    ///
    /// @param stackRef the stack begin push_front/pop_front. This is either a Member or
    ///     a PathExpression.
    /// @param args the list of arguments being passed to method.
    /// @param isPush is true for push_front and false otherwise.
    ///
    /// @returns false
    bool stepStackPushPopFront(const IR::Expression *stackRef, const IR::Vector<IR::Argument> *args,
                               bool isPush = true);

    /// Evaluates an expression by invoking the solver under the current collected constraints.
    /// Optionally, a condition can be provided that is temporarily added to the list of assertions.
    /// If the solver can find a solution, it @returns the assigned value to the expression.
    /// If not, this function @returns nullptr.
    const IR::Literal *evaluateExpression(const IR::Expression *expr,
                                          std::optional<const IR::Expression *> cond) const;

    /// Reset the given reference to an  uninitialized value. If the reference has a
    /// Type_StructLike, unroll the reference and reset each member.
    /// If forceTaint is active, all references are set tainted. Otherwise a target-specific
    /// mechanism is used.
    void setTargetUninitialized(ExecutionState &nextState, const IR::StateVariable &ref,
                                bool forceTaint) const;

    /// This is a helper function to declare structlike data structures.
    /// This also is used to declare the members of a stack. This function is primarily used by the
    /// Declaration_Variable preorder function.
    void declareStructLike(ExecutionState &nextState, const IR::StateVariable &parentExpr,
                           bool forceTaint = false) const;

    /// This is a helper function to declare base type variables. Because all variables need to be a
    /// member in the execution state environment, this helper function suffixes a "*".
    void declareBaseType(ExecutionState &nextState, const IR::StateVariable &paramPath,
                         const IR::Type_Base *baseType) const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_ABSTRACT_STEPPER_H_ */
