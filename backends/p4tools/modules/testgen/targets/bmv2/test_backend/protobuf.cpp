#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/protobuf.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <inja/inja.hpp>

#include "backends/p4tools/common/control_plane/p4info_map.h"
#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/util.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

Protobuf::Protobuf(const TestBackendConfiguration &testBackendConfiguration,
                   P4::P4RuntimeAPI p4RuntimeApi)
    : Bmv2TestFramework(testBackendConfiguration),
      p4RuntimeApi(p4RuntimeApi),
      p4InfoMaps(P4::ControlPlaneAPI::P4InfoMaps(*p4RuntimeApi.p4Info)) {}

inja::json Protobuf::getControlPlane(const TestSpec *testSpec) const {
    inja::json controlPlaneJson = inja::json::object();

    // Map of actionProfiles and actionSelectors for easy reference.
    std::map<cstring, cstring> apAsMap;

    auto tables = testSpec->getTestObjectCategory("tables");
    if (!tables.empty()) {
        controlPlaneJson["tables"] = inja::json::array();
    }
    for (auto const &testObject : tables) {
        inja::json tblJson;
        auto tableName = testObject.first;
        tblJson["table_name"] = tableName;
        const auto *const tblConfig = testObject.second->checkedTo<TableConfig>();
        const auto *table = tblConfig->getTable();
        auto p4RuntimeId = p4InfoMaps.lookUpP4RuntimeId(table->controlPlaneName());
        BUG_CHECK(p4RuntimeId, "Id not present for table %1%. Can not generate test.", table);
        tblJson["id"] = p4RuntimeId.value();
        const auto *tblRules = tblConfig->getRules();
        tblJson["rules"] = inja::json::array();
        for (const auto &tblRule : *tblRules) {
            inja::json rule;
            const auto *matches = tblRule.getMatches();
            const auto *actionCall = tblRule.getActionCall();
            const auto *actionDecl = actionCall->getAction();
            cstring actionName = actionDecl->controlPlaneName();
            const auto *actionArgs = actionCall->getArgs();
            rule["action_name"] = actionCall->getActionName().c_str();
            auto p4RuntimeId = p4InfoMaps.lookUpP4RuntimeId(actionName);
            BUG_CHECK(p4RuntimeId, "Id not present for action %1%. Can not generate test.",
                      actionDecl);
            rule["action_id"] = p4RuntimeId.value();

            auto j = getControlPlaneForTable(tableName, actionName, *matches, *actionArgs);
            rule["rules"] = std::move(j);
            rule["priority"] = tblRule.getPriority();
            tblJson["rules"].push_back(rule);
        }

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

inja::json Protobuf::getControlPlaneForTable(cstring tableName, cstring actionName,
                                             const TableMatchMap &matches,
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

    for (auto const &match : matches) {
        auto const fieldName = match.first;
        auto const &fieldMatch = match.second;

        inja::json j;
        j["field_name"] = fieldName;
        auto combinedFieldName = tableName + "_" + fieldName;
        auto p4RuntimeId = p4InfoMaps.lookUpP4RuntimeId(combinedFieldName);
        BUG_CHECK(p4RuntimeId.has_value(), "Id not present for key. Can not generate test.");
        j["id"] = p4RuntimeId.value();

        // Iterate over the match fields and segregate them.
        if (const auto *elem = fieldMatch->to<Exact>()) {
            j["value"] = formatHexExpressionWithSeparators(*elem->getEvaluatedValue());
            rulesJson["single_exact_matches"].push_back(j);
        } else if (const auto *elem = fieldMatch->to<Range>()) {
            j["lo"] = formatHexExpressionWithSeparators(*elem->getEvaluatedLow());
            j["hi"] = formatHexExpressionWithSeparators(*elem->getEvaluatedHigh());
            rulesJson["range_matches"].push_back(j);
            // If the rule has a range match we need to add the priority.
            rulesJson["needs_priority"] = true;
        } else if (const auto *elem = fieldMatch->to<Ternary>()) {
            j["value"] = formatHexExpressionWithSeparators(*elem->getEvaluatedValue());
            j["mask"] = formatHexExpressionWithSeparators(*elem->getEvaluatedMask());
            rulesJson["ternary_matches"].push_back(j);
            // If the rule has a range match we need to add the priority.
            rulesJson["needs_priority"] = true;
        } else if (const auto *elem = fieldMatch->to<LPM>()) {
            j["value"] = formatHexExpressionWithSeparators(*elem->getEvaluatedValue());
            j["prefix_len"] = elem->getEvaluatedPrefixLength()->value.str();
            rulesJson["lpm_matches"].push_back(j);
        } else if (const auto *elem = fieldMatch->to<Optional>()) {
            j["value"] = formatHexExpr(elem->getEvaluatedValue()).c_str();
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
        j["value"] = formatHexExpressionWithSeparators(*actArg.getEvaluatedValue());
        auto combinedParamName = actionName + "_" + actArg.getActionParamName();
        auto p4RuntimeId = p4InfoMaps.lookUpP4RuntimeId(combinedParamName);
        BUG_CHECK(p4RuntimeId.has_value(), "Id not present for parameter. Can not generate test.");
        j["id"] = p4RuntimeId.value();
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json Protobuf::getSend(const TestSpec *testSpec) const {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    sendJson["pkt"] = formatHexExpressionWithSeparators(*payload);
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json Protobuf::getExpectedPacket(const TestSpec *testSpec) const {
    inja::json verifyData = inja::json::object();
    auto egressPacket = testSpec->getEgressPacket();
    if (egressPacket.has_value()) {
        const auto *packet = egressPacket.value();
        verifyData["eg_port"] = packet->getPort();
        const auto *payload = packet->getEvaluatedPayload();
        const auto *mask = packet->getEvaluatedPayloadMask();
        verifyData["ignore_mask"] = formatHexExpressionWithSeparators(*mask);
        verifyData["exp_pkt"] = formatHexExpressionWithSeparators(*payload);
    }
    return verifyData;
}

std::string Protobuf::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(
# proto-file: p4testgen.proto
# proto-message: TestCase
# A P4TestGen-generated test case for {{test_name}}.p4
metadata: "p4testgen seed: {{ default(seed, "none") }}"
metadata: "Date generated: {{timestamp}}"
## if length(selected_branches) > 0
metadata: "{{selected_branches}}"
## endif
metadata: "Current node coverage: {{coverage}}"

## for trace_item in trace
traces: '{{trace_item}}'
## endfor

input_packet {
  packet: "{{send.pkt}}"
  port: {{send.ig_port}}
}

## if verify
expected_output_packet {
  packet: "{{verify.exp_pkt}}"
  port: {{verify.eg_port}}
  packet_mask: "{{verify.ignore_mask}}"
}
## endif

## if control_plane
## for table in control_plane.tables
## for rule in table.rules
# Table {{table.table_name}}
entities {
  table_entry {
    table_id: {{table.id}}
## if rule.rules.needs_priority
    priority: {{rule.priority}}
## endif
## for r in rule.rules.single_exact_matches
    # Match field {{r.field_name}}
    match {
      field_id: {{r.id}}
      exact {
        value: "{{r.value}}"
      }
    }
## endfor
## for r in rule.rules.optional_matches
  # Match field {{r.field_name}}
  match {
    field_id: {{r.id}}
    optional {
      value: "{{r.value}}"
    }
  }
## endfor
## for r in rule.rules.range_matches
    # Match field {{r.field_name}}
    match {
      field_id: {{r.id}}
      range {
        low: "{{r.lo}}"
        high: "{{r.hi}}"
      }
    }
## endfor
## for r in rule.rules.ternary_matches
    # Match field {{r.field_name}}
    match {
      field_id: {{r.id}}
      ternary {
        value: "{{r.value}}"
        mask: "{{r.mask}}"
      }
    }
## endfor
## for r in rule.rules.lpm_matches
    # Match field {{r.field_name}}
    match {
      field_id: {{r.id}}
      lpm {
        value: "{{r.value}}"
        prefix_len: {{r.prefix_len}}
      }
    }
## endfor
    # Action {{rule.action_name}}
    action {
## if existsIn(table, "has_ap")
      action_profile_action_set {
        action_profile_actions {
          action {
            action_id: {{rule.action_id}}
## for act_param in rule.rules.act_args
            # Param {{act_param.param}}
            params {
              param_id: {{act_param.id}}
              value: "{{act_param.value}}"
            }
## endfor
          }
        }
      }
## else
      action {
        action_id: {{rule.action_id}}
## for act_param in rule.rules.act_args
        # Param {{act_param.param}}
        params {
          param_id: {{act_param.id}}
          value: "{{act_param.value}}"
        }
## endfor
      }
## endif
    }
## endfor
  }
}
## endfor
## endif
)""");
    return TEST_CASE;
}

inja::json Protobuf::produceTestCase(const TestSpec *testSpec, cstring selectedBranches,
                                     size_t testId, float currentCoverage) const {
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
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getExpectedPacket(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    // Check whether this test has a clone configuration.
    // These are special because they require additional instrumentation and produce two output
    // packets.
    auto cloneSpecs = testSpec->getTestObjectCategory("clone_specs");
    if (!cloneSpecs.empty()) {
        dataJson["clone_specs"] = getClone(cloneSpecs);
    }
    auto meterValues = testSpec->getTestObjectCategory("meter_values");
    dataJson["meter_values"] = getMeter(meterValues);

    return dataJson;
}

void Protobuf::writeTestToFile(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                               float currentCoverage) {
    inja::json dataJson = produceTestCase(testSpec, selectedBranches, testId, currentCoverage);
    LOG5("Protobuf test back end: emitting testcase:" << std::setw(4) << dataJson);

    auto optBasePath = getTestBackendConfiguration().fileBasePath;
    BUG_CHECK(optBasePath.has_value(), "Base path is not set.");
    auto incrementedbasePath = optBasePath.value();
    incrementedbasePath.concat("_" + std::to_string(testId));
    incrementedbasePath.replace_extension(".txtpb");
    auto protobufFileStream = std::ofstream(incrementedbasePath);
    inja::render_to(protobufFileStream, getTestCaseTemplate(), dataJson);
    protobufFileStream.flush();
}

AbstractTestReferenceOrError Protobuf::produceTest(const TestSpec *testSpec,
                                                   cstring selectedBranches, size_t testId,
                                                   float currentCoverage) {
    inja::json dataJson = produceTestCase(testSpec, selectedBranches, testId, currentCoverage);
    LOG5("ProtobufIR test back end: generated testcase:" << std::setw(4) << dataJson);

    return new ProtobufTest(inja::render(getTestCaseTemplate(), dataJson));
}

}  // namespace P4Tools::P4Testgen::Bmv2
