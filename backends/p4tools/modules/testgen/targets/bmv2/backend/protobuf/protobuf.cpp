#include "backends/p4tools/modules/testgen/targets/bmv2/backend/protobuf/protobuf.h"

#include <iomanip>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/none.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "gsl/gsl-lite.hpp"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/big_int_util.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/// Wrapper helper function that automatically inserts separators for hex strings.
std::string formatHexExprWithSep(const IR::Expression *expr) {
    return insertHexSeparators(formatHexExpr(expr, false, true, false));
}

Protobuf::Protobuf(cstring testName, boost::optional<unsigned int> seed = boost::none)
    : TF(testName, seed) {}

boost::optional<p4rt_id_t> Protobuf::getIdAnnotation(const IR::IAnnotated *node) {
    const auto *idAnnotation = node->getAnnotation("id");
    if (idAnnotation == nullptr) {
        return boost::none;
    }
    const auto *idConstant = idAnnotation->expr[0]->to<IR::Constant>();
    CHECK_NULL(idConstant);
    if (!idConstant->fitsUint()) {
        ::error(ErrorType::ERR_INVALID, "%1%: @id should be an unsigned integer", node);
        return boost::none;
    }
    return static_cast<p4rt_id_t>(idConstant->value);
}

boost::optional<p4rt_id_t> Protobuf::externalId(
    const P4::ControlPlaneAPI::P4RuntimeSymbolType &type, const IR::IDeclaration *declaration) {
    CHECK_NULL(declaration);
    if (!declaration->is<IR::IAnnotated>()) {
        return boost::none;  // Assign an id later; see below.
    }

    // If the user specified an @id annotation, use that.
    auto idOrNone = getIdAnnotation(declaration->to<IR::IAnnotated>());
    if (!idOrNone) {
        return boost::none;  // the user didn't assign an id
    }
    auto id = *idOrNone;

    // If the id already has an 8-bit type prefix, make sure it is correct for
    // the resource type; otherwise assign the correct prefix.
    const auto typePrefix = static_cast<p4rt_id_t>(type) << 24;
    const auto prefixMask = static_cast<p4rt_id_t>(0xff) << 24;
    if ((id & prefixMask) != 0 && (id & prefixMask) != typePrefix) {
        ::error(ErrorType::ERR_INVALID, "%1%: @id has the wrong 8-bit prefix", declaration);
        return boost::none;
    }
    id |= typePrefix;

    return id;
}

