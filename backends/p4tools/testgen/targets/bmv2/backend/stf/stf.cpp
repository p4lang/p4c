#include "backends/p4tools/testgen/targets/bmv2/backend/stf/stf.h"

#include <algorithm>
#include <iomanip>
#include <map>
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
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/gmputil.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/testgen/lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

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

    // Map of actionProfiles and actionSelectors for easy reference.
    std::map<cstring, cstring> apASMap;

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

        // Action Profile
        const auto* apObject = tblConfig->getProperty("action_profile", false);
        const Bmv2_V1ModelActionProfile* actionProfile = nullptr;
        const Bmv2_V1ModelActionSelector* actionSelector = nullptr;
        if (apObject != nullptr) {
            actionProfile = apObject->checkedTo<Bmv2_V1ModelActionProfile>();
            // Check if we have an Action Selector too.
            // TODO: Change this to check in ActionSelector with table
            // property "action_selectors".
            const auto* asObject = tblConfig->getProperty("action_selector", false);
            if (asObject != nullptr) {
                actionSelector = asObject->checkedTo<Bmv2_V1ModelActionSelector>();
                apASMap[actionProfile->getProfileDecl()->controlPlaneName()] =
                    actionSelector->getSelectorDecl()->controlPlaneName();
            }
        }
        if (actionProfile != nullptr) {
            tblJson["has_ap"] = true;
        }

        if (actionSelector != nullptr) {
            tblJson["has_as"] = true;
        }

        controlPlaneJson["tables"].push_back(tblJson);
    }
    auto actionProfiles = testSpec->getTestObjectCategory("action_profiles");
    if (!actionProfiles.empty()) {
        controlPlaneJson["action_profiles"] = inja::json::array();
    }
    for (auto const& testObject : actionProfiles) {
        const auto* const actionProfile = testObject.second->checkedTo<Bmv2_V1ModelActionProfile>();
        const auto* actions = actionProfile->getActions();
        inja::json j;
        j["profile"] = actionProfile->getProfileDecl()->controlPlaneName();
        j["actions"] = inja::json::array();
        for (size_t idx = 0; idx < actions->size(); ++idx) {
            const auto& action = actions->at(idx);
            auto actionName = action.first;
            auto actionArgs = action.second;
            inja::json a;
            a["action_name"] = actionName;
            a["action_idx"] = std::to_string(idx);
            inja::json b = inja::json::array();
            for (const auto& actArg : actionArgs) {
                inja::json c;
                c["param"] = actArg.getActionParamName().c_str();
                c["value"] = formatHexExpr(actArg.getEvaluatedValue()).c_str();
                b.push_back(c);
            }
            a["action_args"] = b;
            j["actions"].push_back(a);
        }
        if (apASMap.find(actionProfile->getProfileDecl()->controlPlaneName()) != apASMap.end()) {
            j["selector"] = apASMap[actionProfile->getProfileDecl()->controlPlaneName()];
        }
        controlPlaneJson["action_profiles"].push_back(j);
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
        auto const fieldName = match.first;
        auto const& fieldMatch = match.second;

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
                auto maxVal = IR::IRUtils::getMaxBvVal(prefixLen);
                const auto* maskField =
                    IR::IRUtils::getConstant(dataValue->type, maxVal << (fieldWidth - prefixLen));
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

inja::json::array_t STF::getClone(const std::map<cstring, const TestObject*>& cloneInfos) {
    auto cloneJson = inja::json::array_t();
    for (auto cloneInfoTuple : cloneInfos) {
        inja::json cloneInfoJson;
        const auto* cloneInfo = cloneInfoTuple.second->checkedTo<Bmv2_CloneInfo>();
        cloneInfoJson["session_id"] = cloneInfo->getEvaluatedSessionId()->asUint64();
        cloneInfoJson["clone_port"] = cloneInfo->getEvaluatedClonePort()->asInt();
        cloneInfoJson["cloned"] = cloneInfo->isClonedPacket();
        cloneJson.push_back(cloneInfoJson);
    }
    return cloneJson;
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

void STF::outputTest(const TestSpec* testSpec, cstring selectedBranches, size_t testIdx,
                     float currentCoverage) {
    auto incrementedTestName = testName + "_" + std::to_string(testIdx);

    stfFile = std::ofstream(incrementedTestName + ".stf");
    std::string testCase = getTestCase();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
