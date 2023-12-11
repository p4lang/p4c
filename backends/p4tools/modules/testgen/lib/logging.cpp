
#include "backends/p4tools/modules/testgen/lib/logging.h"

#include "lib/log.h"

namespace P4Tools::P4Testgen {

void enableTraceLogging() { ::Log::addDebugSpec("test_traces:4"); }

void enableStepLogging() { ::Log::addDebugSpec("small_step:4"); }

void enableCoverageLogging() { ::Log::addDebugSpec("coverage:4"); }

}  // namespace P4Tools::P4Testgen
