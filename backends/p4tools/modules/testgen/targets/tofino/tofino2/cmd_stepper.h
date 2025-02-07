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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO2_CMD_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO2_CMD_STEPPER_H_

#include <map>
#include <optional>
#include <string>

#include "ir/ir.h"
#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino2/program_info.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class JBayCmdStepper : public CmdStepper {
 protected:
    std::string getClassName() override { return "JBayCmdStepper"; }

    const JBayProgramInfo &getProgramInfo() const override;

    std::optional<const Constraint *> startParserImpl(const IR::P4Parser *parser,
                                                      ExecutionState &nextState) const override;

    void initializeTargetEnvironment(ExecutionState &nextState) const override;

    std::map<Continuation::Exception, Continuation> getExceptionHandlers(
        const IR::P4Parser *parser, Continuation::Body normalContinuation,
        const ExecutionState &nextState) const override;

 public:
    JBayCmdStepper(ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO2_CMD_STEPPER_H_ */
