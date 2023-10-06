#ifndef TESTGEN_TARGETS_EBPF_TABLE_STEPPER_H_
#define TESTGEN_TARGETS_EBPF_TABLE_STEPPER_H_

#include <vector>

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/expr_stepper.h"

namespace P4Tools::P4Testgen::EBPF {

class EBPFTableStepper : public TableStepper {
 private:
    /// Specifies the type of the table implementation:
    /// standard: standard implementation - use normal control plane entries.
    /// constant: The table is constant - no control entries are possible.
    /// skip: Skip the implementation and just use the default entry (no entry at all).
    /// TODO: Do we need to include an array and hash implementation of the eBPF tables?
    enum class TableImplementation { standard, constant, skip };

    /// eBPF-specific table properties.
    struct EBPFProperties {
        /// The type of the table implementation.
        TableImplementation implementaton = TableImplementation::standard;
    } EBPFProperties;

 protected:
    void checkTargetProperties(
        const std::vector<const IR::ActionListElement *> &tableActionList) override;

    void evalTargetTable(
        const std::vector<const IR::ActionListElement *> &tableActionList) override;

 public:
    explicit EBPFTableStepper(EBPFExprStepper *stepper, const IR::P4Table *table);
};

}  // namespace P4Tools::P4Testgen::EBPF

#endif /* TESTGEN_TARGETS_EBPF_TABLE_STEPPER_H_ */
