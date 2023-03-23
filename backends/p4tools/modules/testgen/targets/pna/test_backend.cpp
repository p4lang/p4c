#include "backends/p4tools/modules/testgen/targets/pna/test_backend.h"

#include <map>
#include <ostream>
#include <string>
#include <utility>

#include <boost/none.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/pna/backend/metadata/metadata.h"
#include "backends/p4tools/modules/testgen/targets/pna/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

const std::set<std::string> PnaTestBackend::SUPPORTED_BACKENDS = {"METADATA"};

PnaTestBackend::PnaTestBackend(const ProgramInfo &programInfo, SymbolicExecutor &symbex,
                               const boost::filesystem::path &testPath,
                               boost::optional<uint32_t> seed)
    : TestBackEnd(programInfo, symbex) {
    cstring testBackendString = TestgenOptions::get().testBackend;
    if (testBackendString.isNullOrEmpty()) {
        ::error(
            "No test back end provided. Please provide a test back end using the --test-backend "
            "parameter.");
        exit(EXIT_FAILURE);
    }

    if (testBackendString == "METADATA") {
        testWriter = new Metadata(testPath.c_str(), seed);
    } else {
        P4C_UNIMPLEMENTED(
            "Test back end %1% not implemented for this target. Supported back ends are %2%.",
            testBackendString, Utils::containerToString(SUPPORTED_BACKENDS));
    }
}

TestBackEnd::TestInfo PnaTestBackend::produceTestInfo(
    const ExecutionState *executionState, const Model *completedModel,
    const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
    const std::vector<gsl::not_null<const TraceEvent *>> *programTraces) {
    auto testInfo = TestBackEnd::produceTestInfo(executionState, completedModel, outputPacketExpr,
                                                 outputPortExpr, programTraces);
    return testInfo;
}

const TestSpec *PnaTestBackend::createTestSpec(const ExecutionState *executionState,
                                               const Model *completedModel,
                                               const TestInfo &testInfo) {
    // Create a testSpec.
    TestSpec *testSpec = nullptr;

    const auto *ingressPayload = testInfo.inputPacket;
    const auto *ingressPayloadMask = IR::getConstant(IR::getBitType(1), 1);
    const auto ingressPacket = Packet(testInfo.inputPort, ingressPayload, ingressPayloadMask);

    boost::optional<Packet> egressPacket = boost::none;
    if (!testInfo.packetIsDropped) {
        egressPacket = Packet(testInfo.outputPort, testInfo.outputPacket, testInfo.packetTaintMask);
    }
    testSpec = new TestSpec(ingressPacket, egressPacket, testInfo.programTraces);

    // If metadata mode is enabled, gather the user metadata variable form the parser.
    // Save the values of all the fields in it and return.
    if (TestgenOptions::get().testBackend == "METADATA") {
        auto *metadataCollection = new MetadataCollection();
        const auto *pnaProgInfo = programInfo.checkedTo<PnaDpdkProgramInfo>();
        const auto *localMetadataVar = pnaProgInfo->getBlockParam("MainParserT", 2);
        const auto *localMetadataType = executionState->resolveType(localMetadataVar->type);
        auto flatFields = executionState->getFlatFields(
            localMetadataVar, localMetadataType->checkedTo<IR::Type_Struct>(), {});
        for (const auto *fieldRef : flatFields) {
            const auto *fieldVal = completedModel->evaluate(executionState->get(fieldRef));
            metadataCollection->addMetaDataField(fieldRef->toString(), fieldVal);
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
        const auto *const tableConfig = uninterpretedTableConfig->evaluate(*completedModel);
        testSpec->addTestObject("tables", tableName, tableConfig);
    }

    const auto actionProfiles = executionState->getTestObjectCategory("action_profile");
    for (const auto &testObject : actionProfiles) {
        const auto profileName = testObject.first;
        const auto *actionProfile = testObject.second->checkedTo<PnaDpdkActionProfile>();
        const auto *evaluatedProfile = actionProfile->evaluate(*completedModel);
        testSpec->addTestObject("action_profiles", profileName, evaluatedProfile);
    }

    const auto actionSelectors = executionState->getTestObjectCategory("action_selector");
    for (const auto &testObject : actionSelectors) {
        const auto selectorName = testObject.first;
        const auto *actionSelector = testObject.second->checkedTo<PnaDpdkActionSelector>();
        const auto *evaluatedSelector = actionSelector->evaluate(*completedModel);
        testSpec->addTestObject("action_selectors", selectorName, evaluatedSelector);
    }

    return testSpec;
}

}  // namespace P4Tools::P4Testgen::Pna
