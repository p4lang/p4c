/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include "backends/p4tools/modules/testgen/targets/tofino/test_backend/ptf.h"

#include <ir/irutils.h>

#include <iomanip>
#include <map>
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
#include "backends/p4tools/modules/testgen/lib/test_backend_configuration.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_spec.h"

namespace P4::P4Tools::P4Testgen::Tofino {

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

    auto tables = testSpec->getTestObjectCategory("tables"_cs);
    if (!tables.empty()) {
        controlPlaneJson["tables"] = inja::json::array();
    }
    for (auto const &testObject : tables) {
        inja::json tblJson;
        tblJson["table_name"] = testObject.first.c_str();
        const auto *const tblConfig = testObject.second->checkedTo<TableConfig>();
        auto const *tblRules = tblConfig->getRules();
        tblJson["rules"] = inja::json::array();
        for (const auto &tblRules : *tblRules) {
            inja::json rule;
            auto const *matches = tblRules.getMatches();
            auto const *actionCall = tblRules.getActionCall();
            auto const *actionArgs = actionCall->getArgs();
            rule["action_name"] = actionCall->getActionName().c_str();
            auto j = getControlPlaneForTable(*matches, *actionArgs);
            rule["rules"] = std::move(j);
            tblJson["rules"].push_back(rule);
        }
        // Collect action profiles and selectors associated with the table.
        checkForTableActionProfile<TofinoActionProfile, TofinoActionSelector>(tblJson, apAsMap,
                                                                              tblConfig);

        // Check whether the default action is overridden for this table.
        checkForDefaultActionOverride(tblJson, tblConfig);

        controlPlaneJson["tables"].push_back(tblJson);
    }

    // Collect declarations of action profiles.
    collectActionProfileDeclarations<TofinoActionProfile>(testSpec, controlPlaneJson, apAsMap);

    auto registerParams = testSpec->getTestObjectCategory("registerparams"_cs);
    if (!registerParams.empty()) {
        controlPlaneJson["register_params"] = inja::json::array();
    }
    for (auto const &[objName, obj] : registerParams) {
        const auto *reg = obj->checkedTo<TofinoRegisterParam>();
        inja::json r;
        auto registerParamName = reg->getRegisterParamDeclaration()->controlPlaneName();
        r["name"] = registerParamName;
        r["value"] = formatHexExpr(reg->getEvaluatedInitialValue());
        controlPlaneJson["register_params"].push_back(r);
    }

    auto registers = testSpec->getTestObjectCategory("registervalues"_cs);
    if (!registers.empty()) {
        controlPlaneJson["registers"] = inja::json::array();
    }
    for (auto const &[objName, obj] : registers) {
        const auto *reg = obj->checkedTo<TofinoRegisterValue>();
        inja::json r;
        auto registerName = reg->getRegisterDeclaration()->controlPlaneName();
        r["name"] = registerName;
        r["index"] = reg->getEvaluatedInitialIndex()->value.str();
        const auto *evaluatedVal = reg->getEvaluatedInitialValue();
        if (const auto *structExpr = evaluatedVal->to<IR::StructExpression>()) {
            r["init_val_list"] = inja::json::array();
            for (const auto *structElem : structExpr->components) {
                inja::json structElemJson = inja::json::object();
                structElemJson["name"] = registerName + "." + structElem->name;
                structElemJson["val"] = formatHexExpr(structElem->expression);
                r["init_val_list"].push_back(structElemJson);
            }
        } else if (const auto *constantVal = evaluatedVal->to<IR::Constant>()) {
            r["init_val"] = formatHexExpr(constantVal);
        } else {
            P4C_UNIMPLEMENTED("Unsupported initial register value %1% of type %2%", evaluatedVal,
                              evaluatedVal->node_type_name());
        }
        controlPlaneJson["registers"].push_back(r);
    }

    auto directRegisters = testSpec->getTestObjectCategory("direct_registervalues"_cs);
    if (!directRegisters.empty()) {
        controlPlaneJson["direct_registers"] = inja::json::array();
    }
    for (auto const &[objName, obj] : directRegisters) {
        const auto *reg = obj->checkedTo<TofinoDirectRegisterValue>();
        inja::json r;
        auto registerName = reg->getRegisterDeclaration()->controlPlaneName();
        r["name"] = registerName;
        r["table"] = reg->getRegisterTable()->controlPlaneName();
        const auto *evaluatedVal = reg->getEvaluatedInitialValue();
        if (const auto *structExpr = evaluatedVal->to<IR::StructExpression>()) {
            r["init_val_list"] = inja::json::array();
            for (const auto *structElem : structExpr->components) {
                inja::json structElemJson = inja::json::object();
                structElemJson["name"] = registerName + "." + structElem->name;
                structElemJson["val"] = formatHexExpr(structElem->expression);
                r["init_val_list"].push_back(structElemJson);
            }
        } else if (const auto *constantVal = evaluatedVal->to<IR::Constant>()) {
            r["init_val"] = formatHexExpr(constantVal);
        } else {
            P4C_UNIMPLEMENTED("Unsupported initial register value %1% of type %2%", evaluatedVal,
                              evaluatedVal->node_type_name());
        }
        controlPlaneJson["direct_registers"].push_back(r);
    }

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

