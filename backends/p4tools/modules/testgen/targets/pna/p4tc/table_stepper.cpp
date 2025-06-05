#include "backends/p4tools/modules/testgen/targets/pna/p4tc/table_stepper.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_table_stepper.h"

namespace P4::P4Tools::P4Testgen::Pna {

PnaP4TCTableStepper::PnaP4TCTableStepper(PnaP4TCExprStepper *stepper, const IR::P4Table *table)
    : SharedPnaTableStepper(stepper, table) {}

}  // namespace P4::P4Tools::P4Testgen::Pna
