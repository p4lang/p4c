#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/metadata.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

Metadata::Metadata(const TestBackendConfiguration &testBackendConfiguration)
    : Bmv2TestFramework(testBackendConfiguration) {}

std::string Metadata::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# A P4Testgen-generated test case for {{test_name}}.p4

# Seed used to generate this test.
seed: {{ default(seed, "none") }}
# Test timestamp.
date: {{timestamp}}
# Percentage of nodes covered at the time of this test.
node_coverage: {{coverage}}

# Trace associated with this test.
#
## for trace_item in trace
# {{trace_item}}
## endfor

# The input packet in hexadecimal.
input_packet: "{{send.pkt}}"

# Parsed headers and their offsets.
header_offsets:
## for offset in offsets
  {{offset.label}}: {{offset.offset}}
## endfor

# The in-order list of parser states which were traversed for this test.
parser_states: {% for s in parser_states %}{{s}}{% if not loop.is_last %}, {% endif %}{% endfor %}

# Metadata results after this test has completed.
metadata:
## for metadata_field in metadata_fields
  {{metadata_field.name}}: [value: "{{metadata_field.value}}", offset: {{metadata_field.offset}}]
## endfor
)""");
    return TEST_CASE;
}

void Metadata::computeTraceData(const TestSpec *testSpec, inja::json &dataJson) {
    dataJson["trace"] = inja::json::array();
    dataJson["offsets"] = inja::json::array();
    dataJson["parser_states"] = inja::json::array();
    const auto *traces = testSpec->getTraces();
    if (traces != nullptr) {
        for (const auto &trace : *traces) {
            if (const auto *successfulExtract = trace.get().to<TraceEvents::ExtractSuccess>()) {
                inja::json j;
                j["label"] = successfulExtract->getExtractedHeader()->toString();
                j["offset"] = successfulExtract->getOffset();
                dataJson["offsets"].push_back(j);
            }
            if (const auto *parserState = trace.get().to<TraceEvents::ParserState>()) {
                dataJson["parser_states"].push_back(parserState->getParserState()->getName().name);
            }
            std::stringstream ss;
            ss << trace;
            dataJson["trace"].push_back(ss.str());
        }
    }
}

void Metadata::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                            const std::string &testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }
    auto optSeed = getTestBackendConfiguration().seed;
    if (optSeed.has_value()) {
        dataJson["seed"] = optSeed.value();
    }
    dataJson["test_name"] = getTestBackendConfiguration().testBaseName;
    dataJson["test_id"] = testId;
    computeTraceData(testSpec, dataJson);

    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getExpectedPacket(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();
    dataJson["metadata_fields"] = inja::json::array();
    const auto *metadataCollection =
        testSpec->getTestObject("metadata_collection", "metadata_collection", true)
            ->checkedTo<MetadataCollection>();
    auto offset = 0;
    for (auto const &metadataField : metadataCollection->getMetadataFields()) {
        inja::json jsonField;
        jsonField["name"] = metadataField.first;
        jsonField["value"] = formatHexExpr(metadataField.second);
        // Keep track of the overall offset of the metadata field.
        jsonField["offset"] = offset;
        offset += metadataField.second->type->width_bits();
        dataJson["metadata_fields"].push_back(jsonField);
    }

    LOG5("Metadata back end: emitting testcase:" << std::setw(4) << dataJson);

    auto optBasePath = getTestBackendConfiguration().fileBasePath;
    BUG_CHECK(optBasePath.has_value(), "Base path is not set.");
    auto incrementedbasePath = optBasePath.value();
    incrementedbasePath.concat("_" + std::to_string(testId));
    incrementedbasePath.replace_extension(".yml");
    metadataFile = std::ofstream(incrementedbasePath);
    inja::render_to(metadataFile, testCase, dataJson);
    metadataFile.flush();
}

void Metadata::writeTestToFile(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                               float currentCoverage) {
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testId, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Bmv2