    rulesJson["act_args"] = inja::json::array();

    for (auto const &match : matches) {
        auto const fieldName = match.first;
        auto const &fieldMatch = match.second;

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
    if (testSpec->getEgressPacket() != std::nullopt) {
        const auto &packet = **testSpec->getEgressPacket();
        verifyData["eg_port"] = packet.getPort();
        const auto *payload = packet.getEvaluatedPayload();
        const auto *payloadMask = packet.getEvaluatedPayloadMask();
        verifyData["ignore_masks"] = getIgnoreMasks(payloadMask);

        auto dataStr = formatHexExpr(payload, {false, true, false});
        verifyData["exp_pkt"] = insertHexSeparators(dataStr);
    }
    return verifyData;
}

void PTF::emitPreamble() {
    static const std::string PREAMBLE(
        R"""(# PTF test for {{test_name}}
# p4testgen seed: {{ default(seed, "none") }}

import logging
import itertools

from bfruntime_client_base_tests import BfRuntimeTest
from ptf.mask import Mask
from ptf.testutils import send_packet
from ptf.testutils import verify_packet
from ptf.testutils import verify_no_other_packets

import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc

logger = logging.getLogger('{{test_name}}')
logger.addHandler(logging.StreamHandler())

class AbstractTest(BfRuntimeTest):
    def setUp(self):
        BfRuntimeTest.setUp(self, 0, '{{test_name}}')
        self.dev_id = 0
        self.table_entries = []
        self.bfrt_info = None
        # Get bfrt_info and set it as part of the test.
        self.bfrt_info = self.interface.bfrt_info_get('{{test_name}}')

        # Set target to all pipes on device self.dev_id.
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)

    def tearDown(self):
        # Reset tables.
        for elt in reversed(self.table_entries):
            test_table = self.bfrt_info.table_get(elt[0])
            test_table.entry_del(self.target, elt[1])
        self.table_entries = []

        # End session.
        BfRuntimeTest.tearDown(self)

    def insertTableEntry(
        self, table_name, key_fields=None, action_name=None, data_fields=[]
    ):
        test_table = self.bfrt_info.table_get(table_name)
        key_list = [test_table.make_key(key_fields)]
        data_list = [test_table.make_data(data_fields, action_name)]
        test_table.entry_add(self.target, key_list, data_list)
        self.table_entries.append((table_name, key_list))

    def _responseDumpHelper(self, request):
        for response in self.interface.stub.Read(request, timeout=2):
            yield response

    def overrideDefaultEntry(self, table_name, action_name=None, data_fields=[]):
        test_table = self.bfrt_info.table_get(table_name)
        data = test_table.make_data(data_fields, action_name)
        test_table.default_entry_set(self.target, data)

    def setRegisterValue(self, reg_name, value, index):
        reg_table = self.bfrt_info.table_get(reg_name)
        key_list = [reg_table.make_key([gc.KeyTuple("$REGISTER_INDEX", index)])]
        value_list = []
        if isinstance(value, list):
            for val in value:
                value_list.append(gc.DataTuple(val[0], val[1]))
        else:
            value_list.append(gc.DataTuple("f1", value))
        reg_table.entry_add(self.target, key_list, [reg_table.make_data(value_list)])

    def entryAdd(self, table_obj, target, table_entry):
        req = bfruntime_pb2.WriteRequest()
        gc._cpy_target(req, target)
        req.atomicity = bfruntime_pb2.WriteRequest.CONTINUE_ON_ERROR
        update = req.updates.add()
        update.type = bfruntime_pb2.Update.MODIFY
        update.entity.table_entry.CopyFrom(table_entry)
        resp = self.interface.reader_writer_interface._write(req)
        table_obj.get_parser._parse_entry_write_response(resp)

    def setDirectRegisterValue(self, tbl_name, value):
        test_table = self.bfrt_info.table_get(tbl_name)
        table_id = test_table.info.id
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id
        gc._cpy_target(req, self.target)
        entity = req.entities.add()
        table = entity.table_entry
        table.table_id = table_id
        table_entry = None
        for response in self._responseDumpHelper(req):
            for entity in response.entities:
                assert entity.WhichOneof("entity") == "table_entry"
                table_entry = entity.table_entry
                break
        if table_entry is None:
            raise self.failureException(
                "No entry in the table that the meter is attached to."
            )
        table_entry.ClearField("data")
        value_list = []
        if isinstance(value, list):
            for val in value:
                df = table_entry.data.fields.add()
        else:
            df = table_entry.data.fields.add()
            df.value = gc.DataTuple(gc.DataTuple("f1", value))
        self.entryAdd(test_table, self.target, table_entry)

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
    inja::json dataJson;
    dataJson["test_name"] = getTestBackendConfiguration().testBaseName;
    auto optSeed = getTestBackendConfiguration().seed;
    if (optSeed.has_value()) {
        dataJson["seed"] = optSeed.value();
    }

    inja::render_to(ptfFileStream, PREAMBLE, dataJson);
    ptfFileStream.flush();
}

std::string PTF::getTestCaseTemplate() {
    static const std::string TEST_CASE(
        R"""(
class Test{{test_id}}(AbstractTest):
    # Date generated: {{timestamp}}
## if length(selected_branches) > 0
    # {{selected_branches}}
## endif
    # Current statement coverage: {{coverage}}
    '''
## for trace_item in trace
    {{trace_item}}
##endfor
    '''

    def setupCtrlPlane(self):
## if control_plane
## if existsIn(control_plane, "action_profiles")
## for ap in control_plane.action_profiles
## for action in ap.actions
        self.insertTableEntry(
            '{{ap.profile}}',
            [
                gc.KeyTuple('$ACTION_MEMBER_ID', {{action.action_idx}}),
            ],
            '{{action.action_name}}',
            [
## for act_param in action.act_args
                gc.DataTuple('{{act_param.param}}', {{act_param.value}}),
## endfor
            ]
        )
## endfor
## endfor
## endif
## if existsIn(control_plane, "register_params")
## for reg in control_plane.register_params
        reg_param = self.bfrt_info.table_get('{{reg.name}}')
        reg_param.default_entry_set(
                            self.target,
                            reg_param.make_data([gc.DataTuple('value', {{reg.value}})])
                            )
## endfor
## endif
## if existsIn(control_plane, "registers")
## for reg in control_plane.registers
## if existsIn(reg, "init_val_list")
        self.setRegisterValue('{{reg.name}}', [{% for r in reg.init_val_list %}('{{r.name}}', {{r.val}}){% if not loop.is_last %}, {% endif %}{% endfor %}], {{reg.index}})
## else if existsIn(reg, "init_val")
        self.setRegisterValue('{{reg.name}}', {{reg.init_val}}, {{reg.index}})
## endif
## endfor
## endif
## if existsIn(control_plane, "direct_registers")
## for reg in control_plane.direct_registers
## if existsIn(reg, "init_val_list")
        self.setDirectRegisterValue('{{reg.table}}', [{% for r in reg.init_val_list %}('{{r.name}}', {{r.val}}){% if not loop.is_last %}, {% endif %}{% endfor %}])
## else if existsIn(reg, "init_val")
        self.setDirectRegisterValue('{{reg.table}}', {{reg.init_val}})
## endif
## endfor
## endif
## if existsIn(control_plane, "tables")
## for table in control_plane.tables
## if existsIn(table, "default_override")
        # Table {{table.table_name}}
        self.overrideDefaultEntry(
            '{{table.table_name}}',
            '{{table.default_override.action_name}}',
            [
## for act_param in table.default_override.act_args
                gc.DataTuple('{{act_param.param}}', {{act_param.value}}),
## endfor
            ]
        )
## else if existsIn(table, "has_ap")
## for rule in table.rules
        self.insertTableEntry(
            '{{table.table_name}}',
            [
## for r in rule.rules.single_exact_matches
                gc.KeyTuple('{{r.field_name}}', {{r.value}}),
## endfor
## for r in rule.rules.range_matches
                gc.KeyTuple('{{r.field_name}}', low={{r.lo}}, high={{r.hi}}),
## endfor
## for r in rule.rules.ternary_matches
                gc.KeyTuple('{{r.field_name}}', {{r.value}}, {{r.mask}}),
## endfor
## for r in rule.rules.lpm_matches
                gc.KeyTuple('{{r.field_name}}', {{r.value}}, prefix_len={{r.prefix_len}}),
## endfor
            ],
            None,
            [
                gc.DataTuple('$ACTION_MEMBER_ID', {{rule.action_name}}),
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
                gc.KeyTuple('{{r.field_name}}', {{r.value}}),
## endfor
## for r in rule.rules.range_matches
                gc.KeyTuple('{{r.field_name}}', low={{r.lo}}, high={{r.hi}}),
## endfor
## for r in rule.rules.ternary_matches
                gc.KeyTuple('{{r.field_name}}', {{r.value}}, {{r.mask}}),
## endfor
## for r in rule.rules.lpm_matches
                gc.KeyTuple('{{r.field_name}}', {{r.value}}, prefix_len={{r.prefix_len}}),
## endfor
            ],
            '{{rule.action_name}}',
            [
## for act_param in rule.rules.act_args
                gc.DataTuple('{{act_param.param}}', {{act_param.value}}),
## endfor
            ]
        )
## endfor
## endif
## endfor
## endif
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

void PTF::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                       const std::string &testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }

    dataJson["test_id"] = testIdx;
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

void PTF::writeTestToFile(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                          float currentCoverage) {
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testIdx, testCase, currentCoverage);
}

}  // namespace P4::P4Tools::P4Testgen::Tofino
