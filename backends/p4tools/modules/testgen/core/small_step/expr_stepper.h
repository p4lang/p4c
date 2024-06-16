#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_EXPR_STEPPER_H_

#include <cstdint>
#include <utility>
#include <vector>

#include "ir/ir.h"
#include "ir/solver.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/core/extern_info.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/abstract_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// Implements small-step operational semantics for expressions.
class ExprStepper : public AbstractStepper {
    /**************************************************************************************************
    ExternMethodImpls
    **************************************************************************************************/
 public:
    /// Encapsulates a set of extern method implementations.
    template <typename StepperType>
    class ExternMethodImpls {
     public:
        /// The type of an extern-method implementation.
        /// @param externInfo is general useful information that can be consumsed by externs
        /// (arguments, method names, etc).
        /// @param stepper is the abstract stepper invoking the function. Templated on the type of
        /// stepper invoking it.
        using MethodImpl = std::function<void(const ExternInfo &externInfo, StepperType &stepper)>;

        /// @param call the original method call expression, can be used for stepInto calls.
        /// @param receiver a symbolic value representing the object on which the method is being
        ///     called.
        /// @param name the name of the method being called.
        /// @param args the list of arguments being passed to method.
        /// @param state the state in which the call is being made, with the call at the top of the
        ///     current continuation body.
        ///
        /// @returns the matching extern-method implementation if it was found; std::nullopt
        /// otherwise.
        ///
        /// A BUG occurs if the receiver is not an extern or if the call is ambiguous.
        std::optional<MethodImpl> find(const IR::PathExpression &externObjectRef,
                                       const IR::ID &methodName,
                                       const IR::Vector<IR::Argument> &args) const {
            // We have to check the extern type here. We may receive a specialized canonical type,
            // which we need to unpack.
            const IR::Type_Extern *externType = nullptr;
            if (const auto *type = externObjectRef.type->to<IR::Type_Extern>()) {
                externType = type;
            } else if (const auto *specType =
                           externObjectRef.type->to<IR::Type_SpecializedCanonical>()) {
                CHECK_NULL(specType->substituted);
                externType = specType->substituted->checkedTo<IR::Type_Extern>();
            } else if (externObjectRef.path->name == IR::ID("*method")) {
            } else {
                BUG("Not a valid extern: %1% with member %2%. Type is %3%.", externObjectRef,
                    methodName, externObjectRef.type->node_type_name());
            }

            cstring qualifiedMethodName = externType->name + "." + methodName;
            auto submapIt = impls.find(qualifiedMethodName);
            if (submapIt == impls.end()) {
                return std::nullopt;
            }
            if (submapIt->second.count(args.size()) == 0) {
                return std::nullopt;
            }

            // Find matching methods: if any arguments are named, then the parameter name must
            // match.
            std::optional<MethodImpl> matchingImpl;
            for (const auto &pair : submapIt->second.at(args.size())) {
                const auto &paramNames = pair.first;
                const auto &methodImpl = pair.second;

                if (matches(paramNames, args)) {
                    BUG_CHECK(!matchingImpl, "Ambiguous extern method call: %1%",
                              qualifiedMethodName);
                    matchingImpl = methodImpl;
                }
            }

            return matchingImpl;
        }

     private:
        /// A two-level map. First-level keys are method names qualified by the extern type name
        /// (e.g., "packet_in.advance". Second-level keys are the number of arguments. Values are
        /// lists of matching extern-method implementations, paired with the names of the method
        /// parameters.
        std::map<cstring,
                 std::map<size_t, std::vector<std::pair<std::vector<cstring>, MethodImpl>>>>
            impls;

        /// Determines whether the given list of parameter names matches the given argument list.
        /// According to the P4 specification, a match occurs when the lists have the same length
        /// and the name of any named argument matches the name of the corresponding parameter.
        static bool matches(const std::vector<cstring> &paramNames,
                            const IR::Vector<IR::Argument> &args) {
            // Number of parameters should match the number of arguments.
            if (paramNames.size() != args.size()) {
                return false;
            }
            // Any named argument should match the name of the corresponding parameter.
            for (size_t idx = 0; idx < paramNames.size(); idx++) {
                const auto &paramName = paramNames.at(idx);
                const auto &arg = args.at(idx);

                if (arg->name.name == nullptr) {
                    continue;
                }
                if (paramName != arg->name.name) {
                    return false;
                }
            }

            return true;
        }

     public:
        /// Represents a list of extern method implementations. The components of each element in
        /// this list are as follows:
        ///   1. The method name qualified by the extern type name (e.g., "packet_in.advance").
        ///   2. The names of the method parameters.
        ///   3. The method's implementation.
        using ImplList = std::list<std::tuple<cstring, std::vector<cstring>, MethodImpl>>;

        explicit ExternMethodImpls(const ImplList &implList) {
            for (const auto &implSpec : implList) {
                auto &[name, paramNames, impl] = implSpec;

                auto &tmpImplList = impls[name][paramNames.size()];

                // Make sure that we have at most one implementation for each set of parameter
                // names. This is a quadratic-time algorithm, but should be fine, since we expect
                // the number of overloads to be small in practice.
                for (auto &pair : tmpImplList) {
                    BUG_CHECK(pair.first != paramNames, "Multiple implementations of %1%(%2%)",
                              name, paramNames);
                }

                tmpImplList.emplace_back(paramNames, impl);
            }
        }
    };

