#include "backends/p4tools/testgen/targets/bmv2/backend/ptf/ptf.h"

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
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "p4tools/testgen/lib/tf.h"
#include "p4tools/testgen/targets/bmv2/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

PTF::PTF(cstring testName, boost::optional<unsigned int> seed = boost::none) : TF(testName, seed) {
    boost::filesystem::path testFile(testName + ".py");
    cstring testNameOnly(testFile.stem().c_str());
}

inja::json PTF::getTrace(const TestSpec* testSpec) {
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

std::vector<std::pair<size_t, size_t>> PTF::getIgnoreMasks(const IR::Constant* mask) {
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

inja::json PTF::getControlPlane(const TestSpec* testSpec) {
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
        for (const auto& tblRules : *tblRules) {
            inja::json rule;
            auto const* matches = tblRules.getMatches();
            auto const* actionCall = tblRules.getActionCall();
            auto const* actionArgs = actionCall->getArgs();
            rule["action_name"] = actionCall->getActionName().c_str();
            auto j = getControlPlaneForTable(*matches, *actionArgs);
            rule["rules"] = std::move(j);
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

inja::json PTF::getControlPlaneForTable(const std::map<cstring, const FieldMatch>& matches,
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
                j["value"] = formatHexExpr(elem.getEvaluatedValue()).c_str();
                rulesJson["single_exact_matches"].push_back(j);
            }
            void operator()(const Range& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["lo"] = formatHexExpr(elem.getEvaluatedLow()).c_str();
                j["hi"] = formatHexExpr(elem.getEvaluatedHigh()).c_str();
                rulesJson["range_matches"].push_back(j);
            }
            void operator()(const Ternary& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExpr(elem.getEvaluatedValue()).c_str();
                j["mask"] = formatHexExpr(elem.getEvaluatedMask()).c_str();
                rulesJson["ternary_matches"].push_back(j);
            }
            void operator()(const LPM& elem) const {
                inja::json j;
                j["field_name"] = fieldName;
                j["value"] = formatHexExpr(elem.getEvaluatedValue()).c_str();
                j["prefix_len"] = elem.getEvaluatedPrefixLength()->value.str();
                rulesJson["lpm_matches"].push_back(j);
            }
        };

        boost::apply_visitor(GetRange(rulesJson, fieldName), fieldMatch);
    }

    for (const auto& actArg : args) {
        inja::json j;
        j["param"] = actArg.getActionParamName().c_str();
        j["value"] = formatHexExpr(actArg.getEvaluatedValue()).c_str();
        rulesJson["act_args"].push_back(j);
    }

    return rulesJson;
}

inja::json PTF::getSend(const TestSpec* testSpec) {
    const auto* iPacket = testSpec->getIngressPacket();
    const auto* payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload, false, true, false);
    sendJson["pkt"] = insertHexSeparators(dataStr);
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json PTF::getVerify(const TestSpec* testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != boost::none) {
        const auto& packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto* payload = packet.getEvaluatedPayload();
        const auto* payloadMask = packet.getEvaluatedPayloadMask();
        verifyData["ignore_masks"] = getIgnoreMasks(payloadMask);

        auto dataStr = formatHexExpr(payload, false, true, false);
        verifyData["exp_pkt"] = insertHexSeparators(dataStr);
    }
    return verifyData;
}

static std::string getPreamble() {
    static const std::string PREAMBLE(
        R"""(# P4Runtime PTF test for {{test_name}}
# p4testgen seed: '{{ default(seed, '"none"') }}'

import logging
import itertools

from ptf import config
from ptf.thriftutils import *
from ptf.mask import Mask
from ptf.testutils import send_packet
from ptf.testutils import verify_packet
from ptf.testutils import verify_no_other_packets
from ptf.packet import *

from p4.v1 import p4runtime_pb2
from p4runtime_base_tests import P4RuntimeTest, autocleanup, stringify, ipv4_to_binary, mac_to_binary

logger = logging.getLogger('{{test_name}}')
logger.addHandler(logging.StreamHandler())

class AbstractTest(P4RuntimeTest):
    @autocleanup
    def setUp(self):
        super(AbstractTest, self).setUp()
        self.req = p4runtime_pb2.WriteRequest()
        self.req.device_id = self.device_id

    def tearDown(self):
        # TODO: Figure this out
        pass

    def insertTableEntry(self, table_name, key_fields = None,
            action_name = None, data_fields = []):
        self.push_update_add_entry_to_action(
            self.req, table_name, key_fields, action_name, data_fields)
        self.write_request(req)

    def setupCtrlPlane(self):
        pass

    def sendPacket(self):
        pass

    def verifyPackets(self):
        pass

    def runTestImpl(self):
        self.setupCtrlPlane()
        logger.info("Sending Packet ...")
        self.sendPacket()
        logger.info("Verifying Packet ...")
        self.verifyPackets()
        logger.info("Verifying no other packets ...")
        verify_no_other_packets(self, self.dev_id, timeout=2)
)""");
    return PREAMBLE;
}

