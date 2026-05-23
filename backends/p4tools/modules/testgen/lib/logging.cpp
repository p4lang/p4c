// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/modules/testgen/lib/logging.h"

#include "lib/log.h"

namespace P4::P4Tools::P4Testgen {

void enableTraceLogging() { Log::addDebugSpec("test_traces:4"); }

void enableStepLogging() { Log::addDebugSpec("small_step:4"); }

void enableCoverageLogging() { Log::addDebugSpec("coverage:4"); }

}  // namespace P4::P4Tools::P4Testgen
