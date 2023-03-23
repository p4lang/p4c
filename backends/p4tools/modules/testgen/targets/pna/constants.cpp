#include "backends/p4tools/modules/testgen/targets/pna/constants.h"

#include "ir/ir-generated.h"
#include "ir/irutils.h"

namespace P4Tools::P4Testgen::Pna {

const IR::Member PnaConstants::DROP_VAR =
    IR::Member(new IR::Type_Boolean(), new IR::PathExpression("*pna_internal"), "drop_var");
const IR::Member PnaConstants::OUTPUT_PORT_VAR =
    IR::Member(new IR::Type_Bits(32), new IR::PathExpression("*pna_internal"), "output_port");
}  // namespace P4Tools::P4Testgen::Pna
