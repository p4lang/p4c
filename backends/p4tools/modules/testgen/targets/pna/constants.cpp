#include "backends/p4tools/modules/testgen/targets/pna/constants.h"

#include "ir/id.h"
#include "lib/cstring.h"

namespace P4::P4Tools::P4Testgen::Pna {

using namespace ::P4::literals;

const IR::Member PnaConstants::DROP_VAR =
    IR::Member(IR::Type_Boolean::get(), new IR::PathExpression("*pna_internal"), "drop_var");
const IR::Member PnaConstants::OUTPUT_PORT_VAR =
    IR::Member(IR::Type_Bits::get(32), new IR::PathExpression("*pna_internal"), "output_port");
const IR::Member PnaConstants::PARSER_ERROR =
    IR::Member(IR::Type_Bits::get(32), new IR::PathExpression("*pna_internal"), "parser_error");
// TODO: Make this a proper variables variable.
// We can not use the utilities because of an issue related to the garbage collector.
const IR::SymbolicVariable PnaSymbolicVars::DIRECTION =
    IR::SymbolicVariable(IR::Type_Bits::get(32), "direction"_cs);

}  // namespace P4::P4Tools::P4Testgen::Pna
