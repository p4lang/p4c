#include "backends/p4tools/modules/testgen/targets/bmv2/backend/stf/stf.h"

#include <iomanip>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/core/enable_if.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>
#include <boost/none.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "gsl/gsl-lite.hpp"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

STF::STF(cstring testName, boost::optional<unsigned int> seed = boost::none) : TF(testName, seed) {}

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

        // Collect action profiles and selectors associated with the table.
        checkForTableActionProfile<Bmv2_V1ModelActionProfile, Bmv2_V1ModelActionSelector>(
            tblJson, apAsMap, tblConfig);

        // Check whether the default action is overridden for this table.
        checkForDefaultActionOverride(tblJson, tblConfig);

        controlPlaneJson["tables"].push_back(tblJson);
    }

    // Collect declarations of action profiles.
    collectActionProfileDeclarations<Bmv2_V1ModelActionProfile>(testSpec, controlPlaneJson,
                                                                apAsMap);

    return controlPlaneJson;
}

inja::json STF::getControlPlaneForTable(const TableMatchMap &matches,
                                        const std::vector<ActionArg> &args) {
    inja::json rulesJson;

    rulesJson["matches"] = inja::json::array();
    rulesJson["act_args"] = inja::json::array();
    rulesJson["needs_priority"] = false;

    // Iterate over the match fields and segregate them.
    for (const auto &match : matches) {
        const auto fieldName = match.first;
        const auto &fieldMatch = match.second;

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
            // If the rule has a ternary match we need to add the priority.
            rulesJson["needs_priority"] = true;
        } else if (const auto *elem = fieldMatch->to<Optional>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue()).c_str();
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
    auto dataStr = formatHexExpr(payload, false, true, false);
    sendJson["pkt"] = dataStr;
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json STF::getVerify(const TestSpec *testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != boost::none) {
        const auto &packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto *payload = packet.getEvaluatedPayload();
        const auto *payloadMask = packet.getEvaluatedPayloadMask();
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

inja::json::array_t STF::getClone(const std::map<cstring, const TestObject *> &cloneInfos) {
    auto cloneJson = inja::json::array_t();
    for (auto cloneInfoTuple : cloneInfos) {
        inja::json cloneInfoJson;
        const auto *cloneInfo = cloneInfoTuple.second->checkedTo<Bmv2_CloneInfo>();
        cloneInfoJson["session_id"] = cloneInfo->getEvaluatedSessionId()->asUint64();
        cloneInfoJson["clone_port"] = cloneInfo->getEvaluatedClonePort()->asInt();
        cloneInfoJson["cloned"] = cloneInfo->isClonedPacket();
        cloneJson.push_back(cloneInfoJson);
    }
    return cloneJson;
}

std::string STF::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# p4testgen seed: {{ default(seed, "none") }}
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
## if existsIn(table, "default_override")
setdefault {{table.table_name}} {{table.default_override.action_name}}({% for a in table.default_override.act_args %}{{a.param}}:{{a.value}}{% if not loop.is_last %},{% endif %}{% endfor %})
## else
## for rule in table.rules
add {{table.table_name}} {% if rule.rules.needs_priority %}{{rule.priority}} {% endif %}{% for r in rule.rules.matches %}{{r.field_name}}:{{r.value}} {% endfor %}{{rule.action_name}}({% for a in rule.rules.act_args %}{{a.param}}:{{a.value}}{% if not loop.is_last %},{% endif %}{% endfor %})
## endfor
## endif

## endfor
## endif

## if exists("clone_infos")
## for clone_info in clone_infos
mirroring_add {{clone_info.session_id}} {{clone_info.clone_port}}
packet {{send.ig_port}} {{send.pkt}}
## if clone_info.cloned
## if verify
expect {{clone_info.clone_port}} {{verify.exp_pkt}}$
expect {{verify.eg_port}}
## endif
## else
expect {{clone_info.clone_port}}
## if verify
expect {{verify.eg_port}} {{verify.exp_pkt}}$
## endif
## endif
## endfor
## else
packet {{send.ig_port}} {{send.pkt}}
## if verify
expect {{verify.eg_port}} {{verify.exp_pkt}}$
## endif
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
    if (seed) {
        dataJson["seed"] = *seed;
    }

    dataJson["test_id"] = testId + 1;
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    // Check whether this test has a clone configuration.
    // These are special because they require additional instrumentation and produce two output
    // packets.
    auto cloneInfos = testSpec->getTestObjectCategory("clone_infos");
    if (!cloneInfos.empty()) {
        dataJson["clone_infos"] = getClone(cloneInfos);
    }

    LOG5("STF test back end: emitting testcase:" << std::setw(4) << dataJson);

    inja::render_to(stfFile, testCase, dataJson);
    stfFile.flush();
}

void STF::outputTest(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                     float currentCoverage) {
    auto incrementedTestName = testName + "_" + std::to_string(testIdx);

    stfFile = std::ofstream(incrementedTestName + ".stf");
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Bmv2
