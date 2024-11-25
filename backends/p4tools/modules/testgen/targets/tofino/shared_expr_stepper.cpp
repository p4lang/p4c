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

#include "backends/p4tools/modules/testgen/targets/tofino/shared_expr_stepper.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include <boost/none.hpp>

#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir-generated.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"

#include "backends/p4tools/modules/testgen/core/extern_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/tofino/constants.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_program_info.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_table_stepper.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_spec.h"

namespace P4::P4Tools::P4Testgen::Tofino {

ExprStepper::PacketCursorAdvanceInfo SharedTofinoExprStepper::calculateSuccessfulParserAdvance(
    const ExecutionState &state, int advanceSize) const {
    // Calculate the necessary size of the packet to extract successfully.
    // The minimum required size for a packet is the current cursor and the amount we are
    // advancing into the packet minus whatever has been buffered in the current buffer.
    auto minSize = state.getInputPacketCursor() + advanceSize;
    auto *cond = new IR::Geq(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                             IR::Constant::get(&PacketVars::PACKET_SIZE_VAR_TYPE, minSize));
    auto fcsLeft = std::max<int64_t>(0, state.getProperty<int64_t>("fcsLeft"_cs));
    auto rejectSize = std::max<int64_t>(0, minSize - state.getPacketBufferSize() - fcsLeft);
    const IR::Expression *notCond = nullptr;
    // Do not generate short packets when the parser error is referenced.
    // TODO: Model this exception.
    if (!(state.hasProperty("parserErrReferenced"_cs) &&
          state.getProperty<bool>("parserErrReferenced"_cs))) {
        notCond = new IR::Lss(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                              IR::Constant::get(&PacketVars::PACKET_SIZE_VAR_TYPE, rejectSize));
    }

    return {advanceSize, cond, advanceSize, notCond};
}

ExprStepper::PacketCursorAdvanceInfo SharedTofinoExprStepper::calculateAdvanceExpression(
    const ExecutionState &state, const IR::Expression *advanceExpr,
    const IR::Expression *restrictions) const {
    const auto *packetSizeVarType = &PacketVars::PACKET_SIZE_VAR_TYPE;

    // Compute the accept case.
    const auto *cursorConst = IR::Constant::get(packetSizeVarType, state.getInputPacketCursor());
    const auto *bufferSizeConst = IR::Constant::get(packetSizeVarType, state.getPacketBufferSize());
    const auto *advanceSum = new IR::Add(packetSizeVarType, cursorConst, advanceExpr);
    auto *minSize = new IR::Sub(packetSizeVarType, advanceSum, bufferSizeConst);
    // The packet size must be larger than the current parser cursor minus what is already
    // present in the buffer. The advance expression, i.e., the size of the advance can be freely
    // chosen. If bufferSizeConst is larger than the entire advance, this does not hold.
    auto *cond = new IR::LOr(
        new IR::Grt(bufferSizeConst, advanceSum),
        new IR::Geq(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(), minSize));

    int advanceVal = 0;
    const auto *advanceCond = new IR::LAnd(cond, restrictions);
    const auto *advanceConst = evaluateExpression(advanceExpr, advanceCond);
    // If we can not satisfy the advance, set the condition to nullptr.
    if (advanceConst == nullptr) {
        advanceCond = nullptr;
    } else {
        // Otherwise store both the condition and the computed value in the AdvanceInfo data
        // structure. Do not forget to add the condition that the expression must be equal to the
        // computed value.
        advanceVal = advanceConst->checkedTo<IR::Constant>()->asInt();
        advanceCond = new IR::LAnd(advanceCond, new IR::Equ(advanceConst, advanceExpr));
    }

    // Compute the reject case.
    auto fcsLeft = std::max<int64_t>(0, state.getProperty<int64_t>("fcsLeft"_cs));
    const auto *bufferSizeFcsConst =
        IR::Constant::get(packetSizeVarType, state.getPacketBufferSize() + fcsLeft);
    const auto *minSizeFcs = new IR::Sub(advanceSum, bufferSizeFcsConst);
    const IR::Expression *notAdvanceCond = new IR::Lss(bufferSizeFcsConst, advanceSum);
    notAdvanceCond = new IR::LAnd(notAdvanceCond,
                                  new IR::Lss(ExecutionState::getInputPacketSizeVar(), minSizeFcs));
    notAdvanceCond = new IR::LAnd(notAdvanceCond, restrictions);

    int notAdvanceVal = 0;
    const IR::Expression *notAdvanceConst = nullptr;
    // TODO: We only generate packets that are too short if the parser error is not
    // referenced within the control. This is because of behavior in some targets that
    // pads packets that are too short. We currently do not model this.
    // TODO: Remove this check and model it properly.
    if (!(state.hasProperty("parserErrReferenced"_cs) &&
          state.getProperty<bool>("parserErrReferenced"_cs))) {
        notAdvanceConst = evaluateExpression(advanceExpr, notAdvanceCond);
    }
    // If we can not satisfy the reject, set the condition to nullptr.
    if (notAdvanceConst == nullptr) {
        notAdvanceCond = nullptr;
    } else {
        // structure. Do not forget to add the condition that the expression must be equal to the
        // computed value.
        notAdvanceVal = notAdvanceConst->checkedTo<IR::Constant>()->asInt();
        notAdvanceCond = new IR::LAnd(notAdvanceCond, new IR::Equ(notAdvanceConst, advanceExpr));
    }

    return {advanceVal, advanceCond, notAdvanceVal, notAdvanceCond};
}

const IR::Expression *initializeRegisterParameters(const TestObject *registerState,
                                                   ExecutionState &nextState,
                                                   const ProgramInfo &programInfo,
                                                   cstring externInstanceName,
                                                   const IR::PathExpression *registerParamRef,
                                                   bool canConfigure, const IR::Expression *index) {
    const IR::Expression *registerExpr = nullptr;
    const auto *registerParamType = registerParamRef->type;
    if (registerState != nullptr) {
        if (index != nullptr) {
            auto existingRegVal = registerState->checkedTo<TofinoRegisterValue>();
            registerExpr = existingRegVal->getValueAtIndex(index);
        } else {
            auto existingRegVal = registerState->checkedTo<TofinoDirectRegisterValue>();
            registerExpr = existingRegVal->getInitialValue();
        }
        if (const auto *registerValueList = registerExpr->to<IR::StructExpression>()) {
            /// Set the register parameter based on the derived register value.
            std::vector<IR::StateVariable> validFields;
            const auto fields = nextState.getFlatFields(registerParamRef, &validFields);
            for (size_t idx = 0; idx < fields.size(); ++idx) {
                const auto &field = fields.at(idx);
                const auto *fieldVal = registerValueList->components.at(idx);
                nextState.set(field, fieldVal->expression);
            }
            // We also need to initialize the validity bits of the headers. These are true.
            for (const auto &validField : validFields) {
                nextState.set(validField, IR::BoolLiteral::get(true));
            }
        } else {
            /// Set the register parameter based on the derived register value.
            nextState.set(registerParamRef, registerExpr);
        }
    } else {
        if (const auto *structType = registerParamType->to<IR::Type_StructLike>()) {
            const IR::StructExpression *registerValueList = nullptr;
            IR::IndexedVector<IR::NamedExpression> valueVector{};
            for (const auto *structField : structType->fields) {
                const IR::NamedExpression *inputValue = nullptr;
                if (canConfigure) {
                    inputValue = new IR::NamedExpression(
                        structField->name.name,
                        ToolsVariables::getSymbolicVariable(
                            structField->type, externInstanceName + "_" + structField->name.name));
                } else {
                    inputValue = new IR::NamedExpression(
                        structField->name.name,
                        programInfo.createTargetUninitialized(structField->type, true));
                }
                valueVector.push_back(inputValue);
            }
            registerValueList = new IR::StructExpression(nullptr, valueVector);
            /// Set the register parameter based on the derived register value.
            std::vector<IR::StateVariable> validFields;
            const auto fields = nextState.getFlatFields(registerParamRef, &validFields);
            for (size_t idx = 0; idx < fields.size(); ++idx) {
                const auto &field = fields.at(idx);
                const auto *fieldVal = registerValueList->components.at(idx);
                nextState.set(field, fieldVal->expression);
            }
            // We also need to initialize the validity bits of the headers. These are true.
            for (const auto &validField : validFields) {
                nextState.set(validField, IR::BoolLiteral::get(true));
            }
            registerExpr = registerValueList;
        } else {
            if (canConfigure) {
                registerExpr =
                    ToolsVariables::getSymbolicVariable(registerParamType, externInstanceName);
            } else {
                registerExpr = programInfo.createTargetUninitialized(registerParamType, true);
            }
            /// Set the register parameter based on the derived register value.
            nextState.set(registerParamRef, registerExpr);
        }
    }
    return registerExpr;
}

/* ======================================================================================
 *  (Direct)Meter.execute
 *  See also: https://wiki.ith.intel.com/display/BXDCOMPILER/Meters
 * ====================================================================================== */
const SharedTofinoExprStepper::ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
    SharedTofinoExprStepper::METER_EXECUTE = [](const ExternInfo & /*externInfo*/,
                                                SharedTofinoExprStepper &stepper) {
        auto &nextState = stepper.state.clone();
        // For now, we return 0, which corresponds to the GREEN enum value.
        nextState.replaceTopBody(Continuation::Return(IR::Constant::get(IR::Type_Bits::get(8), 0)));

        stepper.result->emplace_back(nextState);
    };
/* ======================================================================================
 *  DirectCount.count
 *  See also:
 *  https://wiki.ith.intel.com/pages/viewpage.action?pageId=1981604291#Externs-_sk26xix6q9fxCounters
 * ====================================================================================== */
const SharedTofinoExprStepper::ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
    SharedTofinoExprStepper::COUNTER_COUNT =
        [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
            auto &nextState = stepper.state.clone();
            nextState.popBody();
            stepper.result->emplace_back(nextState);
        };

/* ======================================================================================
 *  Checksum.update
 *  Calculate the checksum for a  given list of fields.
 *  @param data : List of fields contributing to the checksum value.
 *  @param zeros_as_ones : encode all-zeros value as all-ones.
 * ====================================================================================== */
const SharedTofinoExprStepper::ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
    SharedTofinoExprStepper::CHECKSUM_UPDATE =
        [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
            auto &nextState = stepper.state.clone();
            // For now, we return taint
            // We actually should be calculating the checksum here.
            nextState.replaceTopBody(Continuation::Return(
                stepper.programInfo.createTargetUninitialized(IR::Type_Bits::get(16), true)));

            stepper.result->emplace_back(nextState);
        };
/* ======================================================================================
 *  Mirror.emit
 *  Sends copy of the packet to a specified destination.
 *  See also:
 *  https://wiki.ith.intel.com/display/BXDCOMPILER/Externs#Externs-Mirror
 * ====================================================================================== */
// TODO: Currently implemented as no-op. We rely on the fact that when mirroring is not
// configured by the control plane the mirrored packets are discarded and hence not seen
// by the STF/PTF test.
const SharedTofinoExprStepper::ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
    SharedTofinoExprStepper::MIRROR_EMIT =
        [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
            auto &nextState = stepper.state.clone();
            nextState.popBody();
            stepper.result->emplace_back(nextState);
        };

/* ======================================================================================
 *  RegisterAction.execute
 *  See also: https://wiki.ith.intel.com/display/BXDCOMPILER/Stateful+ALU
 * ====================================================================================== */
const SharedTofinoExprStepper::ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
    SharedTofinoExprStepper::REGISTER_ACTION_EXECUTE = [](const ExternInfo &externInfo,
                                                          SharedTofinoExprStepper &stepper) {
        const auto &receiverPath = externInfo.externObjectRef;
        const auto *specType =
            externInfo.externObjectRef.type->checkedTo<IR::Type_SpecializedCanonical>();
        BUG_CHECK(specType->arguments->size() == 3,
                  "Expected 3 type arguments for RegisterAction, received %1%",
                  specType->arguments->size());

        const auto *index = externInfo.externArguments.at(0)->expression;

        const auto *declInstance =
            stepper.state.findDecl(&receiverPath)->checkedTo<IR::Declaration_Instance>();
        const auto *declInstanceArgs = declInstance->arguments;
        BUG_CHECK(declInstanceArgs->size() == 1,
                  "RegisterAction types only have one single instance argument.");
        CHECK_NULL(declInstanceArgs->at(0)->expression);
        const auto &externInstancePath =
            declInstanceArgs->at(0)->expression->checkedTo<IR::PathExpression>();
        const auto &externInstance = stepper.state.findDecl(externInstancePath);

        const auto *initializer = declInstance->initializer;
        const auto *applyFunction = initializer->getDeclByName("apply")->checkedTo<IR::Function>();
        CHECK_NULL(applyFunction);

        const auto *applyParams = applyFunction->getParameters();
        const auto *registerParam = applyParams->getParameter(0);
        const auto *registerParamType = registerParam->type;
        if (const auto *typeName = registerParamType->to<IR::Type_Name>()) {
            registerParamType = stepper.state.resolveType(typeName);
        }
        // Check whether we can configure this register.
        auto canConfigure = TestgenOptions::get().testBackend == "PTF";

        auto &nextState = stepper.state.clone();
        /// Initialize the value of the apply call.
        if (applyParams->size() > 1) {
            const auto *param = applyParams->getParameter(1);
            const auto &returnPath =
                new IR::PathExpression(param->type, new IR::Path(param->getName()));
            nextState.set(returnPath, IR::getDefaultValue(param->type));
        }

        const auto *registerState =
            stepper.state.getTestObject("registervalues"_cs, externInstance->toString(), false);
        const auto *registerParamRef =
            new IR::PathExpression(registerParamType, new IR::Path(registerParam->getName()));

        // Initialize the parameters in the registerAction apply call and create a register
        // object, if necessary.
        const auto *registerExpr = initializeRegisterParameters(
            registerState, nextState, stepper.programInfo, externInstance->toString(),
            registerParamRef, canConfigure, index);

        if (canConfigure) {
            CHECK_NULL(registerExpr);
            const auto *registerValue = new TofinoRegisterValue(
                externInstance->checkedTo<IR::Declaration_Instance>(), registerExpr, index);
            nextState.addTestObject("registervalues"_cs, externInstance->toString(), registerValue);
        }
        std::vector<Continuation::Command> replacements;

        if (applyFunction->body != nullptr) {
            replacements.emplace_back(applyFunction->body);
        }

        // Write back the result of the register to the respective index.
        // TODO: Currently, writing to a register has no effect in Tofino.
        // Only the subsequent packet will be able to access the updated register value.
        // const auto *writeBackStmt =
        //     new IR::MethodCallStatement(Utils::generateInternalMethodCall(
        //         "tofino_register_writeback",
        //         {new IR::StringLiteral(externInstance->toString()), registerParamRef,
        //         index}));
        // replacements.emplace_back(writeBackStmt);

        if (applyParams->size() > 1) {
            const auto *returnParam = applyParams->getParameter(1);
            replacements.emplace_back(Continuation::Return(
                new IR::PathExpression(returnParam->type, new IR::Path(returnParam->getName()))));
        } else {
            const auto *returnType = specType->arguments->at(2);
            // Some registers have no return type.
            if (!returnType->is<IR::Type_Void>()) {
                replacements.emplace_back(Continuation::Return(IR::Constant::get(returnType, 0)));
            }
        }

        // Enter the function's namespace.
        nextState.pushNamespace(applyFunction);

        nextState.replaceTopBody(&replacements);
        stepper.result->emplace_back(nextState);
    };
/* ======================================================================================
 *  DirectRegisterAction.execute
 *  Currently, this function is almost identical to RegisterAction.execute. The only
 * difference is the amount of expected type arguments.
 * ====================================================================================== */
const SharedTofinoExprStepper::ExternMethodImpls<SharedTofinoExprStepper>::MethodImpl
    SharedTofinoExprStepper::DIRECT_REGISTER_ACTION_EXECUTE = [](const ExternInfo &externInfo,
                                                                 SharedTofinoExprStepper &stepper) {
        const auto &receiverPath = externInfo.externObjectRef;
        const auto *specType =
            externInfo.externObjectRef.type->checkedTo<IR::Type_SpecializedCanonical>();
        BUG_CHECK(specType->arguments->size() == 2,
                  "Expected 2 type arguments for DirectRegisterAction, received %1%",
                  specType->arguments->size());

        auto &nextState = stepper.state.clone();
        const auto *declInstance =
            nextState.findDecl(&receiverPath)->checkedTo<IR::Declaration_Instance>();
        const auto *declInstanceArgs = declInstance->arguments;
        BUG_CHECK(declInstanceArgs->size() == 1,
                  "DirectRegisterAction types only have one single instance argument.");
        CHECK_NULL(declInstanceArgs->at(0)->expression);
        const auto &externInstancePath =
            declInstanceArgs->at(0)->expression->checkedTo<IR::PathExpression>();
        const auto &externInstance = stepper.state.findDecl(externInstancePath);

        const auto *progInfo = stepper.getProgramInfo().checkedTo<TofinoSharedProgramInfo>();
        const auto *table = progInfo->getTableofDirectExtern(externInstance);
        const TestObject *tableEntry = nullptr;
        if (table != nullptr) {
            tableEntry =
                stepper.state.getTestObject("tableconfigs"_cs, table->controlPlaneName(), false);
        }
        // Check whether we can configure this register.
        auto canConfigure = TestgenOptions::get().testBackend == "PTF" && tableEntry != nullptr;

        const auto *initializer = declInstance->initializer;
        const auto *applyFunction = initializer->getDeclByName("apply")->checkedTo<IR::Function>();
        CHECK_NULL(applyFunction);

        // Enter the function's namespace.
        nextState.pushNamespace(applyFunction);

        const auto *applyParams = applyFunction->getParameters();
        const auto *registerParam = applyParams->getParameter(0);
        const auto *registerParamType = registerParam->type;
        if (const auto *typeName = registerParamType->to<IR::Type_Name>()) {
            registerParamType = nextState.resolveType(typeName);
        }
        const auto *registerState = stepper.state.getTestObject("direct_registervalues"_cs,
                                                                externInstance->toString(), false);
        const auto *registerParamRef =
            new IR::PathExpression(registerParamType, new IR::Path(registerParam->getName()));

        // Initialize the parameters in the registerAction apply call and create a register
        // object, if necessary.
        const auto *registerExpr = initializeRegisterParameters(
            registerState, nextState, stepper.programInfo, externInstance->toString(),
            registerParamRef, canConfigure, nullptr);

        if (canConfigure) {
            CHECK_NULL(registerExpr);
            const auto *registerValue = new TofinoDirectRegisterValue(
                externInstance->checkedTo<IR::Declaration_Instance>(), registerExpr, table);
            nextState.addTestObject("direct_registervalues"_cs, externInstance->toString(),
                                    registerValue);
        }

        std::vector<Continuation::Command> replacements;

        if (applyFunction->body != nullptr) {
            replacements.emplace_back(applyFunction->body);
        }

        // Write back the result of the register to the respective index.
        // TODO: Currently, writing to a register has no effect in Tofino.
        // Only the subsequent packet will be able to access the updated register value.
        // const auto *writeBackStmt =
        //     new IR::MethodCallStatement(Utils::generateInternalMethodCall(
        //         "tofino_register_writeback",
        //         {new IR::StringLiteral(externInstance->toString()), registerParamRef,
        //         index}));
        // replacements.emplace_back(writeBackStmt);

        if (applyParams->size() > 1) {
            const auto *returnParam = applyParams->getParameter(1);
            replacements.emplace_back(Continuation::Return(
                new IR::PathExpression(returnParam->type, new IR::Path(returnParam->getName()))));
        } else {
            const auto *returnType = specType->arguments->at(1);
            // Some registers have no return type.
            if (!returnType->is<IR::Type_Void>()) {
                replacements.emplace_back(Continuation::Return(IR::Constant::get(returnType, 0)));
            }
        }

        // Enter the function's namespace.
        nextState.pushNamespace(applyFunction);

        nextState.replaceTopBody(&replacements);
        stepper.result->emplace_back(nextState);
    };

// Provides implementations of Tofino externs.
const ExprStepper::ExternMethodImpls<SharedTofinoExprStepper>
    SharedTofinoExprStepper::SHARED_TOFINO_EXTERN_METHOD_IMPLS({
        /* ======================================================================================
         *  *.StartIngress internal method.
         *  The method models the ingress semantics of Tofino's P4 Switch instance.
         * ====================================================================================== */
        {"*.StartIngress"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             const auto *progInfo = stepper.getProgramInfo().to<const TofinoSharedProgramInfo>();
             const auto &pipes = progInfo->ingressCmds();
             for (size_t pipeIdx = 0; pipeIdx < pipes.size(); ++pipeIdx) {
                 const auto &pipeInfo = pipes.at(pipeIdx);
                 auto &pipeEntryState = stepper.state.clone();
                 const auto &portVar = progInfo->getTargetInputPortVar();
                 const auto &pipePortRangeCond =
                     progInfo->getPipePortRangeConstraint(portVar, pipeIdx);
                 pipeEntryState.replaceTopBody(&pipeInfo);
                 stepper.result->emplace_back(pipePortRangeCond, stepper.state, pipeEntryState);
             }
         }},
        /* ======================================================================================
         *  *.StartEgress internal method.
         *  The method models the egress semantics of Tofino's P4 Switch instance.
         * ====================================================================================== */
        {"*.StartEgress"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             const auto *progInfo = stepper.getProgramInfo().to<const TofinoSharedProgramInfo>();
             const auto &pipes = progInfo->egressCmds();
             for (size_t pipeIdx = 0; pipeIdx < pipes.size(); ++pipeIdx) {
                 const auto &pipeInfo = pipes.at(pipeIdx);
                 auto &pipeEntryState = stepper.state.clone();
                 const auto &portVar = progInfo->getTargetOutputPortVar();
                 const auto &pipePortRangeCond =
                     progInfo->getPipePortRangeConstraint(portVar, pipeIdx);
                 pipeEntryState.replaceTopBody(&pipeInfo);
                 stepper.result->emplace_back(pipePortRangeCond, stepper.state, pipeEntryState);
             }
         }},
        /* ======================================================================================
         *  Counter.count and its variants.
         *  We delegate to a separate implementation to save space.
         * ====================================================================================== */
        // TODO: Add mechanism that test frameworks can use to check value correctness.
        // Counter.count currently has no effect in the symbolic interpreter.
        {"Counter.count"_cs, {"index"_cs}, COUNTER_COUNT},
        {"Counter.count"_cs, {"index"_cs, "adjust_byte_count"_cs}, COUNTER_COUNT},
        /* ======================================================================================
         *  DirectCounter.count and its variants.
         *  We delegate to a separate implementation to save space.
         * ====================================================================================== */
        // TODO: Add mechanism that test frameworks can use to check value correctness.
        // DirectCounter.count currently has no effect in the symbolic interpreter.
        {"DirectCounter.count"_cs, {}, COUNTER_COUNT},
        {"DirectCounter.count"_cs, {"adjust_byte_count"_cs}, COUNTER_COUNT},
        /* ======================================================================================
         *  Meter.execute and its variants.
         *  We delegate to a separate implementation to save space.
         * ====================================================================================== */
        // TODO: Add mechanism that test frameworks can use to check value correctness.
        // Meter.execute currently has no effect in the symbolic interpreter.
        {"Meter.execute"_cs, {"index"_cs}, METER_EXECUTE},
        {"Meter.execute"_cs, {"index"_cs, "adjust_byte_count"_cs}, METER_EXECUTE},
        {"Meter.execute"_cs,
         {"index"_cs, "color"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // Return the color the meter was set to.
             nextState.replaceTopBody(
                 Continuation::Return(externInfo.externArguments.at(1)->expression));

             stepper.result->emplace_back(nextState);
         }},
        {"Meter.execute"_cs,
         {"index"_cs, "color"_cs, "adjust_byte_count"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // Return the color the meter was set to.
             nextState.replaceTopBody(
                 Continuation::Return(externInfo.externArguments.at(1)->expression));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  DirectMeter.execute and its variants.
         *  We delegate to a separate implementation to save space.
         * ====================================================================================== */
        // TODO: Add mechanism that test frameworks can use to check value correctness.
        // Meter.execute currently has no effect in the symbolic interpreter.
        {"DirectMeter.execute"_cs, {}, METER_EXECUTE},
        {"DirectMeter.execute"_cs,
         {"color"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // Return the color the meter was set to.
             nextState.replaceTopBody(
                 Continuation::Return(externInfo.externArguments.at(0)->expression));

             stepper.result->emplace_back(nextState);
         }},
        {"DirectMeter.execute"_cs, {"adjust_byte_count"_cs}, METER_EXECUTE},
        {"DirectMeter.execute"_cs,
         {"color"_cs, "adjust_byte_count"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // Return the color the meter was set to.
             nextState.replaceTopBody(
                 Continuation::Return(externInfo.externArguments.at(0)->expression));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  RegisterAction.execute
         *  See also: https://wiki.ith.intel.com/display/BXDCOMPILER/Stateful+ALU
         * ====================================================================================== */
        {"RegisterAction.execute"_cs, {"index"_cs}, REGISTER_ACTION_EXECUTE},
        /* ======================================================================================
         *  DirectRegisterAction.execute
         *  Currently, this function is almost identical to RegisterAction.execute. The only
         * difference is the amount of expected type arguments.
         * ====================================================================================== */
        {"DirectRegisterAction.execute"_cs, {}, DIRECT_REGISTER_ACTION_EXECUTE},
        /* ======================================================================================
         *  RegisterParam.read
         *  Construct a read-only run-time configurable parameter that can only be
         *  used by RegisterAction.
         * ====================================================================================== */
        {"RegisterParam.read"_cs,
         {},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *specType =
                 externInfo.externObjectRef.type->checkedTo<IR::Type_SpecializedCanonical>();
             BUG_CHECK(specType->arguments->size() == 1,
                       "Expected 1 type arguments for RegisterParam, received %1%",
                       specType->arguments->size());
             const auto *returnType = specType->arguments->at(0);

             const auto &receiverPath = externInfo.externObjectRef;
             const auto *externInstance =
                 stepper.state.findDecl(&receiverPath)->checkedTo<IR::Declaration_Instance>();
             auto externName = externInstance->toString();

             const auto *testObject =
                 stepper.state.getTestObject("registerparams"_cs, externName, false);
             const IR::Expression *returnVal = nullptr;
             auto &nextState = stepper.state.clone();
             if (testObject != nullptr) {
                 returnVal = testObject->checkedTo<TofinoRegisterParam>()->getInitialValue();
             } else {
                 returnVal = ToolsVariables::getSymbolicVariable(returnType, externName);
                 auto *registerParam = new TofinoRegisterParam(
                     externInstance->checkedTo<IR::Declaration_Instance>(), returnVal);
                 nextState.addTestObject("registerparams"_cs, externName, registerParam);
             }
             nextState.replaceTopBody(Continuation::Return(returnVal));
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  port_metadata_unpack
         *  See also
         *  https://wiki.ith.intel.com/pages/viewpage.action?pageId=2068055260#PacketPaths-Parser
         *  Like a call to extract, calling port_metadata_unpack advances the "pointer" in the
         *  packet being parsed. port_metadata_unpack always advances the pointer by the size of the
         *  port metadata, regardless of the definition of the struct type. See also, the
         *  implementation of extract in the extern_stepper. A lot of code is lifted from there. The
         *  major difference is that unpack returns a value.
         * ====================================================================================== */
        {"*method.port_metadata_unpack"_cs,
         {"pkt"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             // This argument is the structure being written by the extract.
             const auto *typeArgs = externInfo.originalCall.typeArguments;
             BUG_CHECK(typeArgs->size() == 1,
                       "Must have exactly 1 type argument for port metadata unpack.");

             const auto *extractedType = typeArgs->at(0);
             if (const auto *typeName = extractedType->to<IR::Type_Name>()) {
                 extractedType = stepper.state.resolveType(typeName);
             }
             int unpackSize = 0;
             const auto &arch = TestgenTarget::get().spec.archName;
             if (arch == "tna") {
                 unpackSize = TofinoConstants::PORT_METADATA_SIZE;
             } else if (arch == "t2na") {
                 unpackSize = JBayConstants::PORT_METADATA_SIZE;
             } else {
                 BUG("Unexpected architecture");
             }

             // Calculate the necessary size of the packet to extract successfully.
             auto minSize = std::max(0, stepper.state.getInputPacketCursor() + unpackSize);
             const auto *cond =
                 new IR::Geq(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                             IR::Constant::get(&PacketVars::PACKET_SIZE_VAR_TYPE, minSize));
             // Record the condition we evaluate as string.
             std::stringstream condStream;
             condStream << "PortMetaDataUnpack Condition: ";
             cond->dbprint(condStream);
             {
                 auto &nextState = stepper.state.clone();
                 const auto *unpackExpr = nextState.slicePacketBuffer(unpackSize);

                 BUG_CHECK(extractedType->width_bits() <= unpackSize,
                           "Trying unpack %1% bits. Only %2% are available.",
                           extractedType->width_bits(), unpackSize);

                 if (const auto *structType = extractedType->to<IR::Type_StructLike>()) {
                     auto *structExpr = new IR::ListExpression(structType, {});
                     auto localOffset = 0;
                     // We only support flat assignments, so retrieve all fields from the input
                     // argument.

                     for (const auto *field : structType->fields) {
                         const auto *fieldType = field->type;
                         if (const auto *typeName = fieldType->to<IR::Type_Name>()) {
                             fieldType = nextState.resolveType(typeName);
                         }
                         if (fieldType->is<IR::Type_StructLike>()) {
                             BUG("Unexpected struct field %1% of type %2%", field, fieldType);
                         }
                         auto fieldWidth = fieldType->width_bits();
                         // If the width is zero, just add a zero bit vector.
                         if (fieldWidth == 0) {
                             structExpr->components.push_back(
                                 IR::Constant::get(IR::Type_Bits::get(0), 0));
                             continue;
                         }
                         // Assign a slice of the program packet to the field.
                         auto highIdx = unpackSize - localOffset - 1;
                         auto lowIdx = highIdx + 1 - fieldWidth;
                         IR::Expression *packetSlice = new IR::Slice(unpackExpr, highIdx, lowIdx);
                         packetSlice->type = fieldType;
                         if (const auto *bits = fieldType->to<IR::Type_Bits>()) {
                             if (bits->isSigned) {
                                 auto *intBits = bits->clone();
                                 intBits->isSigned = true;
                                 packetSlice = new IR::Cast(intBits, packetSlice);
                             }
                         } else if (fieldType->is<IR::Type_Boolean>()) {
                             const auto *boolType = IR::Type_Boolean::get();
                             packetSlice = new IR::Cast(boolType, packetSlice);
                         }
                         structExpr->components.push_back(packetSlice);
                         // Bookkeeping, advance the packet temporary packet cursor.
                         localOffset += fieldWidth;
                     }
                     unpackExpr = structExpr;
                 } else if (const auto *bitType = extractedType->to<IR::Type_Bits>()) {
                     // Recast the slice to the correct type.
                     if (bitType->isSigned) {
                         unpackExpr = new IR::Cast(bitType, unpackExpr);
                     }
                 } else {
                     TESTGEN_UNIMPLEMENTED("Unsupported unpack type %1%, node %2%", extractedType,
                                           extractedType->node_type_name());
                 }
                 // Record the condition we are passing at this at this point.
                 nextState.add(*new TraceEvents::Generic(condStream.str()));
                 nextState.replaceTopBody(Continuation::Return(unpackExpr));
                 stepper.result->emplace_back(cond, stepper.state, nextState);
             }
             auto fcsLeft = std::max<int64_t>(0, stepper.state.getProperty<int64_t>("fcsLeft"_cs));
             auto rejectSize =
                 std::max<int64_t>(0, minSize - stepper.state.getPacketBufferSize() - fcsLeft);
             const auto *fcsCond =
                 new IR::Lss(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                             IR::Constant::get(&PacketVars::PACKET_SIZE_VAR_TYPE, rejectSize));
             // {
             //     auto*&fcsState = stepper.state.clone();
             //     fcsState.setProperty("fcsLeft", fcsLeft - extractSize);
             //     // If we are dealing with a header, set the header valid.
             //     if (extractedType->is<IR::Type_Header>()) {
             //         setHeaderValidity(extractOutput, true, fcsState);
             //     }

             //     // We only support flat assignments, so retrieve all fields from the input
             //     // argument.
             //     const auto flatFields = fcsState.getFlatFields(extractOutput, extractedType);
             //     for (const auto* fieldRef : flatFields) {
             //         auto fieldWidth = fcsState.get(fieldRef)->type->width_bits();
             //         // If the width is zero, do not bother with extracting.
             //         if (fieldWidth == 0) {
             //             continue;
             //         }

             //         // Assign a slice of the program packet to the field.
             //         const auto* pktVar = ToolsVariables::getTaintExpression(fieldRef->type);

             //         fcsState.add(*new TraceEvents::Extract(fieldRef, pktVar));
             //         fcsState.set(fieldRef, pktVar);
             //     }
             //     fcsState.add(*new TraceEvents::Extract(Utils::getHeaderValidity(extractOutput),
             //                                           Utils::getHeaderValidity(extractOutput)));
             //     // Record the condition we are passing at this at this point.
             //     fcsState.add(*new TraceEvents::Generic("Extract FCS"));
             //     fcsState.add(*new TraceEvents::Generic(condStream.str()));
             //     fcsState.popBody();
             //     stepper.result->emplace_back(fcsCond, stepper.state, fcsState);
             // }
             if (!(stepper.state.hasProperty("parserErrReferenced"_cs) &&
                   stepper.state.getProperty<bool>("parserErrReferenced"_cs))) {
                 // Handle the case where the packet is too short.
                 // TODO: We only generate packets that are too short if the parser error is not
                 // referenced within the control. This is because of behavior in some targets that
                 // pads packets that are too short. We currently do not model this.
                 // TODO: Remove this check.
                 auto &rejectState = stepper.state.clone();
                 // Record the condition we are failing at this at this point.
                 rejectState.add(
                     *new TraceEvents::Generic("PortMetaDataUnpack: Packet too short"_cs));
                 rejectState.add(*new TraceEvents::Generic(condStream.str()));
                 rejectState.replaceTopBody(Continuation::Exception::PacketTooShort);
                 stepper.result->emplace_back(fcsCond, stepper.state, rejectState);
             }
         }},
        /* ======================================================================================
         *  Random.get
         *  See also:
         * https://wiki.ith.intel.com/pages/viewpage.action?pageId=1981604291#ExternsOld-_9li9ntt34errRandom
         * ====================================================================================== */
        {"Random.get"_cs,
         {},

         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *specType =
                 externInfo.externObjectRef.type->checkedTo<IR::Type_SpecializedCanonical>();
             BUG_CHECK(specType->arguments->size() == 1,
                       "Expected 1 type arguments for Random.get, received %1%",
                       specType->arguments->size());
             const auto *returnType = specType->arguments->at(0);
             auto &nextState = stepper.state.clone();
             BUG_CHECK(returnType->is<IR::Type_Bits>(),
                       "Random.Get: return type is not a Type_Bits: %1%", returnType);
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(returnType, true)));
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Checksum.add
         *  Add data to checksum.
         *  @param data : List of fields to be added to checksum calculation. The
         *  data must be byte aligned.
         * ====================================================================================== */
        {"Checksum.add"_cs,
         {"data"_cs},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Checksum.subtract
         *  Subtract data from existing checksum.
         *  @param data : List of fields to be subtracted from the checksum. The
         *  data must be byte aligned.
         * ====================================================================================== */
        {"Checksum.subtract"_cs,
         {"data"_cs},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Checksum.verify
         *  Verify whether the complemented sum is zero, i.e. the checksum is valid.
         *  @return : Boolean flag indicating whether the checksum is valid or not.
         * ====================================================================================== */
        {"Checksum.verify"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // For now, we return taint
             // We actually should be calculating the checksum here.
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(IR::Type_Boolean::get(), true)));
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Checksum.subtract_all_and_deposit
         *  Subtract all header fields after the current state and
         *  return the calculated checksum value.
         *  Marks the end position for residual checksum header.
         *  All header fields extracted after will be automatically subtracted.
         *  @param residual: The calculated checksum value for added fields.
         * ====================================================================================== */
        {"Checksum.subtract_all_and_deposit"_cs,
         {"residual"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *resultField = externInfo.externArguments.at(0)->expression;
             const auto &fieldRef = ToolsVariables::convertReference(resultField);

             auto &nextState = stepper.state.clone();
             nextState.set(fieldRef,
                           stepper.programInfo.createTargetUninitialized(fieldRef.type, true));
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Checksum.get
         *  Get the calculated checksum value.
         *  @return : The calculated checksum value for added fields.
         * ====================================================================================== */
        {"Checksum.get"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // For now, we return taint
             // We actually should be calculating the checksum here.
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(IR::Type_Bits::get(16), true)));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Checksum.update
         *  We delegate to a separate implementation to save space.
         * ====================================================================================== */
        {"Checksum.update"_cs, {"data"_cs}, CHECKSUM_UPDATE},
        {"Checksum.update"_cs, {"data"_cs, "zeros_as_ones"_cs}, CHECKSUM_UPDATE},
        /* ======================================================================================
         *  Digest.pack
         *  Emit data into the stream.  The p4 program can instantiate multiple
         *  Digest instances in the same deparser control block, and call the pack
         *  method once during a single execution of the control block
         *  See also:
         *  https://wiki.ith.intel.com/display/BXDCOMPILER/Externs#Externs-Digest
         * ====================================================================================== */
        {"Digest.pack"_cs,
         {"data"_cs},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Lpf.execute
         * ====================================================================================== */
        // TODO: What is this?
        {"Lpf.execute"_cs,
         {"val"_cs, "index"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *lpfType = externInfo.externArguments.at(0)->expression->type;

             auto &nextState = stepper.state.clone();
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(lpfType, true)));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  DirectLpf.execute
         * ====================================================================================== */
        // TODO: What is this?
        {"DirectLpf.execute"_cs,
         {"val"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *lpfType = externInfo.externArguments.at(0)->expression->type;

             auto &nextState = stepper.state.clone();
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(lpfType, true)));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Wred.execute
         * ====================================================================================== */
        // TODO: What is this?
        {"Wred.execute"_cs,
         {"val"_cs, "index"_cs},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             const auto *wredType = IR::Type_Bits::get(8);
             auto &nextState = stepper.state.clone();
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(wredType, true)));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  DirectWred.execute
         * ====================================================================================== */
        // TODO: What is this?
        {"DirectWred.execute"_cs,
         {"val"_cs},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             const auto *wredType = IR::Type_Bits::get(8);
             auto &nextState = stepper.state.clone();
             nextState.replaceTopBody(Continuation::Return(
                 stepper.programInfo.createTargetUninitialized(wredType, true)));

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Hash.get
         *  See also:
         *  https://wiki.ith.intel.com/display/BXDCOMPILER/Externs#Externs-Hash
         * ====================================================================================== */
        {"Hash.get"_cs,
         {"data"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *specType =
                 externInfo.externObjectRef.type->checkedTo<IR::Type_SpecializedCanonical>();
             BUG_CHECK(specType->arguments->size() == 1,
                       "Expected 1 type arguments for Hash.get, received %1%",
                       specType->arguments->size());

             const IR::Type_Extern *externType = nullptr;
             if (const auto *type = externInfo.externObjectRef.type->to<IR::Type_Extern>()) {
                 externType = type;
             } else if (const auto *specType =
                            externInfo.externObjectRef.type->to<IR::Type_SpecializedCanonical>()) {
                 CHECK_NULL(specType->substituted);
                 externType = specType->substituted->checkedTo<IR::Type_Extern>();
             }

             const auto *hashInput = externInfo.externArguments.at(0)->expression;
             // If the input arguments is tainted, the entire extern is unreliable.
             auto isTainted = Taint::hasTaint(hashInput);

             const auto *decl = stepper.state.findDecl(&externInfo.externObjectRef);
             const auto *returnType = specType->arguments->at(0);
             const auto *externInstance = decl->checkedTo<IR::Declaration_Instance>();
             const auto *externDeclArgs = externInstance->arguments;
             const auto *hashAlgoExpr = externInfo.externArguments.at(0)->expression;
             IR::IndexedVector<IR::Node> decls({externInstance});
             if (externDeclArgs->size() == 2) {
                 const auto *crcCustomPath =
                     externInfo.externArguments.at(1)->expression->checkedTo<IR::PathExpression>();
                 const auto *crcDecl = stepper.state.findDecl(crcCustomPath);
                 decls.push_back(crcDecl->checkedTo<IR::Declaration_Instance>());
             }

             // In our case, enums have been converted to concrete values.
             auto hashAlgoIdx = hashAlgoExpr->checkedTo<IR::Constant>()->asInt();
             // If the hash algorithm is random (the value 0 in the enum) we have no control. We
             // need to mark the output as tainted.
             isTainted = isTainted || hashAlgoIdx == 1;

             auto externName = externType->name + "_" + externInfo.methodName;
             auto &nextState = stepper.state.clone();
             const IR::Expression *hashReturn = nullptr;
             if (returnType->is<IR::Type_Bits>()) {
                 if (isTainted) {
                     hashReturn = stepper.programInfo.createTargetUninitialized(returnType, true);
                 } else {
                     const auto *concolicVar = new IR::ConcolicVariable(
                         returnType, externName, &externInfo.externArguments,
                         externInfo.originalCall.clone_id, 0, decls);
                     hashReturn = concolicVar;
                 }
             } else {
                 TESTGEN_UNIMPLEMENTED("Hash return of type %2% not supported", returnType);
             }
             nextState.replaceTopBody(Continuation::Return{hashReturn});

             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  Mirror.Emit
         * ====================================================================================== */
        {"Mirror.emit"_cs, {"session_id"_cs}, MIRROR_EMIT},
        {"Mirror.emit"_cs, {"session_id"_cs, "hdr"_cs}, MIRROR_EMIT},
        /* ======================================================================================
         *  sizeInBytes
         *  Return the width of the input in bytes.
         * ====================================================================================== */
        {"*method.sizeInBytes"_cs,
         {"h"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             BUG_CHECK(externInfo.externArguments.size() == 1,
                       "Expected 1 argument for sizeInBytes, received %1%",
                       externInfo.externArguments.size());
             const auto *argumentType = externInfo.externArguments.at(0)->expression->type;
             auto typeWidth = argumentType->width_bits();
             auto typeWidthInBytes = typeWidth / 8;
             auto &nextState = stepper.state.clone();
             nextState.replaceTopBody(
                 Continuation::Return(IR::Constant::get(IR::Type_Bits::get(32), typeWidthInBytes)));
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  sizeInBits
         *  Return the width of the input in bits.
         * ====================================================================================== */
        {"*method.sizeInBits"_cs,
         {"h"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             BUG_CHECK(externInfo.externArguments.size() == 1,
                       "Expected 1 argument for sizeInBits, received %1%",
                       externInfo.externArguments.size());
             const auto *argumentType = externInfo.externArguments.at(0)->expression->type;
             auto typeWidth = argumentType->width_bits();
             auto &nextState = stepper.state.clone();
             nextState.replaceTopBody(
                 Continuation::Return(IR::Constant::get(IR::Type_Bits::get(32), typeWidth)));
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  invalidate
         *  The digest_type field has field validity (see Field Validity) and is initially invalid
         *  at the beginning of ingress processing. If at one point in your ingress P4 code you
         *  assign a value to digest_type, and then later want to undo the decision to generate a
         *  digest, call the invalidate extern function on the digest_type field.
         *  See also:
         *  https://wiki.ith.intel.com/display/BXDCOMPILER/Externs#Externs-Digest
         * ====================================================================================== */
        {"*method.invalidate"_cs,
         {"field"_cs},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  max
         *  Return the maximum of the two values. If either of the values is not a constant, a
         *  constraint will be added that mandates that either one of the values is
         *  greater-or-equals than the other.
         * ====================================================================================== */
        {"*method.max"_cs,
         {"t1"_cs, "t2"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             BUG_CHECK(externInfo.externArguments.size() == 2,
                       "Expected 2 arguments for max, received %1%",
                       externInfo.externArguments.size());

             bool argsAreTainted = false;
             // If any of the input arguments is tainted, the entire extern is unreliable.
             for (const auto *arg : externInfo.externArguments) {
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }

             const auto *t1 = externInfo.externArguments.at(0)->expression;
             const auto *t2 = externInfo.externArguments.at(1)->expression;

             // Return taint when either of the arguments is tainted,
             if (argsAreTainted) {
                 auto &nextState = stepper.state.clone();
                 nextState.replaceTopBody(
                     Continuation::Return(ToolsVariables::getTaintExpression(t1->type)));
                 stepper.result->emplace_back(nextState);
                 return;
             }
             // If both values are constants, just calculate the result.
             if (const auto *t1Const = t1->to<IR::Constant>()) {
                 auto t1Val = t1Const->value;
                 if (const auto *t2Const = t2->to<IR::Constant>()) {
                     auto t2Val = t2Const->value;
                     auto &nextState = stepper.state.clone();
                     auto maxVal = std::max(t1Val, t2Val);
                     nextState.replaceTopBody(
                         Continuation::Return(IR::Constant::get(t1->type, maxVal)));
                     stepper.result->emplace_back(nextState);
                     return;
                 }
             }
             // Alternatively, there are two possibilities. Either t1 or t2 is larger. Add two
             // possible states and the appropriate constraints.
             {
                 auto &t1State = stepper.state.clone();
                 t1State.replaceTopBody(Continuation::Return(t1));
                 stepper.result->emplace_back(new IR::Geq(t1, t2), stepper.state, t1State);
             }
             {
                 auto &t2State = stepper.state.clone();
                 t2State.replaceTopBody(Continuation::Return(t2));
                 stepper.result->emplace_back(new IR::Grt(t2, t1), stepper.state, t2State);
             }
         }},
        /* ======================================================================================
         *  min
         *  Return the minimum of the two values. If either of the values is not a constant, a
         *  constraint will be added that mandates that either one of the values is
         *  lesser-or-equals than the other.
         * ====================================================================================== */
        {"*method.min"_cs,
         {"t1"_cs, "t2"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             BUG_CHECK(externInfo.externArguments.size() == 2,
                       "Expected 2 arguments for min, received %1%",
                       externInfo.externArguments.size());

             bool argsAreTainted = false;
             for (const auto *arg : externInfo.externArguments) {
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }

             const auto *t1 = externInfo.externArguments.at(0)->expression;
             const auto *t2 = externInfo.externArguments.at(1)->expression;

             // Return taint when either of the arguments is tainted,
             if (argsAreTainted) {
                 auto &nextState = stepper.state.clone();
                 nextState.replaceTopBody(
                     Continuation::Return(ToolsVariables::getTaintExpression(t1->type)));
                 stepper.result->emplace_back(nextState);
                 return;
             }
             // If both values are constants, just calculate the result.
             if (const auto *t1Const = t1->to<IR::Constant>()) {
                 auto t1Val = t1Const->value;
                 if (const auto *t2Const = t2->to<IR::Constant>()) {
                     auto t2Val = t2Const->value;
                     auto &nextState = stepper.state.clone();
                     auto minVal = std::min(t1Val, t2Val);
                     nextState.replaceTopBody(
                         Continuation::Return(IR::Constant::get(t1->type, minVal)));
                     stepper.result->emplace_back(nextState);
                     return;
                 }
             }
             // Alternatively, there are two possibilities. Either t1 or t2 is larger. Add two
             // possible states and the appropriate constraints.
             {
                 auto &t1State = stepper.state.clone();
                 t1State.replaceTopBody(Continuation::Return(t1));
                 stepper.result->emplace_back(new IR::Leq(t1, t2), stepper.state, t1State);
             }
             {
                 auto &t2State = stepper.state.clone();
                 t2State.replaceTopBody(Continuation::Return(t2));
                 stepper.result->emplace_back(new IR::Lss(t2, t1), stepper.state, t2State);
             }
         }},
        /* ======================================================================================
         *  funnel_shift_right
         *  Performs a right-shift (@param shift_amount) on the combination of the two arguments
         *  (@param src1 and @param src2) into a single, double-the-length, value and writes it into
         *  @param dst.
         * ====================================================================================== */
        {"*method.funnel_shift_right"_cs,
         {"dst"_cs, "src1"_cs, "src2"_cs, "shift_amount"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             BUG_CHECK(externInfo.externArguments.size() == 4,
                       "Expected 4 arguments for funnel_shift_right, received %1%",
                       externInfo.externArguments.size());
             const auto *dst = externInfo.externArguments.at(0)->expression;
             if (!(dst->is<IR::Member>() || dst->is<IR::PathExpression>())) {
                 TESTGEN_UNIMPLEMENTED(
                     "funnel_shift_right target variable %1% of type %2% not supported", dst,
                     dst->type);
             }

             const auto *src1 = externInfo.externArguments.at(1)->expression;
             const auto *src2 = externInfo.externArguments.at(2)->expression;
             const auto *shiftAmount = externInfo.externArguments.at(3)->expression;
             auto concatWidth = src1->type->width_bits() + src2->type->width_bits();
             const auto *concatType = IR::Type_Bits::get(concatWidth);
             auto &nextState = stepper.state.clone();
             auto *shift =
                 new IR::Shr(concatType, new IR::Concat(concatType, src1, src2), shiftAmount);
             const auto *assign = new IR::AssignmentStatement(dst, new IR::Cast(dst->type, shift));
             nextState.replaceTopBody(assign);
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  check_tofino_drop
         *  This internal extern checks whether the packet should be dropped.
         *  If yes, the packet is dropped and exit is called.
         *  This is done by pushing an exit continuation and setting the drop property to "true".
         * ======================================================================================
         */
        {"*.check_tofino_drop"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             const IR::Expression *egressDropVar = nullptr;
             const IR::Expression *ingressDropVar = nullptr;
             const auto &arch = TestgenTarget::get().spec.archName;
             if (arch == "tna") {
                 ingressDropVar = stepper.state.get(&TofinoConstants::INGRESS_DROP_VAR);
                 egressDropVar = stepper.state.get(&TofinoConstants::EGRESS_DROP_VAR);
             } else if (arch == "t2na") {
                 ingressDropVar = stepper.state.get(&JBayConstants::INGRESS_DROP_VAR);
                 egressDropVar = stepper.state.get(&JBayConstants::EGRESS_DROP_VAR);
             } else {
                 BUG("Unexpected architecture");
             }
             auto &dropState = stepper.state.clone();
             const auto *exitCallStmt = new IR::MethodCallStatement(
                 Utils::generateInternalMethodCall("tofino_drop"_cs, {}));
             dropState.replaceTopBody(exitCallStmt);

             const auto &outputPortVar = stepper.programInfo.getTargetOutputPortVar();
             const auto *outputPort = dropState.get(outputPortVar);
             const auto *outputPortType = outputPort->type;

             // The output port is tainted and has not been set.
             // This means the packet is dropped.
             if (outputPort->is<IR::UninitializedTaintExpression>()) {
                 dropState.set(outputPortVar, stepper.programInfo.createTargetUninitialized(
                                                  outputPortType, false));
                 stepper.result->emplace_back(dropState);
                 return;
             }
             // If either of the drop members is tainted, we also mark the port tainted.
             // We discard and drop tests with a tainted output port.
             if (Taint::hasTaint(egressDropVar) || Taint::hasTaint(ingressDropVar) ||
                 Taint::hasTaint(outputPort)) {
                 dropState.set(outputPortVar,
                               stepper.programInfo.createTargetUninitialized(outputPortType, true));
                 stepper.result->emplace_back(dropState);
                 return;
             }
             // If neither the output port nor the port variables are tainted,
             // check manually whether we need to drop.
             const auto *egressCheck =
                 new IR::Equ(IR::Constant::get(egressDropVar->type, 1), egressDropVar);
             const auto *ingressCheck =
                 new IR::Equ(IR::Constant::get(ingressDropVar->type, 1), ingressDropVar);
             const auto *dropConstant =
                 IR::Constant::get(outputPortType, SharedTofinoConstants::DROP_CONSTANT);
             const auto *dropCond = new IR::LOr(new IR::LOr(egressCheck, ingressCheck),
                                                new IR::Equ(dropConstant, outputPort));
             // For Tofino PTF tests there are additional drop conditions, which we try to model.
             if (TestgenOptions::get().testBackend == "PTF" &&
                 stepper.state.getProperty<uint64_t>("gress"_cs) == gress_t::INGRESS) {
                 const auto *pktSizeVar = ExecutionState::getInputPacketSizeVar();
                 const auto *bitType = pktSizeVar->type;
                 auto *payloadSize =
                     new IR::Sub(bitType, pktSizeVar,
                                 IR::Constant::get(bitType, stepper.state.getInputPacketSize()));
                 // PTF drops packets in ingress below a certain size. Assume it is 512 bits (64
                 // bytes).
                 const auto *pktSize = new IR::Add(
                     bitType, IR::Constant::get(bitType, stepper.state.getPacketBufferSize()),
                     payloadSize);
                 const auto *lossCond = new IR::Lss(
                     pktSize,
                     IR::Constant::get(bitType, SharedTofinoConstants::PTF_OUTPUT_PKT_MIN_SIZE));
                 dropState.add(*new TraceEvents::Expression(
                     pktSize, cstring("Drop packet smaller than ") +
                                  std::to_string(SharedTofinoConstants::PTF_OUTPUT_PKT_MIN_SIZE) +
                                  " for PTF."));
                 dropCond = new IR::LOr(dropCond, lossCond);
             }
             stepper.result->emplace_back(dropCond, stepper.state, dropState);

             // The state if the drop condition is not true.
             auto &passState = stepper.state.clone();
             passState.popBody();
             stepper.result->emplace_back(new IR::LNot(dropCond), stepper.state, passState);
         }},
        /* ======================================================================================
         *  tofino_drop
         *  This internal extern always drops a packet.
         *  This is done by pushing an exit continuation and setting the drop property to "true".
         * ======================================================================================
         */
        {"*.tofino_drop"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedTofinoExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             nextState.add(*new TraceEvents::Generic("Packet marked dropped."_cs));
             nextState.setProperty("drop"_cs, true);
             nextState.replaceTopBody(Continuation::Exception::Drop);
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  tofino_register_writeback
         *  A helper function, which commits @param param as the value of the register at @param
         *  index. @param externInstance is used to look up the correct register test object.
         *  This helper function is necessary because RegisterActions typically execute a series
         *  of statements which may transform the register value.
         * ====================================================================================== */
        {"*.tofino_register_writeback"_cs,
         {"externInstance"_cs, "param"_cs, "index"_cs},
         [](const ExternInfo &externInfo, SharedTofinoExprStepper &stepper) {
             const auto *externInstance =
                 externInfo.externArguments.at(0)->expression->checkedTo<IR::StringLiteral>();
             const auto externParamRef =
                 ToolsVariables::convertReference(externInfo.externArguments.at(1)->expression);
             const auto *externParamType = stepper.state.resolveType(externParamRef->type);
             const auto *externIndex = externInfo.externArguments.at(2)->expression;
             const auto *testObject =
                 stepper.state.getTestObject("registervalues"_cs, externInstance->value, false);
             CHECK_NULL(testObject);

             const IR::Expression *updatedRegisterValue = nullptr;
             if (const auto *structType = externParamType->to<IR::Type_StructLike>()) {
                 IR::IndexedVector<IR::NamedExpression> valueVector{};
                 for (const auto *field : structType->fields) {
                     const auto *fieldType = stepper.state.resolveType(field->type);
                     const auto *fieldRef = new IR::Member(fieldType, externParamRef, field->name);
                     valueVector.push_back(
                         new IR::NamedExpression(field->name.name, stepper.state.get(fieldRef)));
                 }
                 updatedRegisterValue = new IR::StructExpression(nullptr, valueVector);
             } else {
                 updatedRegisterValue = stepper.state.get(externParamRef);
             }
             auto &nextState = stepper.state.clone();
             auto *registerValue =
                 new TofinoRegisterValue(*testObject->checkedTo<TofinoRegisterValue>());
             registerValue->writeToIndex(externIndex, updatedRegisterValue);
             nextState.addTestObject("registervalues"_cs, externInstance->value, registerValue);
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
    });

void SharedTofinoExprStepper::evalExternMethodCall(const ExternInfo &externInfo) {
    auto method = SHARED_TOFINO_EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArguments);
    if (method.has_value()) {
        return method.value()(externInfo, *this);
    }
    // Lastly, check whether we are calling an internal extern method.
    return ExprStepper::evalExternMethodCall(externInfo);
}

bool SharedTofinoExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    TofinoTableStepper tableStepper(this, table);

    return tableStepper.eval();
}

}  // namespace P4::P4Tools::P4Testgen::Tofino
