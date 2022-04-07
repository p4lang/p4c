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
        self.table_add(table="MyIC_ap", keys=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_tbl", keys=["11:22:33:44:55:66"], references=[0x10])

        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)


class ActionProfileTwoTablesSameInstancePSATest(P4EbpfTest):
    """
    ActionProfile extern instance can be shared between tables under some circumstances
    """
    p4_file_path = "p4testdata/action-profile2.p4"

    def runTest(self):
        self.table_add(table="MyIC_ap", keys=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_tbl", keys=["11:22:33:44:55:66"], references=[0x10])
        self.table_add(table="MyIC_tbl2", keys=["AA:BB:CC:DD:EE:FF"], references=[0x10])

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
        self.table_add(table="MyIC_tbl", keys=["0x112233445500/40"], references=[0x10])
        self.table_add(table="MyIC_ap", keys=[0x10], action=2, data=[0x1122])

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
        self.table_add(table="MyIC_ap", keys=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_ap", keys=[0x20], action=1, data=["AA:BB:CC:DD:EE:FF"])
        self.table_add(table="MyIC_tbl", keys=["11:22:33:44:55:66"], references=[0x10])
        self.table_add(table="MyIC_tbl", keys=["AA:BB:CC:DD:EE:FF"], references=[0x20])

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

        self.table_add(table="MyIC_ap", keys=[0x10], action=2, data=[0x1122])
        self.table_add(table="MyIC_tbl", keys=["11:22:33:44:55:66"], references=[0x10])

        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)
