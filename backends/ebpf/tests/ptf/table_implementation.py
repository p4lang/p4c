#!/usr/bin/env python
# Copyright 2022-present Orange
# Copyright 2022-present Open Networking Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from common import *

from scapy.layers.l2 import Ether

PORT0 = 0
PORT1 = 1
PORT2 = 2
PORT3 = 3
PORT4 = 4
PORT5 = 5
ALL_PORTS = [PORT0, PORT1, PORT2, PORT3, PORT4, PORT5]


# ----------------------------- Action Profile -----------------------------


class SimpleActionProfilePSATest(P4EbpfTest):
    """
    Basic usage of ActionProfile extern
    """
    p4_file_path = "p4testdata/action-profile1.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_src="11:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        # FIXME: API/CLI for ActionProfile is not done yet, we are using table API/CLI for now
        self.table_add(table="MyIC_ap", key=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_tbl", key=["11:22:33:44:55:66"], references=[0x10])

        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)


class ActionProfileTwoTablesSameInstancePSATest(P4EbpfTest):
    """
    ActionProfile extern instance can be shared between tables under some circumstances
    """
    p4_file_path = "p4testdata/action-profile2.p4"

    def runTest(self):
        self.table_add(table="MyIC_ap", key=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_tbl", key=["11:22:33:44:55:66"], references=[0x10])
        self.table_add(table="MyIC_tbl2", key=["AA:BB:CC:DD:EE:FF"], references=[0x10])

        pkt = testutils.simple_ip_packet(eth_src="11:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt = testutils.simple_ip_packet(eth_src="AA:BB:CC:DD:EE:FF", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)


class ActionProfileLPMTablePSATest(P4EbpfTest):
    """
    LPM key match
    """
    p4_file_path = "p4testdata/action-profile-lpm.p4"

    def runTest(self):
        # Match all 11:22:33:44:55:xx MAC addresses
        self.table_add(table="MyIC_tbl", key=["0x112233445500/40"], references=[0x10])
        self.table_add(table="MyIC_ap", key=[0x10], action=2, data=[0x1122])

        pkt = testutils.simple_ip_packet(eth_src="AA:BB:CC:DD:EE:FF", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt[Ether].src = "11:22:33:44:55:66"
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)


# TODO: add test with ternary table


class ActionProfileActionRunPSATest(P4EbpfTest):
    """
    Test statement table.apply().action_run
    """
    p4_file_path = "p4testdata/action-profile-action-run.p4"

    def runTest(self):
        self.table_add(table="MyIC_ap", key=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_ap", key=[0x20], action=1, data=["AA:BB:CC:DD:EE:FF"])
        self.table_add(table="MyIC_tbl", key=["11:22:33:44:55:66"], references=[0x10])
        self.table_add(table="MyIC_tbl", key=["AA:BB:CC:DD:EE:FF"], references=[0x20])

        # action MyIC_a1
        pkt = testutils.simple_ip_packet(eth_src="AA:BB:CC:DD:EE:FF", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].dst = "AA:BB:CC:DD:EE:FF"
        testutils.verify_packet(self, pkt, PORT1)

        # action MyIC_a2
        pkt = testutils.simple_ip_packet(eth_src="11:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet(self, pkt, PORT2)


class ActionProfileHitPSATest(P4EbpfTest):
    """
    Test statement table.apply().hit
    """
    p4_file_path = "p4testdata/action-profile-hit.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_src="11:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        self.table_add(table="MyIC_ap", key=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_tbl", key=["11:22:33:44:55:66"], references=[0x10])

        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)


# ----------------------------- Action Selector -----------------------------


class ActionSelectorTest(P4EbpfTest):
    """
    Simple tools for manipulating ActionSelector
    """

    def create_actions(self, selector):
        for i in range(1, 7):
            # i: member reference; 3+i: output port
            ref = self.action_selector_add_action(selector, action=1, data=[i+3])
            if i != ref:
                self.fail("Invalid member reference: expected {}, got {}".format(i, ref))

    def group_add_members(self, selector, gid, member_refs):
        for m in member_refs:
            self.action_selector_add_member_to_group(selector=selector, group_ref=gid, member_ref=m)

    def create_default_rule_set(self, table, selector):
        self.create_actions(selector=selector)
        self.group_id = self.action_selector_create_empty_group(selector=selector)
        self.group_add_members(selector=selector, gid=self.group_id, member_refs=[4, 5, 6])
        self.default_group_ports = [PORT3, PORT4, PORT5]

        if table:
            self.table_add(table=table, key=["02:22:33:44:55:66"], references=["0x2"])
            self.table_add(table=table, key=["07:22:33:44:55:66"], references=["group {}".format(self.group_id)])


class SimpleActionSelectorPSATest(ActionSelectorTest):
    """
    Basic usage of ActionSelector: match action directly and from group
    """
    p4_file_path = "p4testdata/action-selector1.p4"

    def runTest(self):
        self.create_default_rule_set(table="MyIC_tbl", selector="MyIC_as")

        # member reference
        pkt = testutils.simple_ip_packet(eth_src="02:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # change selector key, result should be the same
        pkt[Ether].dst = "22:33:44:55:66:78"
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        # group reference
        output_ports = self.default_group_ports
        pkt = testutils.simple_ip_packet(eth_src="07:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        (port, _) = testutils.verify_packet_any_port(self, pkt, output_ports)
        # send again, output port should be the same
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, output_ports[port])
        # change selector key, output port should be changed
        pkt[Ether].dst = "22:33:44:55:66:78"
        output_ports.pop(port)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, output_ports)

        # TODO: test cache


class ActionSelectorTwoTablesSameInstancePSATest(ActionSelectorTest):
    """
    Two tables has different match keys and share the same ActionSelector (one selector key).
    Tests basic sharing of ActionSelector instance. For test purpose tables has also defined
    default empty group action "psa_empty_group_action" (not used).
    """
    p4_file_path = "p4testdata/action-selector2.p4"

    def runTest(self):
        self.create_default_rule_set(table="MyIC_tbl", selector="MyIC_as")
        self.table_add(table="MyIC_tbl2", key=[0x1122], references=[3])

        pkt = testutils.simple_ip_packet(eth_src="02:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        pkt[Ether].type = 0x1122
        pkt[Ether].src = "AA:BB:CC:DD:EE:FF"
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)


class ActionSelectorDefaultEmptyGroupActionPSATest(ActionSelectorTest):
    """
    Tests behaviour of default empty group action, aka table property "psa_empty_group_action".
    """
    p4_file_path = "p4testdata/action-selector3.p4"

    def runTest(self):
        # should be dropped (no group)
        pkt = testutils.simple_ip_packet(eth_src="08:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        gid = self.action_selector_create_empty_group(selector="MyIC_as")
        self.table_add(table="MyIC_tbl", key=["08:22:33:44:55:66"], references=["group {}".format(gid)])

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        cmd = "psabpf-ctl action-selector default_group_action pipe {} MyIC_as id 1 data 6".format(TEST_PIPELINE_ID)
        self.exec_ns_cmd(cmd, "default group action update failed")

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)


class ActionSelectorMultipleSelectorsPSATest(ActionSelectorTest):
    """
    Tests if multiple selectors are allowed and used.
    """
    p4_file_path = "p4testdata/action-selector4.p4"

    def runTest(self):
        self.create_default_rule_set(table="MyIC_tbl", selector="MyIC_as")
        self.table_add(table="MyIC_tbl", key=["07:22:33:44:55:67"], references=["group {}".format(self.group_id)])

        allowed_ports = self.default_group_ports
        pkt = testutils.simple_ip_packet(eth_src="07:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        (port, _) = testutils.verify_packet_any_port(self, pkt, allowed_ports)
        allowed_ports.pop(port)

        # change separately every selector key and test if output port has been changed
        pkt[Ether].src = "07:22:33:44:55:67"
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, allowed_ports)
        pkt[Ether].src = "07:22:33:44:55:66"

        pkt[Ether].dst = "22:33:44:55:66:78"
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, allowed_ports)
        pkt[Ether].dst = "22:33:44:55:66:77"

        pkt[Ether].type = 0x801
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, allowed_ports)


class ActionSelectorMultipleSelectorsTwoTablesPSATest(ActionSelectorTest):
    """
    Similar to ActionSelectorTwoTablesSameInstancePSATest, but tables has two selectors
    and the same match key. Tests order of selectors in both tables.
    """
    p4_file_path = "p4testdata/action-selector5.p4"

    def runTest(self):
        self.create_default_rule_set(table="MyIC_tbl", selector="MyIC_as")
        self.table_add(table="MyIC_tbl2", key=["AA:BB:CC:DD:EE:FF"], references=["group {}".format(self.group_id)])

        pkt = testutils.simple_ip_packet(eth_src="07:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        (port, _) = testutils.verify_packet_any_port(self, pkt, self.default_group_ports)
        # Match second table, same selectors set, so output port should be the same
        pkt[Ether].src = "AA:BB:CC:DD:EE:FF"
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, self.default_group_ports[port])


class ActionSelectorLPMTablePSATest(ActionSelectorTest):
    """
    Tests table with LPM match key.
    """
    p4_file_path = "p4testdata/action-selector-lpm.p4"

    def runTest(self):
        self.create_default_rule_set(table=None, selector="MyIC_as")
        # Match all 00:22:02:44:55:xx MAC addresses into action ref 2
        self.table_add(table="MyIC_tbl", key=["0x002202445500/40"], references=[2])
        # Match all 00:22:07:44:55:xx MAC addresses into group g7
        self.table_add(table="MyIC_tbl", key=["0x2207445500/40"], references=["group {}".format(self.group_id)])

        pkt = testutils.simple_ip_packet(eth_src="00:22:07:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, self.default_group_ports)

        pkt = testutils.simple_ip_packet(eth_src="00:22:02:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


# TODO: test ActionSelector with ternary table


class ActionSelectorActionRunPSATest(ActionSelectorTest):
    """
    Tests action_run statement on table apply().
    """
    p4_file_path = "p4testdata/action-selector-action-run.p4"

    def runTest(self):
        ref1 = self.action_selector_add_action(selector="MyIC_as", action=1, data=[5])
        ref2 = self.action_selector_add_action(selector="MyIC_as", action=0, data=[])
        self.table_add(table="MyIC_tbl", key=["02:22:33:44:55:66"], references=[ref1])
        self.table_add(table="MyIC_tbl", key=["03:22:33:44:55:66"], references=[ref2])

        pkt = testutils.simple_ip_packet(eth_src="02:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt[Ether].src = "03:22:33:44:55:66"
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)


class ActionSelectorHitPSATest(ActionSelectorTest):
    """
    Tests hit statement on table apply().
    """
    p4_file_path = "p4testdata/action-selector-hit.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_src="07:22:33:44:55:66", eth_dst="22:33:44:55:66:77")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        self.create_default_rule_set(table="MyIC_tbl", selector="MyIC_as")

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
