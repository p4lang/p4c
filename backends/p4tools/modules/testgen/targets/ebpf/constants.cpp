#include "backends/p4tools/modules/testgen/targets/ebpf/constants.h"

#include "ir/id.h"

namespace P4Tools::P4Testgen::EBPF {

const IR::PathExpression EBPFConstants::ACCEPT_VAR =
    IR::PathExpression(new IR::Type_Boolean(), new IR::Path("*accept"));

}  // namespace P4Tools::P4Testgen::EBPF
