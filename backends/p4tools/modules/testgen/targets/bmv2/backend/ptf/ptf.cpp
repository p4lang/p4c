#include "backends/p4tools/modules/testgen/targets/bmv2/backend/ptf/ptf.h"

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

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

PTF::PTF(cstring testName, boost::optional<unsigned int> seed = boost::none) : TF(testName, seed) {
    boost::filesystem::path testFile(testName + ".py");
    cstring testNameOnly(testFile.stem().c_str());
}

inja::json::array_t PTF::getClone(const std::map<cstring, const TestObject *> &cloneInfos) {
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

std::vector<std::pair<size_t, size_t>> PTF::getIgnoreMasks(const IR::Constant *mask) {
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

inja::json PTF::getControlPlane(const TestSpec *testSpec) {
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

inja::json PTF::getControlPlaneForTable(const TableMatchMap &matches,
                                        const std::vector<ActionArg> &args) {
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

inja::json PTF::getSend(const TestSpec *testSpec) {
    const auto *iPacket = testSpec->getIngressPacket();
    const auto *payload = iPacket->getEvaluatedPayload();
    inja::json sendJson;
    sendJson["ig_port"] = iPacket->getPort();
    auto dataStr = formatHexExpr(payload, false, true, false);
    sendJson["pkt"] = insertHexSeparators(dataStr);
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json PTF::getVerify(const TestSpec *testSpec) {
    inja::json verifyData = inja::json::object();
    if (testSpec->getEgressPacket() != boost::none) {
        const auto &packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto *payload = packet.getEvaluatedPayload();
        const auto *payloadMask = packet.getEvaluatedPayloadMask();
        verifyData["ignore_masks"] = getIgnoreMasks(payloadMask);

        auto dataStr = formatHexExpr(payload, false, true, false);
        verifyData["exp_pkt"] = insertHexSeparators(dataStr);
    }
    return verifyData;
}

static std::string getPreamble() {
    static const std::string PREAMBLE(
        R"""(# P4Runtime PTF test for {{test_name}}
# p4testgen seed: {{ default(seed, "none") }}

import logging
import sys
import os

from ptf.mask import Mask

from ptf.packet import *
from ptf import testutils as ptfutils


import base_test as bt

class AbstractTest(bt.P4RuntimeTest):

    @bt.autocleanup
    def setUp(self):
        bt.P4RuntimeTest.setUp(self)
        success = bt.P4RuntimeTest.updateConfig(self)
        assert success

    def tearDown(self):
        bt.P4RuntimeTest.tearDown(self)

    def setupCtrlPlane(self):
        pass

    def sendPacket(self):
        pass

    def verifyPackets(self):
        pass

    def runTestImpl(self):
        self.setupCtrlPlane()
        bt.testutils.log.info("Sending Packet ...")
        self.sendPacket()
        bt.testutils.log.info("Verifying Packet ...")
        self.verifyPackets()


)""");
    return PREAMBLE;
}

void PTF::emitPreamble(const std::string &preamble) {
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

std::string PTF::getTestCaseTemplate() {
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
        # Simple noop that is always called as filler.
        pass
## if control_plane
## for table in control_plane.tables
## for rule in table.rules
        self.table_add(
            ('{{table.table_name}}',
            [
## for r in rule.rules.single_exact_matches
                self.Exact('{{r.field_name}}', {{r.value}}),
## endfor
## for r in rule.rules.optional_matches
                self.Optional('{{r.field_name}}', {{r.value}}, {{r.use_exact}}),
## endfor
## for r in rule.rules.range_matches
                self.Range('{{r.field_name}}', {{r.lo}}, {{r.hi}}),
## endfor
## for r in rule.rules.ternary_matches
                self.Ternary('{{r.field_name}}', {{r.value}}, {{r.mask}}),
## endfor
## for r in rule.rules.lpm_matches
                self.Lpm('{{r.field_name}}', {{r.value}}, {{r.prefix_len}}),
## endfor
            ]),
            ('{{rule.action_name}}',
            [
## for act_param in rule.rules.act_args
                ('{{act_param.param}}', {{act_param.value}}),
## endfor
            ])
            , {% if rule.rules.needs_priority %}{{rule.priority}}{% else %}None{% endif %}
            {% if existsIn(table, "has_ap") %}, {"oneshot": True}{% endif %}
        )
## endfor
## endfor
## endif
## if exists("clone_infos")
## for clone_info in clone_infos
        self.insert_pre_clone_session({{clone_info.session_id}}, [{{clone_info.clone_port}}])
## endfor
## endif

    def sendPacket(self):
## if send
        ig_port = {{send.ig_port}}
        pkt = b'{{send.pkt}}'
        ptfutils.send_packet(self, ig_port, pkt)
## else
        pass
## endif

    def verifyPackets(self):
## if verify
        eg_port = {{verify.eg_port}}
        exp_pkt = Mask(b'{{verify.exp_pkt}}')
## for ignore_mask in verify.ignore_masks
        exp_pkt.set_do_not_care({{ignore_mask.0}}, {{ignore_mask.1}})
## endfor
## if exists("clone_infos")
## for clone_info in clone_infos
## if clone_info.cloned
        ptfutils.verify_packet(self, exp_pkt, {{clone_info.clone_port}})
## else
        ptfutils.verify_packet(self, exp_pkt, eg_port)
##endfor
##endif
## else 
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=2)
## endif
## else
        pass
## endif

    def runTest(self):
        self.runTestImpl()

)""");
    return TEST_CASE;
}

void PTF::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                       const std::string &testCase, float currentCoverage) {
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

    // Check whether this test has a clone configuration.
    // These are special because they require additional instrumentation and produce two output
    // packets.
    auto cloneInfos = testSpec->getTestObjectCategory("clone_infos");
    if (!cloneInfos.empty()) {
        dataJson["clone_infos"] = getClone(cloneInfos);
    }

    LOG5("PTF backend: emitting testcase:" << std::setw(4) << dataJson);

    boost::filesystem::path testFile(testName + ".py");
    cstring testNameOnly(testFile.stem().c_str());
    inja::render_to(ptfFile, testCase, dataJson);
    ptfFile.flush();
}

void PTF::outputTest(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                     float currentCoverage) {
    if (!preambleEmitted) {
        ptfFile = std::ofstream(testName + ".py");
        std::string preamble = getPreamble();
        emitPreamble(preamble);
        preambleEmitted = true;
    }
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Bmv2
