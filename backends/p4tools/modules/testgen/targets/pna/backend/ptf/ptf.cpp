#include "backends/p4tools/modules/testgen/targets/pna/backend/ptf/ptf.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

PTF::PTF(const TestBackendConfiguration &testBackendConfiguration)
    : TestFramework(testBackendConfiguration) {}

std::vector<std::pair<size_t, size_t>> PTF::getIgnoreMasks(const IR::Constant *mask) {
    std::vector<std::pair<size_t, size_t>> ignoreMasks;
    if (mask == nullptr) {
        return ignoreMasks;
    }
    auto maskBinStr = formatBinExpr(mask, {false, true, false});
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
        checkForTableActionProfile<PnaDpdkActionProfile, PnaDpdkActionSelector>(tblJson, apAsMap,
                                                                                tblConfig);

        // Check whether the default action is overridden for this table.
        checkForDefaultActionOverride(tblJson, tblConfig);

        controlPlaneJson["tables"].push_back(tblJson);
    }

    // Collect declarations of action profiles.
    collectActionProfileDeclarations<PnaDpdkActionProfile>(testSpec, controlPlaneJson, apAsMap);

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
    auto dataStr = formatHexExpr(payload, {false, true, false});
    sendJson["pkt"] = insertHexSeparators(dataStr);
    sendJson["pkt_size"] = payload->type->width_bits();
    return sendJson;
}

inja::json PTF::getVerify(const TestSpec *testSpec) {
    inja::json verifyData = inja::json::object();
    auto egressPacket = testSpec->getEgressPacket();
    if (egressPacket.has_value()) {
        const auto *packet = egressPacket.value();
        verifyData["eg_port"] = packet->getPort();
        const auto *payload = packet->getEvaluatedPayload();
        const auto *payloadMask = packet->getEvaluatedPayloadMask();
        verifyData["ignore_masks"] = getIgnoreMasks(payloadMask);

        auto dataStr = formatHexExpr(payload, {false, true, false});
        verifyData["exp_pkt"] = insertHexSeparators(dataStr);
    }
    return verifyData;
}

void PTF::emitPreamble() {
    static const std::string PREAMBLE(
        R"""(# P4Runtime PTF test for {{test_name}}
# p4testgen seed: {{ default(seed, "none") }}

from enum import Enum
from pathlib import Path

from ptf.mask import Mask

from ptf.packet import *
from ptf import testutils as ptfutils

# The base_test.py path is relative to the test file, this should be improved
FILE_DIR = Path(__file__).resolve().parent
BASE_TEST_PATH = FILE_DIR.joinpath("../../backends/dpdk/base_test.py")
sys.path.append(str(BASE_TEST_PATH))
import base_test as bt

# This global variable ensures that the forwarding pipeline will only be pushed once in one test.
PIPELINE_PUSHED = False

class AbstractTest(bt.P4RuntimeTest):
    EnumColor = Enum("EnumColor", ["GREEN", "YELLOW", "RED"], start=0)

    def setUp(self):
        bt.P4RuntimeTest.setUp(self)
        global PIPELINE_PUSHED
        if not PIPELINE_PUSHED:
            success = bt.P4RuntimeTest.updateConfig(self)
            assert success
            PIPELINE_PUSHED = True
        packet_wait_time = ptfutils.test_param_get("packet_wait_time")
        if not packet_wait_time:
            self.packet_wait_time = 0.1
        else:
            self.packet_wait_time = float(packet_wait_time)


    def tearDown(self):
        bt.P4RuntimeTest.tearDown(self)

    def setupCtrlPlane(self):
        pass

    def sendPacket(self):
        pass

    def verifyPackets(self):
        pass

    @bt.autocleanup
    def runTestImpl(self):
        self.setupCtrlPlane()
        bt.testutils.log.info("Sending Packet ...")
        self.sendPacket()
        bt.testutils.log.info("Verifying Packet ...")
        self.verifyPackets()
## if 0
    # This is a function copied from bmv2's ptf.cpp, could be deleted
    def meter_write_with_predefined_config(self, meter_name, index, value, direct):
        """Since we can not blast the target with packets, we have to carefully craft an artificial scenario where the meter will return the color we want. We do this by setting the meter config in such a way that the meter is forced to assign the desired color. For example, for RED to the lowest threshold values, to force a RED assignment."""
        value = self.EnumColor(value)
        if value == self.EnumColor.GREEN:
            meter_config = bt.p4runtime_pb2.MeterConfig(
                cir=4294967295, cburst=4294967295, pir=4294967295, pburst=4294967295
            )
        elif value == self.EnumColor.YELLOW:
            meter_config = bt.p4runtime_pb2.MeterConfig(
                cir=1, cburst=1, pir=4294967295, pburst=4294967295
            )
        elif value == self.EnumColor.RED:
            meter_config = bt.p4runtime_pb2.MeterConfig(
                cir=1, cburst=1, pir=1, pburst=1
            )
        else:
            raise self.failureException(f"Unsupported meter value {value}")
        if direct:
            meter_obj = self.get_obj("direct_meters", meter_name)
            table_id = meter_obj.direct_table_id
            req, _ = self.make_table_read_request_by_id(table_id)
            table_entry = None
            for response in self.response_dump_helper(req):
                for entity in response.entities:
                    assert entity.WhichOneof("entity") == "table_entry"
                    table_entry = entity.table_entry
                    break
            if table_entry is None:
                raise self.failureException(
                    "No entry in the table that the meter is attached to."
                )
            return self.direct_meter_write(meter_config, table_id, table_entry)
        return self.meter_write(meter_name, index, meter_config)
## endif
)""");

    inja::json dataJson;
    dataJson["test_name"] = getTestBackendConfiguration().testBaseName;
    auto optSeed = getTestBackendConfiguration().seed;
    if (optSeed.has_value()) {
        dataJson["seed"] = optSeed.value();
    }

    inja::render_to(ptfFileStream, PREAMBLE, dataJson);
    ptfFileStream.flush();
}

// Iteration of meter_values, from BMv2, is deleted
std::string PTF::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(
class Test{{test_id}}(AbstractTest):
    '''
    Date generated: {{timestamp}}
    Current node coverage: {{coverage}}
## if length(selected_branches) > 0
    Selected branches: {{selected_branches}}
## endif
    Trace:
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
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)
## else
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)
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

    dataJson["test_id"] = testId;
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getVerify(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    LOG5("PTF backend: emitting testcase:" << std::setw(4) << dataJson);

    if (!preambleEmitted) {
        BUG_CHECK(getTestBackendConfiguration().fileBasePath.has_value(), "Base path is not set.");
        auto ptfFile = getTestBackendConfiguration().fileBasePath.value();
        ptfFile.replace_extension(".py");
        ptfFileStream = std::ofstream(ptfFile);
        emitPreamble();
        preambleEmitted = true;
    }
    inja::render_to(ptfFileStream, testCase, dataJson);
    ptfFileStream.flush();
}

void PTF::writeTestToFile(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                          float currentCoverage) {
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testId, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Pna
