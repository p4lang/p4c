#include "backends/p4tools/modules/testgen/targets/ebpf/constants.h"

namespace P4Tools::P4Testgen::EBPF {

const IR::PathExpression EBPFConstants::ACCEPT_VAR =
    IR::PathExpression(IR::Type_Boolean::get(), new IR::Path("*accept"));

}  // namespace P4Tools::P4Testgen::EBPF
