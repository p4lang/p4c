#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_TABLE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_TABLE_STEPPER_H_

#include <cstddef>
#include <optional>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen {

/// Implements small-step operational semantics for tables.
class TableStepper {
 protected:
    ExprStepper *stepper;

    const IR::P4Table *table;

    /// KeyProperties define properties of table keys that are useful for execution.
    struct KeyProperties {
        /// The original key element
        IR::KeyElement const *key;

        /// The control plane name of the key.
        cstring name;

        /// The index of this key within the current table.
        size_t index;

        /// The match type of this key (exact, ternary, lpm, etc...)
        cstring matchType;

        /// Whether the key is tainted.
        /// For matches such as LPM and ternary, we can still generate a match.
        bool isTainted;

        explicit KeyProperties(IR::KeyElement const *key, cstring name, size_t index,
                               cstring matchType, bool isTainted)
            : key(key), name(name), index(index), matchType(matchType), isTainted(isTainted) {}
    };

    /// Basic table properties that are set when initializing the TableStepper.
    struct TableProperties {
        /// Control plane name of the table.
        cstring tableName;

        /// Whether key expressions of the table contain taint. This makes it difficult to match.
        bool tableIsTainted = false;

        /// Whether the table is constant and can not accept control plane entries.
        bool tableIsImmutable = false;

        /// Indicates whether the default action can be overridden.
        bool defaultIsImmutable = false;

        /// Ordered list of key fields with useful properties.
        std::vector<KeyProperties> resolvedKeys;
    } properties;

 public:
    /* =========================================================================================
     * Table Variable Getter functions
     * ========================================================================================= */

    /// @returns the zombie variable with the given type, for tracking an aspect of the given
    /// table. The returned variable will be named p4t*zombie.table.t.name.idx1.idx2, where t is
    /// the name of the given table. The "idx1" and "idx2" components are produced only if idx1 and
    /// idx2 are given, respectively.
    static const StateVariable *getZombieTableVar(const IR::Type *type, const IR::P4Table *table,
                                                  cstring name,
                                                  std::optional<int> idx1_opt = std::nullopt,
                                                  std::optional<int> idx2_opt = std::nullopt);

    /// @returns the boolean-typed state variable that tracks whether given table is reached.
    static const StateVariable *getTableReachedVar(const IR::P4Table *table);

    /// @returns the state variable that tracks the value read by the key at the given index in the
    /// given the table.
    static const StateVariable *getTableKeyReadVar(const IR::P4Table *table, int keyIdx);

    /// @returns the boolean-typed state variable that tracks whether a table has resulted in a hit.
    /// The value of this variable is false if the table misses or is not reached.
    static const StateVariable *getTableHitVar(const IR::P4Table *table);

    /// @returns the state variable that tracks the index of the action taken by the given table.
    ///
    /// This variable is initially set to the number of actions in the table, indicating that no
    /// action has been selected. It is set by setTableAction and read by getTableAction.
    static const StateVariable *getTableActionVar(const IR::P4Table *table);

 protected:
    /* =========================================================================================
     * Table Utility functions
     * ========================================================================================= */
    /// @returns the execution state of the friend class ExpressionStepper.
    const ExecutionState *getExecutionState();

    /// @returns the program info of the friend class ExpressionStepper.
    const ProgramInfo *getProgramInfo();

    /// @returns the result pointer of the friend class ExpressionStepper.
    ExprStepper::Result getResult();

    /// The function compares the right and left expression.
    /// Returns true if the values are equal or if the left value is more precise.
    /// If the left value is constant entries and the right value
    /// is Mask/Default then priority is given to constants.
    /// For more detailed information see
    /// https://github.com/p4lang/behavioral-model/blob/main/docs/simple_switch.md#longest-prefix-match-tables
    /// @param lpmIndex indicates the index of the lpm entry in the list of entries.
    /// It is used for comparison.
    static bool compareLPMEntries(const IR::Entry *leftIn, const IR::Entry *rightIn,
                                  size_t lpmIndex);

    /// @returns whether the table is immutable, meaning control plane entries can not be added.
    void checkTableIsImmutable();

