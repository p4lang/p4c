#ifndef BACKENDS_P4TOOLS_TESTGEN_CORE_SMALL_STEP_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_TESTGEN_CORE_SMALL_STEP_EXPR_STEPPER_H_

#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/small_step/abstract_stepper.h"
#include "backends/p4tools/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

class ExprStepper;

/// Implements small-step operational semantics for expressions.
class ExprStepper : public AbstractStepper {
 private:
    /// We delegate evaluation to the TableStepper, which needs to access protected members.
    friend class TableStepper;

    /// Extract utils may access some protected members of the expression stepper.
    friend class ExtractUtils;

 protected:
    /// Contains information that is useful for externs that advance the parser cursor.
    /// For example, advance, extract, or lookahead.
    struct PacketCursorAdvanceInfo {
        /// How much the parser cursor will be advanced in a successful parsing case.
        int advanceSize;

        /// The condition that needs to be satisfied to successfully advance the parser cursor.
        const IR::Expression* advanceCond;

        /// Specifies at what point the parser cursor advancement will fail.
        int advanceFailSize;

        /// The condition that needs to be satisfied for the advance/extract to be rejected.
        const IR::Expression* advanceFailCond;
    };

    /// Calculates the conditions that need to be satisfied for a successful parser advance.
    /// This assumes that the advance amount is known already and a compile-time constant.
    /// Targets may override this function with custom behavior.
    virtual PacketCursorAdvanceInfo calculateSuccessfulParserAdvance(const ExecutionState& state,
                                                                     int advanceSize) const;
    /// Calculates the conditions that need to be satisfied for a successful parser advance.
    /// This assumes that the advance amount is a run-time value.
    //// We need to pick a satisfying value assignment for a reject or advance of the parser.
    /// Targets may override this function with custom behavior.
    virtual PacketCursorAdvanceInfo calculateAdvanceExpression(
        const ExecutionState& state, const IR::Expression* advanceExpr,
        const IR::Expression* restrictions) const;

    /// Iterate over the fields in @param flatFields and set the corresponding values in
    /// @param nextState. If there is a varbit, assign the @param varbitFieldSize as size to
    /// it.
    static void setFields(ExecutionState* nextState,
                          const std::vector<const IR::Member*>& flatFields, int varBitFieldSize);
    /// This function call is used in member expressions to cleanly resolve hit, miss, and action
    /// run expressions. These are return values of a table.apply() call, and fairly special in P4.
    /// We have to use this rewrite to execute the table, and then return the corresponding
    /// values for hit, miss and action_run after that.
    void handleHitMissActionRun(const IR::Member* member);

    /// Evaluates a call to an extern method. Upon return, the given result will be augmented with
    /// the successor states resulting from evaluating the call.
    ///
    /// @param call the original method call expression, can be used for stepInto calls.
    /// @param receiver a symbolic value representing the object on which the method is being
    ///     called.
    /// @param name the name of the method being called.
    /// @param args the list of arguments being passed to method.
    /// @param state the state in which the call is being made, with the call at the top of the
    ///     current continuation body.
    /// TODO(fruffy): Move this call out of the expression stepper. The location is confusing.
    virtual void evalExternMethodCall(const IR::MethodCallExpression* call,
                                      const IR::Expression* receiver, IR::ID name,
                                      const IR::Vector<IR::Argument>* args, ExecutionState& state);
    /// Evaluates a call to an extern method that only exists in the interpreter. These are helper
    /// functions used to execute custom operations and specific control flow. They do not exist as
    /// P4 code or call.
    ///
    /// @param call the original method call expression, can be used for stepInto calls.
    /// @param receiver a symbolic value representing the object on which the method is being
    ///     called.
    /// @param name the name of the method being called.
    /// @param args the list of arguments being passed to method.
    /// @param state the state in which the call is being made, with the call at the top of the
    ///     current continuation body.
    /// TODO(fruffy): Move this call out of the expression stepper. The location is confusing.
    virtual void evalInternalExternMethodCall(const IR::MethodCallExpression* call,
                                              const IR::Expression* receiver, IR::ID name,
                                              const IR::Vector<IR::Argument>* args,
                                              const ExecutionState& state);

    /// Evaluates a call to an action. This usually only happens when a table is invoked.
    /// In other cases, actions should be inlined. When the action call is evaluated, we use
    /// zombie variables to pass arguments across execution boundaries. These variables persist
    /// until the end of program execution.
    /// @param action the action declaration that is being referenced.
    /// @param call the actual method call containing the arguments.
    void evalActionCall(const IR::P4Action* action, const IR::MethodCallExpression* call);

    /// @returns an assignment corresponding to the direction @dir that is provided. In the case
    /// of "out", we reset. If @targetPath is the destination we will write to. If @srcPath does
    /// not exist, we create a new zombie variable for it.
    /// If @param forceTaint is true, out will set the parameter to tainted.
    // Otherwise, the target default value is chosen.
    /// TODO: Consolidate this into the copy_in_out extern.
    void generateCopyIn(ExecutionState* nextState, const IR::Expression* targetPath,
                        const IR::Expression* srcPath, cstring dir, bool forceTaint) const;

    /// Takes a step to reflect a "select" expression failing to match. The default implementation
    /// raises Continuation::Exception::NoMatch.
    virtual void stepNoMatch();

 public:
    ExprStepper(const ExprStepper&) = default;

    ExprStepper(ExprStepper&&) = default;

    ExprStepper& operator=(const ExprStepper&) = delete;

    ExprStepper& operator=(ExprStepper&&) = delete;

    ExprStepper(ExecutionState& state, AbstractSolver& solver, const ProgramInfo& programInfo);

    bool preorder(const IR::BoolLiteral* boolLiteral) override;
    bool preorder(const IR::Constant* constant) override;
    bool preorder(const IR::Member* member) override;
    bool preorder(const IR::MethodCallExpression* call) override;
    bool preorder(const IR::Mux* mux) override;
    bool preorder(const IR::PathExpression* pathExpression) override;

    /// This is a special function that handles the case where structure include P4ValueSet.
    /// Returns an updated structure, replacing P4ValueSet with a list of P4ValueSet components,
    /// splitting the list into separate keys if possible
    bool preorder(const IR::P4ValueSet* valueSet) override;
    bool preorder(const IR::Operation_Binary* binary) override;
    bool preorder(const IR::Operation_Unary* unary) override;
    bool preorder(const IR::SelectExpression* selectExpression) override;
    bool preorder(const IR::ListExpression* listExpression) override;
    bool preorder(const IR::StructExpression* structExpression) override;
    bool preorder(const IR::Slice* slice) override;
    bool preorder(const IR::P4Table* table) override;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_CORE_SMALL_STEP_EXPR_STEPPER_H_ */
