/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TEST_BACKEND_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TEST_BACKEND_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_program_info.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_spec.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class TofinoTestBackend : public TestBackEnd {
 private:
    static const std::vector<std::string> SUPPORTED_BACKENDS;

 public:
    explicit TofinoTestBackend(const TofinoSharedProgramInfo &programInfo,
                               const TestBackendConfiguration &testBackendConfiguration,
                               SymbolicExecutor &symbex);

    TestInfo produceTestInfo(
        const ExecutionState *executionState, const Model *finalModel,
        const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
        const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces) override;

    const TestSpec *createTestSpec(const ExecutionState *executionState, const Model *finalModel,
                                   const TestInfo &testInfo) override;

    bool printTestInfo(const ExecutionState *executionState, const TestInfo &testInfo,
                       const IR::Expression *outputPortExpr) override;
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TEST_BACKEND_H_ */
