
#include "backends/p4tools/modules/testgen/lib/logging.h"

#include "lib/log.h"

namespace p4c::P4Tools::P4Testgen {

void enableTraceLogging() { ::p4c::Log::addDebugSpec("test_traces:4"); }

void enableStepLogging() { ::p4c::Log::addDebugSpec("small_step:4"); }

void enableCoverageLogging() { ::p4c::Log::addDebugSpec("coverage:4"); }

}  // namespace p4c::P4Tools::P4Testgen
