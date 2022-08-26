#include "backends/p4tools/smith/smith.h"

#include <cstdlib>
#include <iostream>

#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

void Smith::registerTarget() { P4C_UNIMPLEMENTED("Smith targets not registered yet."); }

int Smith::mainImpl(const IR::P4Program* /*program*/) {
    P4C_UNIMPLEMENTED("Smith not implemented yet");
    return EXIT_SUCCESS;
}

}  // namespace P4Smith

}  // namespace P4Tools
