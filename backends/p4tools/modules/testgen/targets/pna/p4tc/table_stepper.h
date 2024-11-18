#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_P4TC_TABLE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_P4TC_TABLE_STEPPER_H_

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/pna/p4tc/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_table_stepper.h"

namespace P4::P4Tools::P4Testgen::Pna {

class PnaP4TCTableStepper : public SharedPnaTableStepper {
 public:
    explicit PnaP4TCTableStepper(PnaP4TCExprStepper *stepper, const IR::P4Table *table);
};

}  // namespace P4::P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_P4TC_TABLE_STEPPER_H_ */