void PTF::emitPreamble(const std::string& preamble) {
    boost::filesystem::path testFile(testName + ".py");
    cstring testNameOnly(testFile.stem().c_str());
    inja::json dataJson;
    dataJson["test_name"] = testNameOnly.c_str();
    if (seed) {
        dataJson["seed"] = *seed;
    }

    inja::render_to(ptfFile, preamble, dataJson);
    ptfFile.flush();
}

static std::string getTestCase() {
    static std::string TEST_CASE(
        R"""(
class Test{{test_id}}(AbstractTest):
    # Date generated: {{timestamp}}
## if length(selected_branches) > 0
    # {{selected_branches}}
## endif
    '''
    # Current statement coverage: {{coverage}}
## for trace_item in trace
    {{trace_item}}
##endfor
    '''

    def setupCtrlPlane(self):
## if control_plane
## if existsIn(control_plane, '"action_profiles"')
## for ap in control_plane.action_profiles
## for action in ap.actions
        self.insertTableEntry(
            '{{ap.profile}}',
            [
                gc.KeyTuple('$ACTION_MEMBER_ID', {{action.action_idx}}),
            ],
            '{{action.action_name}}',
            [
## for act_param in action.action_args
                gc.DataTuple('{{act_param.param}}', {{act_param.value}}),
## endfor
            ]
        )
## endfor
## endfor
## endif
## for table in control_plane.tables
## if existsIn(table, '"has_ap"')
## for rule in table.rules
        self.insertTableEntry(
            '{{table.table_name}}',
            [
## for r in rule.rules.single_exact_matches
                self.Exact('{{r.field_name}}', {{r.value}}),
## endfor
## for r in rule.rules.range_matches
                # TODO: p4Runtime doesn't have Range match - this would fail. Need to fix.
                self.Range('{{r.field_name}}', {{r.lo}}, {{r.hi}}),
## endfor
## for r in rule.rules.ternary_matches
                self.Ternary('{{r.field_name}}', {{r.value}}, {{r.mask}}),
## endfor
## for r in rule.rules.lpm_matches
                self.Lpm('{{r.field_name}}', {{r.value}}, {{r.prefix_len}}),
## endfor
            ],
            None,
            [
                ('$ACTION_MEMBER_ID', {{rule.action_name}}),
            ]
        )
## endfor
## else
        # Table {{table.table_name}}
## for rule in table.rules
        self.insertTableEntry(
            '{{table.table_name}}',
            [
## for r in rule.rules.single_exact_matches
                self.Exact('{{r.field_name}}', {{r.value}}),
## endfor
## for r in rule.rules.range_matches
                # TODO: p4Runtime doesn't have Range match - this would fail. Need to fix.
                self.Range('{{r.field_name}}', {{r.lo}}, {{r.hi}}),
## endfor
## for r in rule.rules.ternary_matches
                self.Ternary('{{r.field_name}}', {{r.value}}, {{r.mask}}),
## endfor
## for r in rule.rules.lpm_matches
                self.Lpm('{{r.field_name}}', {{r.value}}, {{r.prefix_len}}),
## endfor
            ],
            '{{rule.action_name}}',
            [
## for act_param in rule.rules.act_args
                ('{{act_param.param}}', {{act_param.value}}),
## endfor
            ]
        )
## endfor
## endif
## endfor
## else
        pass
## endif

    def sendPacket(self):
## if send
        ig_port = {{send.ig_port}}
        pkt = b'{{send.pkt}}'
        send_packet(self, ig_port, pkt)
## else
        pass
## endif

    def verifyPackets(self):
## if verify
        eg_port = {{verify.eg_port}}
        exp_pkt = b'{{verify.exp_pkt}}'
        exp_pkt = Mask(exp_pkt)
## for ignore_mask in verify.ignore_masks
        exp_pkt.set_do_not_care({{ignore_mask.0}}, {{ignore_mask.1}})
## endfor
        verify_packet(self, exp_pkt, eg_port)
## else
        pass
## endif

    def runTest(self):
        self.runTestImpl()

)""");
    return TEST_CASE;
}

void PTF::emitTestcase(const TestSpec* testSpec, cstring selectedBranches, size_t testId,
                       const std::string& testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
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

    LOG5("PTF backend: emitting testcase:" << std::setw(4) << dataJson);

    boost::filesystem::path testFile(testName + ".py");
    cstring testNameOnly(testFile.stem().c_str());
    inja::render_to(ptfFile, testCase, dataJson);
    ptfFile.flush();
}

void PTF::outputTest(const TestSpec* testSpec, cstring selectedBranches, size_t testIdx,
                     float currentCoverage) {
    if (!preambleEmitted) {
        ptfFile = std::ofstream(testName + ".py");
        std::string preamble = getPreamble();
        emitPreamble(preamble);
        preambleEmitted = true;
    }
    std::string testCase = getTestCase();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
