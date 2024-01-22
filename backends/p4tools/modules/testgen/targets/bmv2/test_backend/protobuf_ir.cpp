#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/protobuf_ir.h"

#include <filesystem>
#include <iomanip>
#include <optional>
#include <string>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "control-plane/p4infoApi.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

ProtobufIr::ProtobufIr(const TestBackendConfiguration &testBackendConfiguration,
                       P4::P4RuntimeAPI p4RuntimeApi)
    : Bmv2TestFramework(testBackendConfiguration), p4RuntimeApi(p4RuntimeApi) {}

std::string ProtobufIr::getFormatOfNode(const IR::IAnnotated *node) {
    const auto *formatAnnotation = node->getAnnotation("format");
    std::string formatString = "hex_str";
    if (formatAnnotation != nullptr) {
        BUG_CHECK(formatAnnotation->body.size() == 1,
                  "@format annotation can only have one member.");
        auto formatString = formatAnnotation->body.at(0)->text;
        if (formatString == "IPV4_ADDRESS") {
            formatString = "ipv4";
        } else if (formatString == "IPV6_ADDRESS") {
            formatString = "ipv6";
        } else if (formatString == "MAC_ADDRESS") {
            formatString = "mac";
        } else if (formatString == "HEX_STR") {
            formatString = "hex_str";
        } else {
            TESTGEN_UNIMPLEMENTED("Unsupported @format string %1%", formatString);
        }
    }
    return formatString;
}

std::string ProtobufIr::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# proto-file: p4testgen_ir.proto
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
    table_name: "{{table.table_name}}"
## if rule.rules.needs_priority
    priority: {{rule.priority}}
## endif
## for r in rule.rules.single_exact_matches
    # Match field {{r.field_name}}
    matches {
      name: "{{r.field_name}}"
      exact: { {{r.format}}: "{{r.value}}" }
    }
## endfor
## for r in rule.rules.optional_matches
  # Match field {{r.field_name}}
  matches {
    name: "{{r.field_name}}"
    optional {
      value: { {{r.format}}: "{{r.value}}" }
    }
  }
## endfor
## for r in rule.rules.range_matches
    # Match field {{r.field_name}}
    matches {
      name: "{{r.field_name}}"
      range {
        low: { {{r.format}}: "{{r.lo}}" }
        high: { {{r.format}}:  "{{r.hi}}" }
      }
    }
## endfor
## for r in rule.rules.ternary_matches
    # Match field {{r.field_name}}
    matches {
      name: "{{r.field_name}}"
      ternary {
        value: { {{r.format}}: "{{r.value}}" }
        mask: { {{r.format}}: "{{r.mask}}" }
      }
    }
## endfor
## for r in rule.rules.lpm_matches
    # Match field {{r.field_name}}
    matches {
      name: "{{r.field_name}}"
      lpm {
        value: { {{r.format}}: "{{r.value}}" }
        prefix_length: {{r.prefix_len}}
      }
    }
## endfor
    # Action {{rule.action_name}}
## if existsIn(table, "has_ap")
      action_set {
        actions {
          action {
            name: "{{rule.action_name}}"
## for act_param in rule.rules.act_args
            # Param {{act_param.param}}
            params {
              name: "{{act_param.param}}"
              value: { {{act_param.format}}: "{{act_param.value}}" }
            }
## endfor
          }
        }
      }
## else
      action {
        name: "{{rule.action_name}}"
## for act_param in rule.rules.act_args
        # Param {{act_param.param}}
        params {
          name: "{{act_param.param}}"
          value: { {{act_param.format}}: "{{act_param.value}}" }
      }
## endfor
    }