    /// Definitions of internal helper functions.
    static const ExprStepper::ExternMethodImpls<ExprStepper> INTERNAL_EXTERN_METHOD_IMPLS;

    /// Provides implementations of all known extern methods built into P4 core.
    static const ExprStepper::ExternMethodImpls<ExprStepper> CORE_EXTERN_METHOD_IMPLS;

    /**************************************************************************************************
    ExprStepper
    **************************************************************************************************/

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
        const IR::Expression *advanceCond;

        /// Specifies at what point the parser cursor advancement will fail.
        int advanceFailSize;

        /// The condition that needs to be satisfied for the advance/extract to be rejected.
        const IR::Expression *advanceFailCond;
    };

    /// Calculates the conditions that need to be satisfied for a successful parser advance.
    /// This assumes that the advance amount is known already and a compile-time constant.
    /// Targets may override this function with custom behavior.
    virtual PacketCursorAdvanceInfo calculateSuccessfulParserAdvance(const ExecutionState &state,
                                                                     int advanceSize) const;
    /// Calculates the conditions that need to be satisfied for a successful parser advance.
    /// This assumes that the advance amount is a run-time value.
    //// We need to pick a satisfying value assignment for a reject or advance of the parser.
    /// Targets may override this function with custom behavior.
    virtual PacketCursorAdvanceInfo calculateAdvanceExpression(
        const ExecutionState &state, const IR::Expression *advanceExpr,
        const IR::Expression *restrictions) const;

    /// Iterate over the fields in @param flatFields and set the corresponding values in
    /// @param nextState. If there is a varbit, assign the @param varbitFieldSize as size to
    /// it. @returns the list of members and their assigned values.
    static std::vector<std::pair<IR::StateVariable, const IR::Expression *>> setFields(
        ExecutionState &nextState, const std::vector<IR::StateVariable> &flatFields,
        int varBitFieldSize);
    /// This function call is used in member expressions to cleanly resolve hit, miss, and action
    /// run expressions. These are return values of a table.apply() call, and fairly special in P4.
    /// We have to use this rewrite to execute the table, and then return the corresponding
    /// values for hit, miss and action_run after that.
    void handleHitMissActionRun(const IR::Member *member);

    /// Resolve all arguments to the method call by stepping into each argument that is not yet
    /// symbolic or a pure reference (represented as Out direction).
    /// @returns false when an argument needs to be resolved, true otherwise.
    bool resolveMethodCallArguments(const IR::MethodCallExpression *call);

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
    virtual void evalExternMethodCall(const ExternInfo &externInfo);

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
    void evalInternalExternMethodCall(const ExternInfo &externInfo);

    /// Evaluates a call to an action. This usually only happens when a table is invoked or when
    /// action is directly invoked from a control.
    /// In other cases, actions should be inlined. When the action call is evaluated, we use
    /// symbolic variables to pass arguments across execution boundaries. These variables persist
    /// until the end of program execution.
    /// @param action the action declaration that is being referenced.
    /// @param call the actual method call containing the arguments.
    void evalActionCall(const IR::P4Action *action, const IR::MethodCallExpression *call);

    /// @returns an assignment corresponding to the direction @dir that is provided. In the case
    /// of "out", we reset. If @targetPath is the destination we will write to. If @srcPath does
    /// not exist, we create a new symbolic variable for it.
    /// If @param forceTaint is true, out will set the parameter to tainted.
    // Otherwise, the target default value is chosen.
    /// TODO: Consolidate this into the copy_in_out extern.
    void generateCopyIn(ExecutionState &nextState, const IR::StateVariable &targetPath,
                        const IR::StateVariable &srcPath, cstring dir, bool forceTaint) const;

    /// Takes a step to reflect a "select" expression failing to match. If condition is given, this
    /// will create a new state that is guarded by the given condition. The default implementation
    /// raises Continuation::Exception::NoMatch.
    virtual void stepNoMatch(std::string traceLog, const IR::Expression *condition = nullptr);

 public:
    ExprStepper(const ExprStepper &) = default;

    ExprStepper(ExprStepper &&) = default;

    ExprStepper &operator=(const ExprStepper &) = delete;

    ExprStepper &operator=(ExprStepper &&) = delete;

    ExprStepper(ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo);

    bool preorder(const IR::BoolLiteral *boolLiteral) override;
    bool preorder(const IR::Constant *constant) override;
    bool preorder(const IR::Member *member) override;
    bool preorder(const IR::ArrayIndex *arr) override;
    bool preorder(const IR::MethodCallExpression *call) override;
    bool preorder(const IR::Mux *mux) override;
    bool preorder(const IR::PathExpression *pathExpression) override;

    /// This is a special function that handles the case where structure include P4ValueSet.
    /// Returns an updated structure, replacing P4ValueSet with a list of P4ValueSet components,
    /// splitting the list into separate keys if possible
    bool preorder(const IR::P4ValueSet *valueSet) override;
    bool preorder(const IR::Operation_Binary *binary) override;
    bool preorder(const IR::Operation_Unary *unary) override;
    bool preorder(const IR::SelectExpression *selectExpression) override;
    bool preorder(const IR::BaseListExpression *listExpression) override;
    bool preorder(const IR::StructExpression *structExpression) override;
    bool preorder(const IR::Slice *slice) override;
    bool preorder(const IR::P4Table *table) override;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_EXPR_STEPPER_H_ */
