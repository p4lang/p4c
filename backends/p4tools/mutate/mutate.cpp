#include "backends/p4tools/mutate/mutate.h"

#include <cstdlib>
#include <iostream>

#include "ir/ir.h"

namespace P4Tools {

namespace P4Mutate {

void Mutate::registerTarget() { P4C_UNIMPLEMENTED("Mutate not implemented yet."); }

int Mutate::mainImpl(const IR::P4Program* /*program*/) {
    P4C_UNIMPLEMENTED("Mutate not implemented yet.");
    return EXIT_SUCCESS;
}
}  // namespace P4Mutate

}  // namespace P4Tools
