#include "backends/p4tools/testgen/targets/bmv2/backend/protobuf/protobuf.h"

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

#include "backends/p4tools/common/lib/format_bin_hex.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/gmputil.h"
#include "lib/log.h"
#include "lib/null.h"
#include "nlohmann/json.hpp"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

using P4::ControlPlaneAPI::p4rt_id_t;
using P4::ControlPlaneAPI::P4RuntimeSymbolType;

Protobuf::Protobuf(cstring testName, boost::optional<unsigned int> seed = boost::none)
    : TF(testName, seed) {}

inja::json Protobuf::getTrace(const TestSpec* testSpec) {
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

/// @return the id allocated to the object through the @id annotation if any, or
/// boost::none.
static boost::optional<p4rt_id_t> getIdAnnotation(const IR::IAnnotated* node) {
    const auto* idAnnotation = node->getAnnotation("id");
    if (idAnnotation == nullptr) {
        return boost::none;
    }
    const auto* idConstant = idAnnotation->expr[0]->to<IR::Constant>();
    CHECK_NULL(idConstant);
    if (!idConstant->fitsUint()) {
        ::error(ErrorType::ERR_INVALID, "%1%: @id should be an unsigned integer", node);
        return boost::none;
    }
    return static_cast<p4rt_id_t>(idConstant->value);
}

/// @return the value of any P4 '@id' annotation @declaration may have, and
/// ensure that the value is correct with respect to the P4Runtime
/// specification. The name 'externalId' is in analogy with externalName().
static boost::optional<p4rt_id_t> externalId(const P4RuntimeSymbolType& type,
                                             const IR::IDeclaration* declaration) {
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

std::vector<std::pair<size_t, size_t>> Protobuf::getIgnoreMasks(const IR::Constant* mask) {
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

inja::json Protobuf::getControlPlane(const TestSpec* testSpec) {
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
        const auto* table = tblConfig->getTable();

        auto p4RuntimeId = externalId(P4RuntimeSymbolType::TABLE(), table);
        BUG_CHECK(p4RuntimeId, "Id not present for table %1%. Can not generate test.", table);
        tblJson["id"] = *p4RuntimeId;

        auto const* tblRules = tblConfig->getRules();
        tblJson["rules"] = inja::json::array();
        for (const auto& tblRules : *tblRules) {
            inja::json rule;
            auto const* matches = tblRules.getMatches();
            auto const* actionCall = tblRules.getActionCall();
            auto const* actionDecl = actionCall->getAction();
            auto const* actionArgs = actionCall->getArgs();
            rule["action_name"] = actionCall->getActionName().c_str();
            auto p4RuntimeId = externalId(P4RuntimeSymbolType::ACTION(), actionDecl);
            BUG_CHECK(p4RuntimeId, "Id not present for action %1%. Can not generate test.",
                      actionDecl);
            rule["action_id"] = *p4RuntimeId;
            auto j = getControlPlaneForTable(*matches, *actionArgs);
            rule["rules"] = std::move(j);
            tblJson["rules"].push_back(rule);
        }

        // Action Profile
        const auto* apObject = tblConfig->getProperty("action_profile", false);
        const ActionProfile* actionProfile = nullptr;
        const ActionSelector* actionSelector = nullptr;
        if (apObject != nullptr) {
            actionProfile = apObject->checkedTo<ActionProfile>();
            // Check if we have an Action Selector too.
            // TODO: Change this to check in ActionSelector with table
            // property "action_selectors".
            const auto* asObject = tblConfig->getProperty("action_selector", false);
            if (asObject != nullptr) {
                actionSelector = asObject->checkedTo<ActionSelector>();
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
        const auto* const actionProfile = testObject.second->checkedTo<ActionProfile>();
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

inja::json Protobuf::getControlPlaneForTable(const std::map<cstring, const FieldMatch>& matches,
                                             const std::vector<ActionArg>& args) {
    inja::json rulesJson;

    rulesJson["single_exact_matches"] = inja::json::array();
    rulesJson["multiple_exact_matches"] = inja::json::array();
    rulesJson["range_matches"] = inja::json::array();
    rulesJson["ternary_matches"] = inja::json::array();
    rulesJson["lpm_matches"] = inja::json::array();

    rulesJson["act_args"] = inja::json::array();

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
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["single_exact_matches"].push_back(j);
            }
            void operator()(const Range& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["lo"] = formatHexExpr(elem.getEvaluatedLow());
                j["hi"] = formatHexExpr(elem.getEvaluatedHigh());
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["range_matches"].push_back(j);
            }
            void operator()(const Ternary& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExpr(elem.getEvaluatedValue());
                j["mask"] = formatHexExpr(elem.getEvaluatedMask());
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["ternary_matches"].push_back(j);
            }
            void operator()(const LPM& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExpr(elem.getEvaluatedValue());
                j["prefix_len"] = elem.getEvaluatedPrefixLength()->value.str();
                auto p4RuntimeId = getIdAnnotation(elem.getKey());
                BUG_CHECK(p4RuntimeId, "Id not present for key. Can not generate test.");
                j["id"] = *p4RuntimeId;
                rulesJson["lpm_matches"].push_back(j);
            }
        };

        boost::apply_visitor(GetRange(rulesJson, fieldName), fieldMatch);
    }

    for (const auto& actArg : args) {
        inja::json j;
        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatHexExpr(actArg.getEvaluatedValue());
        auto p4RuntimeId = getIdAnnotation(actArg.getActionParam());
        BUG_CHECK(p4RuntimeId, "Id not present for parameter. Can not generate test.");
        j["id"] = *p4RuntimeId;
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json Protobuf::getSend(const TestSpec* testSpec) {
    const auto* iPacket = testSpec->getIngressPacket();
    const auto* payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload);
    sendJson["pkt"] = dataStr;
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json Protobuf::getVerify(const TestSpec* testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != boost::none) {
        const auto& packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto* payload = packet.getEvaluatedPayload();
        const auto* payloadMask = packet.getEvaluatedPayloadMask();
        verifyData["ignore_mask"] = formatHexExpr(payloadMask);
        verifyData["exp_pkt"] = formatHexExpr(payload);
    }
    return verifyData;
}

static std::string getPreamble() {
    static const std::string PREAMBLE(
        R"""(// P4Testgen Protobuf template.
syntax = "proto3";

package p4testgen;

import "third_party/p4lang_p4runtime/proto/p4/v1/p4runtime.proto";

message PacketAtPort {
  // The raw bytes of the test packet.
  bytes packet = 1;
  // The raw bytes of the port associated with the packet.
  bytes port = 2;
  // The don't care mask of the packet. Not used for input packets.
  bytes packet_mask = 3;
}

message TestCase {
  // The input packet.
  PacketAtPort input_packet = 1;
  // The corresponding expected output packet.
  repeated PacketAtPort expected_output_packet = 2;
  // The entities (e.g., table entries) to install on the switch before
  // injecting the `input_packet`.
  repeated p4.v1.Entity entities = 3;
  // The trace associated with this particular test.
  repeated string traces = 4;
  // Additional metadata and information.
  repeated string metadata = 5;
}
)""");
    return PREAMBLE;
}

void Protobuf::emitPreamble(const std::string& preamble) {
    boost::filesystem::path testFile(testName);
    auto parent = testFile.parent_path();
    auto protoFile = parent /= "p4testgen.proto";
    auto outStream = std::ofstream(protoFile.c_str());
    inja::json dataJson;
    inja::render_to(outStream, preamble, dataJson);
    outStream.flush();
}

static std::string getTestCase() {
    static std::string TEST_CASE(
        R"""(# A P4TestGen-generated test case for {{test_name}}.p4
metadata: "p4testgen seed: '{{ default(seed, '"none"') }}'"
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
  port: "{{send.ig_port}}"
  packet_mask: "{{send.pkt}}"
}

## if verify
expected_output_packet {
  packet: "{{verify.exp_pkt}}"
  port: "{{verify.eg_port}}"
  packet_mask: "{{verify.ignore_mask}}"
}
## endif

## if control_plane
entities {
## for table in control_plane.tables
## for rule in table.rules
  # Table {{table.table_name}}
  table_entry {
    table_id: {{table.id}}
## for r in rule.rules.single_exact_matches
    # Match field {{r.field_name}}
    match {
      field_id: {{r.id}}
      exact {
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
        prefix_len: "{{r.prefix_len}}"
      }
    }
## endfor
    # Action {{rule.action_name}}
    action {
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
## endfor
  }
## endfor
}
## endif
)""");
    return TEST_CASE;
}

void Protobuf::emitTestcase(const TestSpec* testSpec, cstring selectedBranches, size_t testId,
                            const std::string& testCase, float currentCoverage) {
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
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = TestgenUtils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    LOG5("Protobuf backend: emitting testcase:" << std::setw(4) << dataJson);

    inja::render_to(protobufFile, testCase, dataJson);
    protobufFile.flush();
}

void Protobuf::outputTest(const TestSpec* testSpec, cstring selectedBranches, size_t testIdx,
                          float currentCoverage) {
    if (!preambleEmitted) {
        auto preamble = getPreamble();
        emitPreamble(preamble);
        preambleEmitted = true;
    }
    auto incrementedTestName = testName + "_" + std::to_string(testIdx);

    protobufFile = std::ofstream(incrementedTestName + ".proto");
    std::string testCase = getTestCase();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
