#include "backends/p4tools/testgen/targets/ebpf/test_backend.h"

#include <map>
#include <ostream>
#include <string>
#include <utility>

#include <boost/none.hpp>

#include "backends/p4tools/common/lib/ir.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/gmputil.h"

#include "backends/p4tools/testgen/options.h"
#include "backends/p4tools/testgen/targets/ebpf/backend/stf/stf.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

const big_int EBPFTestBackend::ZERO_PKT_VAL = 0x2000000;
const big_int EBPFTestBackend::ZERO_PKT_MAX = 0xffffffff;
const std::vector<std::string> EBPFTestBackend::SUPPORTED_BACKENDS = {"STF"};

EBPFTestBackend::EBPFTestBackend(const ProgramInfo& programInfo, ExplorationStrategy& symbex,
                                 const boost::filesystem::path& testPath,
                                 boost::optional<uint32_t> seed)
    : TestBackEnd(programInfo, symbex) {
    cstring testBackendString = TestgenOptions::get().testBackend;
    if (testBackendString == "STF") {
        testWriter = new STF(testPath.c_str(), seed);
    } else {
        std::stringstream supportedBackendString;
        bool isFirst = true;
        for (const auto& backend : SUPPORTED_BACKENDS) {
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

TestBackEnd::TestInfo EBPFTestBackend::produceTestInfo(
    const ExecutionState* executionState, const Model* completedModel,
    const IR::Expression* outputPacketExpr, const IR::Expression* outputPortExpr,
    const std::vector<gsl::not_null<const TraceEvent*>>* programTraces) {
    auto testInfo = TestBackEnd::produceTestInfo(executionState, completedModel, outputPacketExpr,
                                                 outputPortExpr, programTraces);
    // This is a hack to deal with an virtual kernel interface quirk.
    // Packets that are too small are truncated to 02000000 (in hex) with width 32 bit.
    if (testInfo.outputPacket->type->width_bits() == 0) {
        int outPktSize = ZERO_PKT_WIDTH;
        testInfo.outputPacket =
            IRUtils::getConstant(IRUtils::getBitType(outPktSize), EBPFTestBackend::ZERO_PKT_VAL);
        testInfo.packetTaintMask =
            IRUtils::getConstant(IRUtils::getBitType(outPktSize), EBPFTestBackend::ZERO_PKT_MAX);
    }
    // eBPF actually can not modify the input packet. It can only filter. Thus we reuse our input
    // packet here.
    testInfo.outputPacket = testInfo.inputPacket;
    testInfo.packetTaintMask = IRUtils::getConstant(
        testInfo.inputPacket->type, IRUtils::getMaxBvVal(testInfo.inputPacket->type));
    return testInfo;
}

const TestSpec* EBPFTestBackend::createTestSpec(const ExecutionState* executionState,
                                                const Model* completedModel,
                                                const TestInfo& testInfo) {
    // Create a testSpec.
    TestSpec* testSpec = nullptr;

    const auto* ingressPayload = testInfo.inputPacket;
    const auto* ingressPayloadMask = IRUtils::getConstant(IRUtils::getBitType(1), 1);
    const auto ingressPacket = Packet(testInfo.inputPort, ingressPayload, ingressPayloadMask);

    boost::optional<Packet> egressPacket = boost::none;
    if (!testInfo.packetIsDropped) {
        egressPacket = Packet(testInfo.outputPort, testInfo.outputPacket, testInfo.packetTaintMask);
    }
    testSpec = new TestSpec(ingressPacket, egressPacket, testInfo.programTraces);
    // We retrieve the individual table configurations from the execution state.
    const auto uninterpretedTableConfigs = executionState->getTestObjectCategory("tableconfigs");
    // Since these configurations are uninterpreted we need to convert them. We launch a
    // helper function to solve the variables involved in each table configuration.
    for (const auto& tablePair : uninterpretedTableConfigs) {
        const auto tableName = tablePair.first;
        const auto* uninterpretedTableConfig = tablePair.second->checkedTo<TableConfig>();
        const auto* const tableConfig = uninterpretedTableConfig->evaluate(*completedModel);
        testSpec->addTestObject("tables", tableName, tableConfig);
    }
    return testSpec;
}

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
