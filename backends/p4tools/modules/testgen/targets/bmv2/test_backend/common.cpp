#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/common.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "ir/ir.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

Bmv2TestFramework::Bmv2TestFramework(const TestBackendConfiguration &testBackendConfiguration)
    : TestFramework(testBackendConfiguration) {}

std::string Bmv2TestFramework::formatHexExpressionWithSeparators(const IR::Expression &expr) {
    return insertHexSeparators(formatHexExpr(&expr, {false, true, false}));
}

inja::json Bmv2TestFramework::getClone(const TestObjectMap &cloneSpecs) const {
    auto cloneSpec = inja::json::object();
    auto cloneJsons = inja::json::array_t();
    auto hasClone = false;
    for (auto cloneSpecTuple : cloneSpecs) {
        inja::json cloneSpecJson;
        const auto *cloneSpec = cloneSpecTuple.second->checkedTo<Bmv2V1ModelCloneSpec>();
        cloneSpecJson["session_id"] = cloneSpec->getEvaluatedSessionId()->asUint64();
        cloneSpecJson["clone_port"] = cloneSpec->getEvaluatedClonePort()->asInt();
        cloneSpecJson["cloned"] = cloneSpec->isClonedPacket();
        hasClone = hasClone || cloneSpec->isClonedPacket();
        cloneJsons.push_back(cloneSpecJson);
    }
    cloneSpec["clone_pkts"] = cloneJsons;
    cloneSpec["has_clone"] = hasClone;
    return cloneSpec;
}

inja::json::array_t Bmv2TestFramework::getMeter(const TestObjectMap &meterValues) const {
    auto meterJson = inja::json::array_t();
    for (auto meterValueInfoTuple : meterValues) {
        const auto *meterValue = meterValueInfoTuple.second->checkedTo<Bmv2V1ModelMeterValue>();

        const auto meterEntries = meterValue->unravelMap();
        for (const auto &meterEntry : meterEntries) {
            inja::json meterInfoJson;
            meterInfoJson["name"] = meterValueInfoTuple.first;
            meterInfoJson["value"] = formatHexExpr(meterEntry.second.second);
            meterInfoJson["index"] = formatHex(meterEntry.first, meterEntry.second.first);
            if (meterValue->isDirectMeter()) {
                meterInfoJson["is_direct"] = "True";
            } else {
                meterInfoJson["is_direct"] = "False";
            }
            meterJson.push_back(meterInfoJson);
        }
    }
    return meterJson;
}

inja::json Bmv2TestFramework::getControlPlaneTable(const TableConfig &tblConfig) const {
    inja::json tblJson;
    tblJson["table_name"] = tblConfig.getTable()->controlPlaneName();
    const auto *tblRules = tblConfig.getRules();
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
    return tblJson;
}

inja::json Bmv2TestFramework::getControlPlane(const TestSpec *testSpec) const {
    auto controlPlaneJson = inja::json::object();
    // Map of actionProfiles and actionSelectors for easy reference.
    std::map<cstring, cstring> apAsMap;

    auto tables = testSpec->getTestObjectCategory("tables");
    if (!tables.empty()) {
        controlPlaneJson["tables"] = inja::json::array();
    }
    for (const auto &testObject : tables) {
        const auto *const tblConfig = testObject.second->checkedTo<TableConfig>();
        inja::json tblJson = getControlPlaneTable(*tblConfig);

        // Collect action profiles and selectors associated with the table.
        checkForTableActionProfile<Bmv2V1ModelActionProfile, Bmv2V1ModelActionSelector>(
            tblJson, apAsMap, tblConfig);

        // Check whether the default action is overridden for this table.
        checkForDefaultActionOverride(tblJson, tblConfig);

        controlPlaneJson["tables"].push_back(tblJson);
    }

    // Collect declarations of action profiles.
    collectActionProfileDeclarations<Bmv2V1ModelActionProfile>(testSpec, controlPlaneJson, apAsMap);

    return controlPlaneJson;
}

inja::json Bmv2TestFramework::getControlPlaneForTable(const TableMatchMap &matches,
                                                      const std::vector<ActionArg> &args) const {
    inja::json rulesJson;

    rulesJson["single_exact_matches"] = inja::json::array();
    rulesJson["multiple_exact_matches"] = inja::json::array();
    rulesJson["range_matches"] = inja::json::array();
    rulesJson["ternary_matches"] = inja::json::array();
    rulesJson["lpm_matches"] = inja::json::array();
    rulesJson["optional_matches"] = inja::json::array();

    rulesJson["act_args"] = inja::json::array();
    rulesJson["needs_priority"] = false;

    // Iterate over the match fields and segregate them.
    for (const auto &match : matches) {
        const auto fieldName = match.first;
        const auto &fieldMatch = match.second;

        inja::json j;
        j["field_name"] = fieldName;
        if (const auto *elem = fieldMatch->to<Exact>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue()).c_str();
            rulesJson["single_exact_matches"].push_back(j);
        } else if (const auto *elem = fieldMatch->to<Range>()) {
            j["lo"] = formatHexExpr(elem->getEvaluatedLow()).c_str();
            j["hi"] = formatHexExpr(elem->getEvaluatedHigh()).c_str();
            rulesJson["range_matches"].push_back(j);
        } else if (const auto *elem = fieldMatch->to<Ternary>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue()).c_str();
            j["mask"] = formatHexExpr(elem->getEvaluatedMask()).c_str();
            rulesJson["ternary_matches"].push_back(j);
            // If the rule has a ternary match we need to add the priority.
            rulesJson["needs_priority"] = true;
        } else if (const auto *elem = fieldMatch->to<LPM>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue()).c_str();
            j["prefix_len"] = elem->getEvaluatedPrefixLength()->value.str();
            rulesJson["lpm_matches"].push_back(j);
        } else if (const auto *elem = fieldMatch->to<Optional>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue()).c_str();
            if (elem->addAsExactMatch()) {
                j["use_exact"] = "True";
            } else {
                j["use_exact"] = "False";
            }
            rulesJson["needs_priority"] = true;
            rulesJson["optional_matches"].push_back(j);
        } else {
            TESTGEN_UNIMPLEMENTED("Unsupported table key match type \"%1%\"",
                                  fieldMatch->getObjectName());
        }
    }

    for (const auto &actArg : args) {
        inja::json j;
        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatHexExpr(actArg.getEvaluatedValue()).c_str();
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json Bmv2TestFramework::getSend(const TestSpec *testSpec) const {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload, {false, true, false});
    sendJson["pkt"] = insertHexSeparators(dataStr);
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json Bmv2TestFramework::getExpectedPacket(const TestSpec *testSpec) const {
    inja::json verifyData = inja::json::object();
    auto egressPacket = testSpec->getEgressPacket();
    if (egressPacket.has_value()) {
        const auto *packet = egressPacket.value();
        verifyData["eg_port"] = packet->getPort();
        const auto *payload = packet->getEvaluatedPayload();
        const auto *payloadMask = packet->getEvaluatedPayloadMask();
        verifyData["ignore_mask"] = formatHexExpr(payloadMask);
        verifyData["exp_pkt"] = formatHexExpr(payload);
    }
    return verifyData;
}

}  // namespace P4Tools::P4Testgen::Bmv2
