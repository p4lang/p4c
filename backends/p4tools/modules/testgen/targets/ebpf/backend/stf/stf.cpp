#include "backends/p4tools/modules/testgen/targets/ebpf/backend/stf/stf.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <optional>
#include <regex>  // NOLINT
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>
#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"

namespace P4Tools::P4Testgen::EBPF {

STF::STF(const TestBackendConfiguration &testBackendConfiguration)
    : TestFramework(testBackendConfiguration) {}

inja::json STF::getControlPlane(const TestSpec *testSpec) {
    inja::json controlPlaneJson = inja::json::object();

    // Map of actionProfiles and actionSelectors for easy reference.
    std::map<cstring, cstring> apAsMap;

    auto tables = testSpec->getTestObjectCategory("tables");
    if (!tables.empty()) {
        controlPlaneJson["tables"] = inja::json::array();
    }
    for (const auto &testObject : tables) {
        inja::json tblJson;
        tblJson["table_name"] = testObject.first.c_str();
        const auto *const tblConfig = testObject.second->checkedTo<TableConfig>();
        const auto *tblRules = tblConfig->getRules();
        tblJson["rules"] = inja::json::array();
        for (const auto &tblRule : *tblRules) {
            inja::json rule;
            const auto *matches = tblRule.getMatches();
            const auto *actionCall = tblRule.getActionCall();
            const auto *actionArgs = actionCall->getArgs();
            rule["action_name"] = actionCall->getActionName().c_str();
            auto j = getControlPlaneForTable(*matches, *actionArgs);
            rule["rules"] = std::move(j);
            rule["priority"] = tblRule.getPriority();
            tblJson["rules"].push_back(rule);
        }

        controlPlaneJson["tables"].push_back(tblJson);
    }

    return controlPlaneJson;
}

inja::json STF::getControlPlaneForTable(const TableMatchMap &matches,
                                        const std::vector<ActionArg> &args) {
    inja::json rulesJson;

    rulesJson["matches"] = inja::json::array();
    rulesJson["act_args"] = inja::json::array();
    rulesJson["needs_priority"] = false;

    for (const auto &match : matches) {
        auto fieldName = match.first;
        const auto &fieldMatch = match.second;

        // Replace header stack indices hdr[<index>] with hdr$<index>.
        // TODO: This is a limitation of the stf parser. We should fix this.
        std::regex hdrStackRegex(R"(\[([0-9]+)\])");
        auto indexName = std::regex_replace(fieldName.c_str(), hdrStackRegex, "$$$1");
        if (indexName != fieldName.c_str()) {
            fieldName = cstring::to_cstring(indexName);
        }

        inja::json j;
        j["field_name"] = fieldName;
        if (const auto *elem = fieldMatch->to<Exact>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue());
        } else if (const auto *elem = fieldMatch->to<Ternary>()) {
            const auto *dataValue = elem->getEvaluatedValue();
            const auto *maskField = elem->getEvaluatedMask();
            BUG_CHECK(dataValue->type->width_bits() == maskField->type->width_bits(),
                      "Data value and its mask should have the same bit width.");
            // Using the width from mask - should be same as data
            auto dataStr = formatBinExpr(dataValue, {false, true, false});
            auto maskStr = formatBinExpr(maskField, {false, true, false});
            std::string data = "0b";
            for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
                if (maskStr.at(dataPos) == '0') {
                    data += "*";
                } else {
                    data += dataStr.at(dataPos);
                }
            }
            j["value"] = data;
            // If the rule has a ternary match we need to add the priority.
            rulesJson["needs_priority"] = true;
        } else if (const auto *elem = fieldMatch->to<LPM>()) {
            const auto *dataValue = elem->getEvaluatedValue();
            auto prefixLen = elem->getEvaluatedPrefixLength()->asInt();
            auto fieldWidth = dataValue->type->width_bits();
            auto maxVal = IR::getMaxBvVal(prefixLen);
            const auto *maskField =
                IR::getConstant(dataValue->type, maxVal << (fieldWidth - prefixLen));
            BUG_CHECK(dataValue->type->width_bits() == maskField->type->width_bits(),
                      "Data value and its mask should have the same bit width.");
            // Using the width from mask - should be same as data
            auto dataStr = formatBinExpr(dataValue, {false, true, false});
            auto maskStr = formatBinExpr(maskField, {false, true, false});
            std::string data = "0b";
            for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
                if (maskStr.at(dataPos) == '0') {
                    data += "*";
                } else {
                    data += dataStr.at(dataPos);
                }
            }
            j["value"] = data;
            // If the rule has a ternary match we need to add the priority.
            rulesJson["needs_priority"] = true;
        } else {
            TESTGEN_UNIMPLEMENTED("Unsupported table key match type \"%1%\"",
                                  fieldMatch->getObjectName());
        }
        rulesJson["matches"].push_back(j);
    }

    for (const auto &actArg : args) {
        inja::json j;
        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatHexExpr(actArg.getEvaluatedValue());
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json STF::getSend(const TestSpec *testSpec) {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload, {false, true, false});
    sendJson["pkt"] = dataStr;
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json STF::getVerify(const TestSpec *testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != std::nullopt) {
        const auto &packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto *payload = packet.getEvaluatedPayload();
        const auto *payloadMask = packet.getEvaluatedPayloadMask();
        auto dataStr = formatHexExpr(payload, {false, true, false});
        if (payloadMask != nullptr) {
            // If a mask is present, construct the packet data  with wildcard `*` where there are
            // non zero nibbles
            auto maskStr = formatHexExpr(payloadMask, {false, true, false});
            std::string packetData;
            for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
                if (maskStr.at(dataPos) != 'F') {
                    // TODO: We are being conservative here and adding a wildcard for any 0
                    // in the 4b nibble
                    packetData += "*";
                } else {
                    packetData += dataStr[dataPos];
                }
            }
            verifyData["exp_pkt"] = packetData;
        } else {
            verifyData["exp_pkt"] = dataStr;
        }
    }
    return verifyData;
}

