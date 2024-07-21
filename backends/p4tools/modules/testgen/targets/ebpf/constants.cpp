#include "backends/p4tools/modules/testgen/targets/ebpf/constants.h"

namespace p4c::P4Tools::P4Testgen::EBPF {

const IR::PathExpression EBPFConstants::ACCEPT_VAR =
    IR::PathExpression(IR::Type_Boolean::get(), new IR::Path("*accept"));

}  // namespace p4c::P4Tools::P4Testgen::EBPF
