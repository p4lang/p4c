#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/protobuf_ir.h"

#include <filesystem>
#include <iomanip>
#include <optional>
#include <string>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/util.h"
#include "control-plane/p4infoApi.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

ProtobufIr::ProtobufIr(const TestBackendConfiguration &testBackendConfiguration,
                       P4::P4RuntimeAPI p4RuntimeApi)
    : Bmv2TestFramework(testBackendConfiguration), p4RuntimeApi(p4RuntimeApi) {}

std::optional<std::string> ProtobufIr::checkForP4RuntimeTranslationAnnotation(
    const IR::IAnnotated *node) {
    const auto *p4RuntimeTranslationAnnotation = node->getAnnotation("p4runtime_translation");
    if (p4RuntimeTranslationAnnotation == nullptr) {
        return std::nullopt;
    }
    auto annotationVector = p4RuntimeTranslationAnnotation->expr;
    BUG_CHECK(annotationVector.size() == 2, "Expected size of %1% to be 2. ", annotationVector);
    const auto *targetValue = annotationVector.at(1);
    if (targetValue->is<IR::StringLiteral>()) {
        return "str";
    }
    // An integer technically specifies a width but that is not (yet) of any concern to us.
    if (targetValue->is<IR::Constant>()) {
        return "hex_str";
    }
    TESTGEN_UNIMPLEMENTED("Unsupported @p4runtime_translation token %1%", targetValue);
}

std::map<cstring, cstring> ProtobufIr::getP4RuntimeTranslationMappings(const IR::IAnnotated *node) {
    std::map<cstring, cstring> p4RuntimeTranslationMappings;
    const auto *p4RuntimeTranslationMappingAnnotation =
        node->getAnnotation("p4runtime_translation_mappings");
    if (p4RuntimeTranslationMappingAnnotation == nullptr) {
        return p4RuntimeTranslationMappings;
    }
    BUG_CHECK(!p4RuntimeTranslationMappingAnnotation->needsParsing,
              "The @p4runtime_translation_mappings annotation should have been parsed already.");
    auto annotationExpr = p4RuntimeTranslationMappingAnnotation->expr;
    BUG_CHECK(annotationExpr.size() == 1, "Expected size of %1% to be 1. ", annotationExpr);
    const auto *exprList = annotationExpr.at(0)->checkedTo<IR::ListExpression>();
    for (const auto *expr : exprList->components) {
        const auto *exprTuple = expr->checkedTo<IR::ListExpression>();
        const auto &components = exprTuple->components;
        auto left = components.at(0)->checkedTo<IR::StringLiteral>()->value;
        auto right = components.at(1)->checkedTo<IR::Constant>()->value;
        p4RuntimeTranslationMappings.emplace(right.str().c_str(), left);
    }

    return p4RuntimeTranslationMappings;
}

