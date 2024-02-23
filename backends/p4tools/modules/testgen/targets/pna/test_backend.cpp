#include "backends/p4tools/modules/testgen/targets/pna/test_backend.h"

#include <cstdlib>
#include <list>
#include <string>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/log.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/pna/backend/metadata/metadata.h"
#include "backends/p4tools/modules/testgen/targets/pna/backend/ptf/ptf.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

const std::set<std::string> PnaTestBackend::SUPPORTED_BACKENDS = {"METADATA", "PTF"};

PnaTestBackend::PnaTestBackend(const ProgramInfo &programInfo,
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
    if (testBackendString == "METADATA") {
        testWriter = new Metadata(testBackendConfiguration);
    } else if (testBackendString == "PTF") {
        testWriter = new PTF(testBackendConfiguration);
    } else {
        P4C_UNIMPLEMENTED(
            "Test back end %1% not implemented for this target. Supported back ends are %2%.",
            testBackendString, Utils::containerToString(SUPPORTED_BACKENDS));
    }
}

TestBackEnd::TestInfo PnaTestBackend::produceTestInfo(
    const ExecutionState *executionState, const Model *finalModel,
    const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
    const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces) {
    auto testInfo = TestBackEnd::produceTestInfo(executionState, finalModel, outputPacketExpr,
                                                 outputPortExpr, programTraces);
    return testInfo;
}

const TestSpec *PnaTestBackend::createTestSpec(const ExecutionState *executionState,
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

    // If metadata mode is enabled, gather the user metadata variable from the parser.
    // Save the values of all the fields in it and return.
    if (TestgenOptions::get().testBackend == "METADATA") {
        auto *metadataCollection = new MetadataCollection();
        const auto *pnaProgInfo = getProgramInfo().checkedTo<PnaDpdkProgramInfo>();
        const auto *localMetadataVar = pnaProgInfo->getBlockParam("MainParserT", 2);
        const auto &flatFields = executionState->getFlatFields(localMetadataVar, {});
        for (const auto &fieldRef : flatFields) {
            const auto *fieldVal = finalModel->evaluate(executionState->get(fieldRef), true);
            // Try to remove the leading internal name for the metadata field.
            // Thankfully, this string manipulation is safe if we are out of range.
            auto fieldString = fieldRef->toString();
            fieldString = fieldString.substr(fieldString.find('.') - fieldString.begin() + 1);
            metadataCollection->addMetaDataField(fieldString, fieldVal);
        }
        testSpec->addTestObject("metadata_collection", "metadata_collection", metadataCollection);
        return testSpec;
    }

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

    const auto actionProfiles = executionState->getTestObjectCategory("action_profile");
    for (const auto &testObject : actionProfiles) {
        const auto profileName = testObject.first;
        const auto *actionProfile = testObject.second->checkedTo<PnaDpdkActionProfile>();
        const auto *evaluatedProfile = actionProfile->evaluate(*finalModel, true);
        testSpec->addTestObject("action_profiles", profileName, evaluatedProfile);
    }

    const auto actionSelectors = executionState->getTestObjectCategory("action_selector");
    for (const auto &testObject : actionSelectors) {
        const auto selectorName = testObject.first;
        const auto *actionSelector = testObject.second->checkedTo<PnaDpdkActionSelector>();
        const auto *evaluatedSelector = actionSelector->evaluate(*finalModel, true);
        testSpec->addTestObject("action_selectors", selectorName, evaluatedSelector);
    }

    return testSpec;
}

}  // namespace P4Tools::P4Testgen::Pna