    /// Add the default action path to the expression stepper. If only hits if @param
    /// tableMissCondition is true.
    void addDefaultAction(std::optional<const IR::Expression *> tableMissCondition);

    /// Helper function that collects the list of actions contained in the table.
    std::vector<const IR::ActionListElement *> buildTableActionList();

    /// Sets the action taken by the given table. The arguments in the given MethodCallExpression
    /// are assumed to be symbolic values.
    void setTableAction(ExecutionState &nextState, const IR::MethodCallExpression *actionCall);

    /// A helper function to iteratively resolve table keys into symbolic values.
    /// This function returns false, if no key needs to be resolved.
    /// If keys are tainted or a key needs to be resolved, this function returns true.
    bool resolveTableKeys();

    /// This function allows target back ends to add their own match types.
    /// For example, some back ends implement the "optional" match type, which either hits as exact
    /// match or does not match at all. The table stepper first checks these custom match types. If
    /// these do not match it steps through the default implementation. If it does not match either,
    /// a P4C_UNIMPLEMENTED is thrown.
    virtual const IR::Expression *computeTargetMatchType(ExecutionState &nextState,
                                                         const KeyProperties &keyProperties,
                                                         TableMatchMap *matches,
                                                         const IR::Expression *hitCondition);

    /// A helper function that computes whether a control-plane/table-key hits or not. This does not
    /// handle constant entries, it is specialized for control plane entries.
    /// The function also tracks the list of field matches created to achieve a  hit. We later use
    /// this to insert table entries using the STF/PTF framework.
    const IR::Expression *computeHit(ExecutionState &nextState, TableMatchMap *matches);

    /// Collects properties that may be set per table. Target back end may have different semantics
    /// for table execution that need to be collect before evaluation the table.
    virtual void checkTargetProperties(
        const std::vector<const IR::ActionListElement *> &tableActionList);

    /* =========================================================================================
     * Table Evaluation functions
     * ========================================================================================= */
    /// When we hit a table.apply() method call expression we transition into the table call
    /// phase. Handling table calls is fairly involved as we need to respect the control plane
    /// manipulating these tables. This is why we are using several helper functions.
    /// @param table the table we invoke.
    void evalTableCall();

    /// Tables may have constant match-action entries. This usually implies that the table is
    /// immutable. This helper function unrolls all entries and aligns the table key with the match
    /// provided by the constant entry.
    /// @param table the table we invoke.
    /// @returns tableMissCondition reference to the current constraints of the miss condition.
    /// Any constant entry we hit implies that the table is hit, and the default action is not
    /// executed.
    const IR::Expression *evalTableConstEntries();

    /// This helper function evaluates potential insertion from the control plane. We use zombie
    /// variables to mimic an operator inserting entries. We only cover ONE entry per table for
    /// now.
    /// @param table the table we invoke.
    void evalTableControlEntries(const std::vector<const IR::ActionListElement *> &tableActionList);

    /// This is a special function that handles the case where the key of a table is tainted. This
    /// means that the entire execution of the table is tainted. We can dramatically simplify
    /// execution here and we also do not need control plane entries.
    /// The tricky part is that we still need execute all possible actions and taint whatever they
    /// may write, iff the table has constant entries.
    void evalTaintedTable();

    /// This function allows target back ends to implement their own interpretation of table
    /// execution. How a table is evaluated is target-specific.
    virtual void evalTargetTable(const std::vector<const IR::ActionListElement *> &tableActionList);

    void setTableDefaultEntries(const std::vector<const IR::ActionListElement *> &tableActionList);

 public:
    /// Table implementations in P4 are rather flexible. Eval is a delegation function that chooses
    /// the right implementation depending on the properties of the table. For example, immutable
    /// tables can not be programmed using the control plane. If an table key is tainted, we can
    /// also not predict, which action is tainted. Some frameworks may have implementation details
    /// such as annotations, action profiles/selectors that may alter semantics, too.
    bool eval();

    explicit TableStepper(ExprStepper *stepper, const IR::P4Table *table);

    virtual ~TableStepper() = default;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_TABLE_STEPPER_H_ */
