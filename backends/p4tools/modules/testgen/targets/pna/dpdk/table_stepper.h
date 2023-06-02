#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_TABLE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_TABLE_STEPPER_H_

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_table_stepper.h"

namespace P4Tools::P4Testgen::Pna {

class PnaDpdkTableStepper : public SharedPnaTableStepper {
 public:
    explicit PnaDpdkTableStepper(PnaDpdkExprStepper *stepper, const IR::P4Table *table);
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_TABLE_STEPPER_H_ */
