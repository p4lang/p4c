#include "backends/p4tools/modules/testgen/targets/pna/constants.h"

#include "ir/id.h"

namespace P4Tools::P4Testgen::Pna {

const IR::StateVariable PnaConstants::DROP_VAR = IR::StateVariable(
    {{IR::Type_Unknown::get(), "*pna_internal"}, {new IR::Type_Boolean(), "drop_var"}});
const IR::StateVariable PnaConstants::OUTPUT_PORT_VAR = IR::StateVariable(
    {{IR::Type_Unknown::get(), "*pna_internal"}, {new IR::Type_Bits(32, false), "output_port"}});
const IR::StateVariable PnaConstants::PARSER_ERROR = IR::StateVariable(
    {{IR::Type_Unknown::get(), "*pna_internal"}, {new IR::Type_Bits(32, false), "parser_error"}});
// TODO: Make this a proper zombie variable.
// We can not use the utilities because of an issue related to the garbage collector.
const IR::Member PnaZombies::DIRECTION = IR::Member(
    new IR::Type_Bits(32, false),
    new IR::Member(new IR::Type_Bits(32, false), new IR::PathExpression("p4t*zombie"), "const"),
    "parser_error");

}  // namespace P4Tools::P4Testgen::Pna
