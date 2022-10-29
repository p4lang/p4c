#include "backends/p4tools/testgen/targets/ebpf/backend/stf/stf.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <regex>  // NOLINT
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/none.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/testgen/lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

STF::STF(cstring testName, boost::optional<unsigned int> seed = boost::none) : TF(testName, seed) {
    boost::filesystem::path testFile(testName + ".stf");
    cstring testNameOnly(testFile.stem().c_str());
}

inja::json STF::getTrace(const TestSpec* testSpec) {
    inja::json traceList = inja::json::array();
    const auto* traces = testSpec->getTraces();
    if ((traces != nullptr) && !traces->empty()) {
        for (const auto& trace : *traces) {
            std::stringstream ss;
            ss << *trace;
            traceList.push_back(ss.str());
        }
    }
    return traceList;
}

inja::json STF::getControlPlane(const TestSpec* testSpec) {
    inja::json controlPlaneJson = inja::json::object();

    auto tables = testSpec->getTestObjectCategory("tables");
    if (!tables.empty()) {
        controlPlaneJson["tables"] = inja::json::array();
    }
    for (auto const& testObject : tables) {
        inja::json tblJson;
        tblJson["table_name"] = testObject.first.c_str();
        const auto* const tblConfig = testObject.second->checkedTo<TableConfig>();
        auto const* tblRules = tblConfig->getRules();
        tblJson["rules"] = inja::json::array();
        for (const auto& tblRule : *tblRules) {
            inja::json rule;
            auto const* matches = tblRule.getMatches();
            auto const* actionCall = tblRule.getActionCall();
            auto const* actionArgs = actionCall->getArgs();
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

inja::json STF::getControlPlaneForTable(const std::map<cstring, const FieldMatch>& matches,
                                        const std::vector<ActionArg>& args) {
    inja::json rulesJson;

    rulesJson["matches"] = inja::json::array();
    rulesJson["act_args"] = inja::json::array();
    rulesJson["needs_priority"] = false;

    for (auto const& match : matches) {
        auto fieldName = match.first;
        auto const& fieldMatch = match.second;

        // Replace header stack indices hdr[<index>] with hdr$<index>.
        // TODO: This is a limitation of the stf parser. We should fix this.
        std::regex hdrStackRegex(R"(\[([0-9]+)\])");
        auto indexName = std::regex_replace(fieldName.c_str(), hdrStackRegex, "$$$1");
        if (indexName != fieldName.c_str()) {
            fieldName = cstring::to_cstring(indexName);
        }

        // Iterate over the match fields and segregate them.
        struct GetRange : public boost::static_visitor<void> {
            cstring fieldName;
            inja::json& rulesJson;

            GetRange(inja::json& rulesJson, cstring fieldName)
                : fieldName(fieldName), rulesJson(rulesJson) {}

            void operator()(const Exact& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExpr(elem.getEvaluatedValue());
                rulesJson["matches"].push_back(j);
            }
            void operator()(const Range& /*elem*/) const {
                TESTGEN_UNIMPLEMENTED("Match type range not implemented.");
            }
            void operator()(const Ternary& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                const auto* dataValue = elem.getEvaluatedValue();
                const auto* maskField = elem.getEvaluatedMask();
                BUG_CHECK(dataValue->type->width_bits() == maskField->type->width_bits(),
                          "Data value and its mask should have the same bit width.");
                // Using the width from mask - should be same as data
                auto dataStr = formatBinExpr(dataValue, false, true, false);
                auto maskStr = formatBinExpr(maskField, false, true, false);
                std::string data = "0b";
                for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
                    if (maskStr.at(dataPos) == '0') {
                        data += "*";
                    } else {
                        data += dataStr.at(dataPos);
                    }
                }
                j["value"] = data;
                rulesJson["matches"].push_back(j);
                // If the rule has a ternary match we need to add the priority.
                rulesJson["needs_priority"] = true;
            }
            void operator()(const LPM& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                const auto* dataValue = elem.getEvaluatedValue();
                auto prefixLen = elem.getEvaluatedPrefixLength()->asInt();
                auto fieldWidth = dataValue->type->width_bits();
                auto maxVal = IRUtils::getMaxBvVal(prefixLen);
                const auto* maskField =
                    IRUtils::getConstant(dataValue->type, maxVal << (fieldWidth - prefixLen));
                BUG_CHECK(dataValue->type->width_bits() == maskField->type->width_bits(),
                          "Data value and its mask should have the same bit width.");
                // Using the width from mask - should be same as data
                auto dataStr = formatBinExpr(dataValue, false, true, false);
                auto maskStr = formatBinExpr(maskField, false, true, false);
                std::string data = "0b";
                for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
                    if (maskStr.at(dataPos) == '0') {
                        data += "*";
                    } else {
                        data += dataStr.at(dataPos);
                    }
                }
                j["value"] = data;
                rulesJson["matches"].push_back(j);
                // If the rule has a ternary match we need to add the priority.
                rulesJson["needs_priority"] = true;
            }
        };
        boost::apply_visitor(GetRange(rulesJson, fieldName), fieldMatch);
    }

    for (const auto& actArg : args) {
        inja::json j;
        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatHexExpr(actArg.getEvaluatedValue());
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json STF::getSend(const TestSpec* testSpec) {
    const auto* iPacket = testSpec->getIngressPacket();
    const auto* payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload, false, true, false);
    sendJson["pkt"] = dataStr;
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json STF::getVerify(const TestSpec* testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != boost::none) {
        const auto& packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto* payload = packet.getEvaluatedPayload();
        const auto* payloadMask = packet.getEvaluatedPayloadMask();
        auto dataStr = formatHexExpr(payload, false, true, false);
        if (payloadMask != nullptr) {
            // If a mask is present, construct the packet data  with wildcard `*` where there are
            // non zero nibbles
            auto maskStr = formatHexExpr(payloadMask, false, true, false);
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

static std::string getTestCase() {
    static std::string TEST_CASE(
        R"""(# p4testgen seed: '{{ default(seed, '"none"') }}'
# Date generated: {{timestamp}}
## if length(selected_branches) > 0
    # {{selected_branches}}
## endif
# Current statement coverage: {{coverage}}
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

void STF::emitTestcase(const TestSpec* testSpec, cstring selectedBranches, size_t testId,
                       const std::string& testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }
    if (seed) {
        dataJson["seed"] = *seed;
    }

    dataJson["test_id"] = testId + 1;
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = TestgenUtils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    LOG5("STF test back end: emitting testcase:" << std::setw(4) << dataJson);

    inja::render_to(stfFile, testCase, dataJson);
    stfFile.flush();
}

void STF::outputTest(const TestSpec* testSpec, cstring selectedBranches, size_t testIdx,
                     float currentCoverage) {
    auto incrementedTestName = testName + "_" + std::to_string(testIdx);

    stfFile = std::ofstream(incrementedTestName + ".stf");
    std::string testCase = getTestCase();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
