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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_SHARED_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_SHARED_EXPR_STEPPER_H_

#include "ir/ir.h"
#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/* This unit contains expression operations that are shared between Tofino 1 and 2. The purpose is
 * to avoid excessive code duplication. */
class SharedTofinoExprStepper : public ExprStepper {
 protected:
    PacketCursorAdvanceInfo calculateSuccessfulParserAdvance(const ExecutionState &state,
                                                             int advanceSize) const override;

    PacketCursorAdvanceInfo calculateAdvanceExpression(
        const ExecutionState &state, const IR::Expression *advanceExpr,
        const IR::Expression *restrictions) const override;

    /* ======================================================================================
     *  (Direct)Meter.execute
     *  See also: https://wiki.ith.intel.com/display/BXDCOMPILER/Meters
     * ====================================================================================== */
    static const ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl METER_EXECUTE;
    /* ======================================================================================
     *  DirectCount.count
     *  See also:
     *  https://wiki.ith.intel.com/pages/viewpage.action?pageId=1981604291#Externs-_sk26xix6q9fxCounters
     * ====================================================================================== */
    static const ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl COUNTER_COUNT;
    /* ======================================================================================
     *  Checksum.update
     *  Calculate the checksum for a  given list of fields.
     *  @param data : List of fields contributing to the checksum value.
     *  @param zeros_as_ones : encode all-zeros value as all-ones.
     * ====================================================================================== */
    static const ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl CHECKSUM_UPDATE;
    /* ======================================================================================
     *  Mirror.emit
     *  Sends copy of the packet to a specified destination.
     *  See also:
     *  https://wiki.ith.intel.com/display/BXDCOMPILER/Externs#Externs-Mirror
     * ====================================================================================== */
    // TODO: Currently implemented as no-op. We rely on the fact that when mirroring is not
    // configured by the control plane the mirrored packets are discarded and hence not seen
    // by the STF/PTF test.
    static const ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl MIRROR_EMIT;

    /* ======================================================================================
     *  RegisterAction.execute
     *  See also: https://wiki.ith.intel.com/display/BXDCOMPILER/Stateful+ALU
     * ====================================================================================== */
    static const ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl REGISTER_ACTION_EXECUTE;
    /* ======================================================================================
     *  DirectRegisterAction.execute
     *  Currently, this function is almost identical to RegisterAction.execute. The only
     * difference is the amount of expected type arguments.
     * ====================================================================================== */
    static const ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
        DIRECT_REGISTER_ACTION_EXECUTE;

    // Provides implementations of Tofino externs.
    static const ExternMethodImpls<SharedTofinoExprStepper> SHARED_TOFINO_EXTERN_METHOD_IMPLS;

 public:
    SharedTofinoExprStepper(ExecutionState &state, AbstractSolver &solver,
                            const ProgramInfo &programInfo)
        : ExprStepper(state, solver, programInfo) {}

    void evalExternMethodCall(const ExternInfo &externInfo) override;

    bool preorder(const IR::P4Table *table) override;
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_SHARED_EXPR_STEPPER_H_ */
