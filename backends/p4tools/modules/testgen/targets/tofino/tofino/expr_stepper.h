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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO_EXPR_STEPPER_H_

#include <string>

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_expr_stepper.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class Tofino1ExprStepper : public SharedTofinoExprStepper {
 protected:
    std::string getClassName() override { return "Tofino1ExprStepper"; }

    static const ExternMethodImpls<Tofino1ExprStepper> TOFINO_EXTERN_METHOD_IMPLS;

 public:
    Tofino1ExprStepper(ExecutionState &state, AbstractSolver &solver,
                       const ProgramInfo &programInfo);

    void evalExternMethodCall(const ExternInfo &externInfo) override;
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO_EXPR_STEPPER_H_ */