std::vector<std::pair<size_t, size_t>> Protobuf::getIgnoreMasks(const IR::Constant *mask) {
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

inja::json Protobuf::getControlPlane(const TestSpec *testSpec) {
    inja::json controlPlaneJson = inja::json::object();

    // Map of actionProfiles and actionSelectors for easy reference.
    std::map<cstring, cstring> apAsMap;

    auto tables = testSpec->getTestObjectCategory("tables");
    if (!tables.empty()) {
        controlPlaneJson["tables"] = inja::json::array();
    }
    for (auto const &testObject : tables) {
        inja::json tblJson;
        tblJson["table_name"] = testObject.first.c_str();
        const auto *const tblConfig = testObject.second->checkedTo<TableConfig>();
        const auto *table = tblConfig->getTable();

        auto p4RuntimeId = externalId(SymbolType::P4RT_TABLE(), table);
        BUG_CHECK(p4RuntimeId, "Id not present for table %1%. Can not generate test.", table);
        tblJson["id"] = *p4RuntimeId;

        const auto *tblRules = tblConfig->getRules();
        tblJson["rules"] = inja::json::array();
        for (const auto &tblRule : *tblRules) {
            inja::json rule;
            const auto *matches = tblRule.getMatches();
            const auto *actionCall = tblRule.getActionCall();
            const auto *actionDecl = actionCall->getAction();
            const auto *actionArgs = actionCall->getArgs();
            rule["action_name"] = actionCall->getActionName().c_str();
            auto p4RuntimeId = externalId(SymbolType::P4RT_ACTION(), actionDecl);
            BUG_CHECK(p4RuntimeId, "Id not present for action %1%. Can not generate test.",
                      actionDecl);
            rule["action_id"] = *p4RuntimeId;
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

inja::json Protobuf::getControlPlaneForTable(const std::map<cstring, const FieldMatch> &matches,
                                             const std::vector<ActionArg> &args) {
    inja::json rulesJson;

    rulesJson["single_exact_matches"] = inja::json::array();
    rulesJson["multiple_exact_matches"] = inja::json::array();
    rulesJson["range_matches"] = inja::json::array();
    rulesJson["ternary_matches"] = inja::json::array();
    rulesJson["lpm_matches"] = inja::json::array();
    rulesJson["act_args"] = inja::json::array();
    rulesJson["needs_priority"] = false;

    for (auto const &match : matches) {
        auto const fieldName = match.first;
        auto const &fieldMatch = match.second;

        // Iterate over the match fields and segregate them.
        struct GetRange : public boost::static_visitor<void> {
            cstring fieldName;
            inja::json &rulesJson;

            GetRange(inja::json &rulesJson, cstring fieldName)
                : fieldName(fieldName), rulesJson(rulesJson) {}

            void operator()(const Exact &elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExprWithSep(elem.getEvaluatedValue());
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["single_exact_matches"].push_back(j);
            }
            void operator()(const Range &elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["lo"] = formatHexExprWithSep(elem.getEvaluatedLow());
                j["hi"] = formatHexExprWithSep(elem.getEvaluatedHigh());
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["range_matches"].push_back(j);
                // If the rule has a range match we need to add the priority.
                rulesJson["needs_priority"] = true;
            }
            void operator()(const Ternary &elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExprWithSep(elem.getEvaluatedValue());
                j["mask"] = formatHexExprWithSep(elem.getEvaluatedMask());
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["ternary_matches"].push_back(j);
                // If the rule has a range match we need to add the priority.
                rulesJson["needs_priority"] = true;
            }
            void operator()(const LPM &elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExprWithSep(elem.getEvaluatedValue());
                j["prefix_len"] = elem.getEvaluatedPrefixLength()->value.str();
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["lpm_matches"].push_back(j);
            }
            void operator()(const Optional &elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExpr(elem.getEvaluatedValue()).c_str();
                rulesJson["needs_priority"] = true;
                rulesJson["optional_matches"].push_back(j);
            }
        };
        boost::apply_visitor(GetRange(rulesJson, fieldName), fieldMatch);
    }

    for (const auto &actArg : args) {
        inja::json j;
        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatHexExprWithSep(actArg.getEvaluatedValue());
        auto p4RuntimeId = getIdAnnotation(actArg.getActionParam());
        BUG_CHECK(p4RuntimeId, "Id not present for parameter. Can not generate test.");
        j["id"] = *p4RuntimeId;
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json Protobuf::getSend(const TestSpec *testSpec) {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExprWithSep(payload);
    sendJson["pkt"] = dataStr;
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json Protobuf::getVerify(const TestSpec *testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != boost::none) {
        const auto &packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto *payload = packet.getEvaluatedPayload();
        const auto *payloadMask = packet.getEvaluatedPayloadMask();
        verifyData["ignore_mask"] = formatHexExprWithSep(payloadMask);
        verifyData["exp_pkt"] = formatHexExprWithSep(payload);
    }
    return verifyData;
}

std::string Protobuf::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# A P4TestGen-generated test case for {{test_name}}.p4
metadata: "p4testgen seed: {{ default(seed, "none") }}"
metadata: "Date generated: {{timestamp}}"
## if length(selected_branches) > 0
metadata: "{{selected_branches}}"
## endif
metadata: "Current statement coverage: {{coverage}}"

## for trace_item in trace
traces: "{{trace_item}}"
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
entities : [
## for table in control_plane.tables
## for rule in table.rules
  # Table {{table.table_name}}
  {
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
  }{% if not loop.is_last %},{% endif %}
## endfor
]
## endif
)""");
    return TEST_CASE;
}

void Protobuf::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                            const std::string &testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }
    boost::filesystem::path testFile(testName + ".proto");
    cstring testNameOnly(testFile.stem().c_str());
    if (seed) {
        dataJson["seed"] = *seed;
    }
    dataJson["test_name"] = testNameOnly.c_str();
    dataJson["test_id"] = testId + 1;
    // TODO: Traces are disabled until we are able to escape illegal characters (e.g., '"').
    // dataJson["trace"] = getTrace(testSpec);
    dataJson["trace"] = inja::json::array();
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    LOG5("Protobuf backend: emitting testcase:" << std::setw(4) << dataJson);

    inja::render_to(protobufFile, testCase, dataJson);
    protobufFile.flush();
}

void Protobuf::outputTest(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                          float currentCoverage) {
    auto incrementedTestName = testName + "_" + std::to_string(testIdx);

    protobufFile = std::ofstream(incrementedTestName + ".proto");
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
