#include "backends/p4tools/smith/options.h"

#include <cstdlib>
#include <iostream>

namespace P4Tools {

SmithOptions& SmithOptions::get() {
    static SmithOptions INSTANCE;
    return INSTANCE;
}

const char* SmithOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for P4Smith.");
}

SmithOptions::SmithOptions() : AbstractP4cToolOptions("P4Smith options.") {
    P4C_UNIMPLEMENTED("Smith options not implemented yet.");
}
}  // namespace P4Tools