std::string STF::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# p4testgen seed: {{ default(seed, "none") }}
# Date generated: {{timestamp}}
## if length(selected_branches) > 0
    # {{selected_branches}}
## endif
# Current node coverage: {{coverage}}
# Traces
## for trace_item in trace
# {{trace_item}}
##endfor

## if control_plane
## for table in control_plane.tables
# Table {{table.table_name}}
## for rule in table.rules
add {{table.table_name}} {% if rule.rules.needs_priority %}{{rule.priority}} {% endif %}{% for r in rule.rules.matches %}{{r.field_name}}:{{r.value}} {% endfor %}{{rule.action_name}}({% for a in rule.rules.act_args %}{{a.param}}:{{a.value}}{% if not loop.is_last %},{% endif %}{% endfor %})
## endfor
## endfor
## endif

packet {{send.ig_port}} {{send.pkt}}
## if verify
expect {{verify.eg_port}} {{verify.exp_pkt}}
## endif
)""");
    return TEST_CASE;
}

void STF::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                       const std::string &testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }

    auto optSeed = getTestBackendConfiguration().seed;
    if (optSeed.has_value()) {
        dataJson["seed"] = optSeed.value();
    }
    dataJson["test_id"] = testId;
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    LOG5("STF test back end: emitting testcase:" << std::setw(4) << dataJson);
    auto optBasePath = getTestBackendConfiguration().fileBasePath;
    BUG_CHECK(optBasePath.has_value(), "Base path is not set.");
    auto incrementedbasePath = optBasePath.value();
    incrementedbasePath.concat("_" + std::to_string(testId));
    incrementedbasePath.replace_extension(".stf");
    auto stfFileStream = std::ofstream(incrementedbasePath);
    inja::render_to(stfFileStream, testCase, dataJson);
    stfFileStream.flush();
}

void STF::writeTestToFile(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                          float currentCoverage) {
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testId, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::EBPF
