
#include "backends/p4tools/modules/testgen/lib/logging.h"

#include "lib/log.h"

namespace P4C::P4Tools::P4Testgen {

void enableTraceLogging() { ::P4C::Log::addDebugSpec("test_traces:4"); }

void enableStepLogging() { ::P4C::Log::addDebugSpec("small_step:4"); }

void enableCoverageLogging() { ::P4C::Log::addDebugSpec("coverage:4"); }

}  // namespace P4C::P4Tools::P4Testgen
