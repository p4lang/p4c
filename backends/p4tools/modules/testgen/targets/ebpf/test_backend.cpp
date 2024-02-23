#include "backends/p4tools/modules/testgen/targets/ebpf/test_backend.h"

#include <list>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/backend/stf/stf.h"

namespace P4Tools::P4Testgen::EBPF {

const big_int EBPFTestBackend::ZERO_PKT_VAL = 0x2000000;
const big_int EBPFTestBackend::ZERO_PKT_MAX = 0xffffffff;
const std::vector<std::string> EBPFTestBackend::SUPPORTED_BACKENDS = {"STF"};

EBPFTestBackend::EBPFTestBackend(const ProgramInfo &programInfo,
                                 const TestBackendConfiguration &testBackendConfiguration,
                                 SymbolicExecutor &symbex)
    : TestBackEnd(programInfo, testBackendConfiguration, symbex) {
    cstring testBackendString = TestgenOptions::get().testBackend;
    if (testBackendString.isNullOrEmpty()) {
        ::error(
            "No test back end provided. Please provide a test back end using the --test-backend "
            "parameter. Supported back ends are %1%.",
            Utils::containerToString(SUPPORTED_BACKENDS));
        exit(EXIT_FAILURE);
    }

    if (testBackendString == "STF") {
        testWriter = new STF(testBackendConfiguration);
    } else {
        P4C_UNIMPLEMENTED(
            "Test back end %1% not implemented for this target. Supported back ends are %2%.",
            testBackendString, Utils::containerToString(SUPPORTED_BACKENDS));
    }
}

TestBackEnd::TestInfo EBPFTestBackend::produceTestInfo(
    const ExecutionState *executionState, const Model *finalModel,
    const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
    const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces) {
    auto testInfo = TestBackEnd::produceTestInfo(executionState, finalModel, outputPacketExpr,
                                                 outputPortExpr, programTraces);
    // This is a hack to deal with an virtual kernel interface quirk.
    // Packets that are too small are truncated to 02000000 (in hex) with width 32 bit.
    if (testInfo.outputPacket->type->width_bits() == 0) {
        int outPktSize = ZERO_PKT_WIDTH;
        testInfo.outputPacket =
            IR::getConstant(IR::getBitType(outPktSize), EBPFTestBackend::ZERO_PKT_VAL);
        testInfo.packetTaintMask =
            IR::getConstant(IR::getBitType(outPktSize), EBPFTestBackend::ZERO_PKT_MAX);
    } else {
        // eBPF actually can not modify the input packet. It can only filter. Thus we reuse our
        // input packet here.
        testInfo.outputPacket = testInfo.inputPacket;
        testInfo.packetTaintMask = IR::getConstant(testInfo.inputPacket->type,
                                                   IR::getMaxBvVal(testInfo.inputPacket->type));
    }
    return testInfo;
}

const TestSpec *EBPFTestBackend::createTestSpec(const ExecutionState *executionState,
                                                const Model *finalModel, const TestInfo &testInfo) {
    // Create a testSpec.
    TestSpec *testSpec = nullptr;

    const auto *ingressPayload = testInfo.inputPacket;
    const auto *ingressPayloadMask = IR::getConstant(IR::getBitType(1), 1);
    const auto ingressPacket = Packet(testInfo.inputPort, ingressPayload, ingressPayloadMask);

    std::optional<Packet> egressPacket = std::nullopt;
    if (!testInfo.packetIsDropped) {
        egressPacket = Packet(testInfo.outputPort, testInfo.outputPacket, testInfo.packetTaintMask);
    }
    testSpec = new TestSpec(ingressPacket, egressPacket, testInfo.programTraces);
    // We retrieve the individual table configurations from the execution state.
    const auto uninterpretedTableConfigs = executionState->getTestObjectCategory("tableconfigs");
    // Since these configurations are uninterpreted we need to convert them. We launch a
    // helper function to solve the variables involved in each table configuration.
    for (const auto &tablePair : uninterpretedTableConfigs) {
        const auto tableName = tablePair.first;
        const auto *uninterpretedTableConfig = tablePair.second->checkedTo<TableConfig>();
        const auto *const tableConfig = uninterpretedTableConfig->evaluate(*finalModel, true);
        testSpec->addTestObject("tables", tableName, tableConfig);
    }
    return testSpec;
}

}  // namespace P4Tools::P4Testgen::EBPF
