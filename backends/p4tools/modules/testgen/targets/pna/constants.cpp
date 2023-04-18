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
// It looks like the static labels are not allocated at that point in time.
const IR::StateVariable PnaZombies::DIRECTION =
    IR::StateVariable({{IR::Type_Unknown::get(), "p4t*zombie"},
                       {IR::Type_Unknown::get(), "const"},
                       {new IR::Type_Bits(32, false), "parser_error"}});

}  // namespace P4Tools::P4Testgen::Pna
