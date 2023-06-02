#include "backends/p4tools/modules/testgen/targets/pna/dpdk/table_stepper.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_table_stepper.h"

namespace P4Tools::P4Testgen::Pna {

PnaDpdkTableStepper::PnaDpdkTableStepper(PnaDpdkExprStepper *stepper, const IR::P4Table *table)
    : SharedPnaTableStepper(stepper, table) {}

}  // namespace P4Tools::P4Testgen::Pna
