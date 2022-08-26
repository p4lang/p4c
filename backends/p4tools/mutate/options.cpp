#include "backends/p4tools/mutate/options.h"

#include <cstdlib>
#include <iostream>

namespace P4Tools {

MutateOptions& MutateOptions::get() {
    static MutateOptions INSTANCE;
    return INSTANCE;
}

const char* MutateOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for P4Mutate.");
}

MutateOptions::MutateOptions() : AbstractP4cToolOptions("P4mutate options.") {
    P4C_UNIMPLEMENTED("Mutate options not implemented yet.");
}

}  // namespace P4Tools
