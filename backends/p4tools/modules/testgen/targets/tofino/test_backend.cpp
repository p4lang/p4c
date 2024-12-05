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

#include "backends/p4tools/modules/testgen/targets/tofino/test_backend.h"

#include <ostream>
#include <string>

#include <boost/none.hpp>

#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_backend/ptf.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_backend/stf.h"

namespace P4::P4Tools::P4Testgen::Tofino {

const std::vector<std::string> TofinoTestBackend::SUPPORTED_BACKENDS = {"PTF", "STF"};

TofinoTestBackend::TofinoTestBackend(const TofinoSharedProgramInfo &programInfo,
                                     const TestBackendConfiguration &testBackendConfiguration,
                                     SymbolicExecutor &symbex)
    : TestBackEnd(programInfo, testBackendConfiguration, symbex) {
    cstring testBackendString = TestgenOptions::get().testBackend;

    if (testBackendString.isNullOrEmpty()) {
        ::error(
            "No test back end provided. Please provide a test back end using the --test-backend "
            "parameter.");
        exit(EXIT_FAILURE);
    }
    if (testBackendString == "PTF") {
        testWriter = new PTF(testBackendConfiguration);
    } else if (testBackendString == "STF") {
        testWriter = new STF(testBackendConfiguration);
    } else {
        std::stringstream supportedBackendString;
        bool isFirst = true;
        for (const auto &backend : SUPPORTED_BACKENDS) {
            if (!isFirst) {
                supportedBackendString << ", ";
            } else {
                isFirst = false;
            }
            supportedBackendString << backend;
        }
        P4C_UNIMPLEMENTED(
            "Test back end %1% not implemented for this target. Supported back ends are %2%.",
            testBackendString, supportedBackendString.str());
    }
}

bool TofinoTestBackend::printTestInfo(const ExecutionState *executionState,
                                      const TestBackEnd::TestInfo &testInfo,
                                      const IR::Expression *outputPortExpr) {
    // If the port is uninitalized it is not actually tainted. It is dropped instead.
    if (outputPortExpr->is<IR::UninitializedTaintExpression>()) {
        outputPortExpr = IR::Constant::get(outputPortExpr->type, 0);
        BUG_CHECK(testInfo.packetIsDropped, "Uninitialized output port that is not dropped.");
    }
    return TestBackEnd::printTestInfo(executionState, testInfo, outputPortExpr);
}

TestBackEnd::TestInfo TofinoTestBackend::produceTestInfo(
    const ExecutionState *executionState, const Model *finalModel,
    const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
    const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces) {
    auto testInfo = TestBackEnd::produceTestInfo(executionState, finalModel, outputPacketExpr,
                                                 outputPortExpr, programTraces);
    if (outputPortExpr->is<IR::UninitializedTaintExpression>()) {
        testInfo.packetIsDropped = true;
    }
    return testInfo;
}

const TestSpec *TofinoTestBackend::createTestSpec(const ExecutionState *executionState,
                                                  const Model *finalModel,
                                                  const TestInfo &testInfo) {
    // Create a testSpec.
    TestSpec *testSpec = nullptr;

    const auto *ingressPayload = testInfo.inputPacket;
    const auto *ingressPayloadMask = IR::Constant::get(IR::Type_Bits::get(1), 1);
    const auto ingressPacket = Packet(testInfo.inputPort, ingressPayload, ingressPayloadMask);

    std::optional<Packet> egressPacket = std::nullopt;
    if (!testInfo.packetIsDropped) {
        egressPacket = Packet(testInfo.outputPort, testInfo.outputPacket, testInfo.packetTaintMask);
    }
    testSpec = new TestSpec(ingressPacket, egressPacket, testInfo.programTraces);
    // We retrieve the individual table configurations from the execution state.
    const auto uninterpretedTableConfigs = executionState->getTestObjectCategory("tableconfigs"_cs);
    // Since these configurations are uninterpreted we need to convert them. We launch a
    // helper function to solve the variables involved in each table configuration.
    for (const auto &tablePair : uninterpretedTableConfigs) {
        const auto tableName = tablePair.first;
        const auto *uninterpretedTableConfig = tablePair.second->checkedTo<TableConfig>();
        const auto *const tableConfig = uninterpretedTableConfig->evaluate(*finalModel, true);
        testSpec->addTestObject("tables"_cs, tableName, tableConfig);
    }
    // TODO: Move this to target specific test specification.
    const auto actionProfiles = executionState->getTestObjectCategory("action_profile"_cs);
    for (const auto &testObject : actionProfiles) {
        const auto profileName = testObject.first;
        const auto *actionProfile = testObject.second->checkedTo<TofinoActionProfile>();
        const auto *evaluatedProfile = actionProfile->evaluate(*finalModel, true);
        testSpec->addTestObject("action_profiles"_cs, profileName, evaluatedProfile);
    }

    // TODO: Move this to target specific test specification.
    const auto actionSelectors = executionState->getTestObjectCategory("action_selector"_cs);
    for (const auto &testObject : actionSelectors) {
        const auto selectorName = testObject.first;
        const auto *actionSelector = testObject.second->checkedTo<TofinoActionSelector>();
        const auto *evaluatedSelector = actionSelector->evaluate(*finalModel, true);
        testSpec->addTestObject("action_selectors"_cs, selectorName, evaluatedSelector);
    }

    const auto registerParams = executionState->getTestObjectCategory("registerparams"_cs);
    for (const auto &[name, obj] : registerParams) {
        const auto *reg = obj->checkedTo<TofinoRegisterParam>();
        const auto *evaluatedReg = reg->evaluate(*finalModel, true);
        testSpec->addTestObject("registerparams"_cs, name, evaluatedReg);
    }

    const auto registers = executionState->getTestObjectCategory("registervalues"_cs);
    for (const auto &[name, obj] : registers) {
        const auto *reg = obj->checkedTo<TofinoRegisterValue>();
        const auto *evaluatedReg = reg->evaluate(*finalModel, true);
        testSpec->addTestObject("registervalues"_cs, name, evaluatedReg);
    }

    const auto directRegisters = executionState->getTestObjectCategory("direct_registervalues"_cs);
    for (const auto &[name, obj] : directRegisters) {
        const auto *reg = obj->checkedTo<TofinoDirectRegisterValue>();
        const auto *evaluatedReg = reg->evaluate(*finalModel, true);
        testSpec->addTestObject("direct_registervalues"_cs, name, evaluatedReg);
    }

    return testSpec;
}
}  // namespace P4::P4Tools::P4Testgen::Tofino