std::string ProtobufIr::getFormatOfNode(const IR::IAnnotated *node) {
    auto p4RuntimeTranslationFormat = checkForP4RuntimeTranslationAnnotation(node);
    if (p4RuntimeTranslationFormat.has_value()) {
        return p4RuntimeTranslationFormat.value();
    }

    const auto *formatAnnotation = node->getAnnotation("format");
    if (formatAnnotation == nullptr) {
        return "hex_str";
    }
    BUG_CHECK(formatAnnotation->body.size() == 1, "@format annotation can only have one member.");
    auto annotationFormatString = formatAnnotation->body.at(0)->text;
    if (annotationFormatString == "IPV4_ADDRESS") {
        return "ipv4";
    }
    if (annotationFormatString == "IPV6_ADDRESS") {
        return "ipv6";
    }
    if (annotationFormatString == "MAC_ADDRESS") {
        return "mac";
    }
    if (annotationFormatString == "HEX_STR") {
        return "hex_str";
    }
    if (annotationFormatString == "STRING") {
        return "str";
    }
    TESTGEN_UNIMPLEMENTED("Unsupported @format string %1%", annotationFormatString);
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
        return formatHexExpr(value, {false, true, true, false});
    }
    // Assume that any string format can be converted from a string literal, bool
    // literal, or constant.
    // TODO: Extract this into a helper function once the Protobuf IR back end is stable.
    if (type == "str") {
        if (const auto *constant = value->to<IR::Constant>()) {
            return constant->value.str();
        }
        if (const auto *literal = value->to<IR::StringLiteral>()) {
            return literal->value.c_str();
        }
        if (const auto *boolValue = value->to<IR::BoolLiteral>()) {
            return boolValue->value ? "true" : "false";
        }
        TESTGEN_UNIMPLEMENTED("Unsupported string format value \"%1%\".", value);
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

std::string ProtobufIr::formatNetworkValue(const IR::IAnnotated *node, const std::string &type,
                                           const IR::Expression *value) {
    auto p4RuntimeTranslationMappings = getP4RuntimeTranslationMappings(node);
    auto formattedNetworkValue = formatNetworkValue(type, value);

    auto it = p4RuntimeTranslationMappings.find(formattedNetworkValue);
    if (it != p4RuntimeTranslationMappings.end()) {
        return (*it).second.c_str();
    }

    return formattedNetworkValue;
}

void ProtobufIr::createKeyMatch(cstring fieldName, const TableMatch &fieldMatch,
                                inja::json &rulesJson) {
    inja::json j;
    j["field_name"] = fieldName;
    const auto *keyElement = fieldMatch.getKey();
    j["format"] = getFormatOfNode(keyElement);

    if (const auto *elem = fieldMatch.to<Exact>()) {
        j["value"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedValue()).c_str();
        rulesJson["single_exact_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<Range>()) {
        j["lo"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedLow()).c_str();
        j["hi"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedHigh()).c_str();
        rulesJson["range_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<Ternary>()) {
        // If the rule has a ternary match we need to add the priority.
        rulesJson["needs_priority"] = true;
        // Skip any ternary match where the mask is all zeroes.
        if (elem->getEvaluatedMask()->value == 0) {
            return;
        }
        j["value"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedValue()).c_str();
        j["mask"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedMask()).c_str();
        rulesJson["ternary_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<LPM>()) {
        j["value"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedValue()).c_str();
        j["prefix_len"] = elem->getEvaluatedPrefixLength()->value.str();
        rulesJson["lpm_matches"].push_back(j);
    } else if (const auto *elem = fieldMatch.to<Optional>()) {
        j["value"] = formatNetworkValue(keyElement, j["format"], elem->getEvaluatedValue()).c_str();
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
        const auto *actionParameter = actArg.getActionParam();
        j["format"] = getFormatOfNode(actionParameter);

        j["param"] = actArg.getActionParamName().c_str();
        j["value"] =
            formatNetworkValue(actionParameter, j["format"], actArg.getEvaluatedValue()).c_str();
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json ProtobufIr::getSend(const TestSpec *testSpec) const {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    sendJson["pkt"] = formatHexExpressionWithSeparators(*payload);
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json ProtobufIr::getExpectedPacket(const TestSpec *testSpec) const {
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

inja::json ProtobufIr::produceTestCase(const TestSpec *testSpec, cstring selectedBranches,
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

void ProtobufIr::writeTestToFile(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                                 float currentCoverage) {
    inja::json dataJson = produceTestCase(testSpec, selectedBranches, testId, currentCoverage);
    LOG5("ProtobufIR test back end: emitting testcase:" << std::setw(4) << dataJson);

    auto optBasePath = getTestBackendConfiguration().fileBasePath;
    BUG_CHECK(optBasePath.has_value(), "Base path is not set.");
    auto incrementedbasePath = optBasePath.value();
    incrementedbasePath.concat("_" + std::to_string(testId));
    incrementedbasePath.replace_extension(".txtpb");
    auto protobufFileStream = std::ofstream(incrementedbasePath);
    inja::render_to(protobufFileStream, getTestCaseTemplate(), dataJson);
    protobufFileStream.flush();
}

AbstractTestReferenceOrError ProtobufIr::produceTest(const TestSpec *testSpec,
                                                     cstring selectedBranches, size_t testId,
                                                     float currentCoverage) {
    inja::json dataJson = produceTestCase(testSpec, selectedBranches, testId, currentCoverage);
    LOG5("ProtobufIR test back end: generated testcase:" << std::setw(4) << dataJson);

    return new ProtobufIrTest(inja::render(getTestCaseTemplate(), dataJson));
}

}  // namespace P4Tools::P4Testgen::Bmv2
