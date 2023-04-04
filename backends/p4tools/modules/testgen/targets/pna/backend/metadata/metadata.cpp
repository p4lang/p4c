#include "backends/p4tools/modules/testgen/targets/pna/backend/metadata/metadata.h"

#include <algorithm>
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

#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

Metadata::Metadata(cstring testName, std::optional<unsigned int> seed = std::nullopt)
    : TF(testName, seed) {}

std::vector<std::pair<size_t, size_t>> Metadata::getIgnoreMasks(const IR::Constant *mask) {
    std::vector<std::pair<size_t, size_t>> ignoreMasks;
    if (mask == nullptr) {
        return ignoreMasks;
    }
    auto maskBinStr = formatBinExpr(mask, false, true, false);
    int countZeroes = 0;
    size_t offset = 0;
    for (; offset < maskBinStr.size(); ++offset) {
        if (maskBinStr.at(offset) == '0') {
            countZeroes++;
        } else {
            if (countZeroes > 0) {
                ignoreMasks.emplace_back(offset - countZeroes, countZeroes);
                countZeroes = 0;
            }
        }
    }
    if (countZeroes > 0) {
        ignoreMasks.emplace_back(offset - countZeroes, countZeroes);
    }
    return ignoreMasks;
}

inja::json Metadata::getSend(const TestSpec *testSpec) {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload);
    sendJson["pkt"] = dataStr;
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json Metadata::getVerify(const TestSpec *testSpec) {
    inja::json verifyData = inja::json::object();
    auto egressPacket = testSpec->getEgressPacket();
    if (egressPacket.has_value()) {
        const auto *const packet = egressPacket.value();
        verifyData["eg_port"] = packet->getPort();
        const auto *payload = packet->getEvaluatedPayload();
        const auto *payloadMask = packet->getEvaluatedPayloadMask();
        verifyData["ignore_mask"] = formatHexExpr(payloadMask);
        verifyData["exp_pkt"] = formatHexExpr(payload);
    }
    return verifyData;
}

std::string Metadata::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# A P4TestGen-generated test case for {{test_name}}.p4
# p4testgen seed: {{ default(seed, "none") }}
# Date generated: {{timestamp}}
## if length(selected_branches) > 0
# {{selected_branches}}
## endif
# Current statement coverage: {{coverage}}

## for trace_item in trace
# {{trace_item}}
## endfor

input_packet: {{send.pkt}}
input_port: {{send.ig_port}}

## if verify
output_packet: "{{verify.exp_pkt}}"
output_port: {{verify.eg_port}}
output_packet_mask: "{{verify.ignore_mask}}"
## endif

Offsets: {% for o in offsets %}{{o.label}}@{{o.offset}}{% if not loop.is_last %},{% endif %}{% endfor %}

## for metadata_field in metadata_fields
Metadata: {{metadata_field.name}}@{{metadata_field.value}}
## endfor

)""");
    return TEST_CASE;
}

void Metadata::computeTraceData(const TestSpec *testSpec, inja::json &dataJson) {
    dataJson["trace"] = inja::json::array();
    dataJson["offsets"] = inja::json::array();
    const auto *traces = testSpec->getTraces();
    if (traces != nullptr) {
        for (const auto &trace : *traces) {
            if (const auto *successfulExtract = trace.get().to<TraceEvents::ExtractSuccess>()) {
                inja::json j;
                j["label"] = successfulExtract->getExtractedHeader()->toString();
                j["offset"] = successfulExtract->getOffset();
                dataJson["offsets"].push_back(j);
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
    if (seed) {
        dataJson["seed"] = *seed;
    }
    dataJson["test_name"] = testName;
    dataJson["test_id"] = testId + 1;
    computeTraceData(testSpec, dataJson);

    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();
    dataJson["metadata_fields"] = inja::json::array();
    const auto *metadataCollection =
        testSpec->getTestObject("metadata_collection", "metadata_collection", true)
            ->checkedTo<MetadataCollection>();
    for (auto const &metadataField : metadataCollection->getMetadataFields()) {
        inja::json jsonField;
        jsonField["name"] = metadataField.first;
        jsonField["value"] = formatHexExpr(metadataField.second);
        dataJson["metadata_fields"].push_back(jsonField);
    }

    LOG5("Metadata back end: emitting testcase:" << std::setw(4) << dataJson);

    inja::render_to(metadataFile, testCase, dataJson);
    metadataFile.flush();
}

void Metadata::outputTest(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                          float currentCoverage) {
    auto incrementedTestName = testName + "_" + std::to_string(testIdx);

    metadataFile = std::ofstream(incrementedTestName + ".metadata");
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Pna
