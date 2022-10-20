#ifndef TESTGEN_TARGETS_EBPF_TABLE_STEPPER_H_
#define TESTGEN_TARGETS_EBPF_TABLE_STEPPER_H_

#include <map>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/lib/test_spec.h"
#include "backends/p4tools/testgen/targets/ebpf/expr_stepper.h"
#include "backends/p4tools/testgen/targets/ebpf/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFTableStepper : public TableStepper {
 private:
    /// Specifies the type of the table implementation:
    /// standard: standard implementation - use normal control plane entries.
    /// constant: The table is constant - no control entries are possible.
    /// skip: Skip the implementation and just use the default entry (no entry at all).
    enum class TableImplementation { standard, constant, skip };

    /// eBPF-specific table properties.
    struct EBPFProperties {
        /// The type of the table implementation.
        TableImplementation implementaton;
    } EBPFProperties;

 protected:
    const IR::Expression* computeTargetMatchType(ExecutionState* nextState,
                                                 const KeyProperties& keyProperties,
                                                 std::map<cstring, const FieldMatch>* matches,
                                                 const IR::Expression* hitCondition) override;

    void checkTargetProperties(
        const std::vector<const IR::ActionListElement*>& tableActionList) override;

    void evalTargetTable(const std::vector<const IR::ActionListElement*>& tableActionList) override;

 public:
    explicit EBPFTableStepper(EBPFExprStepper* stepper, const IR::P4Table* table);
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_TABLE_STEPPER_H_ */