## endif
## endfor
  }
}
## endfor
## endif
)""");
    return TEST_CASE;
}

std::string ProtobufIr::formatNetworkValue(const std::string &type, const IR::Expression *value) {
    if (type == "hex_str") {
        return formatHexExpr(value, {false, true, false, false});
    }
    // At this point, any value must be a constant.
    const auto *constant = value->checkedTo<IR::Constant>();
    if (type == "ipv4") {
        auto convertedString = convertToIpv4String(convertBigIntToBytes(constant->value, 32));
        if (convertedString.has_value()) {
            return convertedString.value();
        }
        BUG("Failed to convert \"%1%\" to an IPv4 string.", value);
    }
    if (type == "ipv6") {
        auto convertedString = convertToIpv6String(convertBigIntToBytes(constant->value, 128));
        if (convertedString.has_value()) {
            return convertedString.value();
        }
        BUG("Failed to convert \"%1%\" to an IPv6 string.", value);
    }
    if (type == "mac") {
        auto convertedString = convertToMacString(convertBigIntToBytes(constant->value, 48));
        if (convertedString.has_value()) {
            return convertedString.value();
        }
        BUG("Failed to convert \"%1%\" to an MAC string.", value);
    }
    TESTGEN_UNIMPLEMENTED("Unsupported network value type %1%", type);
}

void ProtobufIr::createKeyMatch(cstring fieldName, const TableMatch &fieldMatch,
                                inja::json &rulesJson) {
    inja::json j;
    j["field_name"] = fieldName;
    j["format"] = getFormatOfNode(fieldMatch.getKey());

    if (const auto *elem = fieldMatch.to<Exact>()) {
        j["value"] = formatNetworkValue(j["format"], elem->getEvaluatedValue()).c_str();
        rulesJson["single_exact_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<Range>()) {
        j["lo"] = formatNetworkValue(j["format"], elem->getEvaluatedLow()).c_str();
        j["hi"] = formatNetworkValue(j["format"], elem->getEvaluatedHigh()).c_str();
        rulesJson["range_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<Ternary>()) {
        j["value"] = formatNetworkValue(j["format"], elem->getEvaluatedValue()).c_str();
        j["mask"] = formatNetworkValue(j["format"], elem->getEvaluatedMask()).c_str();
        rulesJson["ternary_matches"].push_back(j);
        // If the rule has a ternary match we need to add the priority.
        rulesJson["needs_priority"] = true;
    } else if (const auto *elem = fieldMatch.to<LPM>()) {
        j["value"] = formatNetworkValue(j["format"], elem->getEvaluatedValue()).c_str();
        j["prefix_len"] = elem->getEvaluatedPrefixLength()->value.str();
        rulesJson["lpm_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<Optional>()) {
        j["value"] = formatNetworkValue(j["format"], elem->getEvaluatedValue()).c_str();
        if (elem->addAsExactMatch()) {
            j["use_exact"] = "True";
        } else {
            j["use_exact"] = "False";
        }
        rulesJson["needs_priority"] = true;
        rulesJson["optional_matches"].push_back(j);
    } else {
        TESTGEN_UNIMPLEMENTED("Unsupported table key match type \"%1%\"",
                              fieldMatch.getObjectName());
    }
}

inja::json ProtobufIr::getControlPlaneTable(const TableConfig &tblConfig) const {
    inja::json tblJson;
    const auto *p4RuntimeTableOpt = P4::ControlPlaneAPI::findP4RuntimeTable(
        *p4RuntimeApi.p4Info, tblConfig.getTable()->controlPlaneName());
    BUG_CHECK(p4RuntimeTableOpt != nullptr, "Table not found in the P4Info file.");
    tblJson["table_name"] = p4RuntimeTableOpt->preamble().alias();

    const auto *tblRules = tblConfig.getRules();
    tblJson["rules"] = inja::json::array();
    for (const auto &tblRule : *tblRules) {
        inja::json rule;
        const auto *matches = tblRule.getMatches();
        const auto *actionCall = tblRule.getActionCall();
        const auto *actionArgs = actionCall->getArgs();
        const auto *p4RuntimeActionOpt = P4::ControlPlaneAPI::findP4RuntimeAction(
            *p4RuntimeApi.p4Info, actionCall->getAction()->controlPlaneName());
        BUG_CHECK(p4RuntimeActionOpt != nullptr, "Action not found in the P4Info file.");
        rule["action_name"] = p4RuntimeActionOpt->preamble().alias();
        auto j = getControlPlaneForTable(*matches, *actionArgs);
        rule["rules"] = std::move(j);
        rule["priority"] = tblRule.getPriority();
        tblJson["rules"].push_back(rule);
    }
    return tblJson;
}

inja::json ProtobufIr::getControlPlaneForTable(const TableMatchMap &matches,
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
        createKeyMatch(match.first, *match.second, rulesJson);
    }

    for (const auto &actArg : args) {
        inja::json j;
        j["format"] = getFormatOfNode(actArg.getActionParam());

        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatNetworkValue(j["format"], actArg.getEvaluatedValue()).c_str();
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

void ProtobufIr::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                              const std::string &testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }

    auto optSeed = getTestBackendConfiguration().seed;
    if (optSeed.has_value()) {
        dataJson["seed"] = optSeed.value();
    }
    dataJson["test_name"] = getTestBackendConfiguration().testName;
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

    LOG5("ProtobufIR test back end: emitting testcase:" << std::setw(4) << dataJson);

    auto optBasePath = getTestBackendConfiguration().fileBasePath;
    BUG_CHECK(optBasePath.has_value(), "Base path is not set.");
    auto incrementedbasePath = optBasePath.value();
    incrementedbasePath.concat("_" + std::to_string(testId));
    incrementedbasePath.replace_extension(".txtpb");
    auto protobufFileStream = std::ofstream(incrementedbasePath);
    inja::render_to(protobufFileStream, testCase, dataJson);
    protobufFileStream.flush();
}

void ProtobufIr::outputTest(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                            float currentCoverage) {
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testId, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Bmv2
