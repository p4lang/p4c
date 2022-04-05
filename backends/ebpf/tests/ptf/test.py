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

import copy

from scapy.fields import ShortField, IntField
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet, bind_layers, split_layers
from ptf.packet import MPLS
from ptf.mask import Mask

PORT0 = 0
PORT1 = 1
PORT2 = 2
ALL_PORTS = [PORT0, PORT1, PORT2]


class SimpleForwardingPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/simple-fwd.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        # use default action
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

    def tearDown(self):
        super(SimpleForwardingPSATest, self).tearDown()


class PSAResubmitTest(P4EbpfTest):

    p4_file_path = "p4testdata/resubmit.p4"

    def runTest(self):
        pkt = testutils.simple_eth_packet()
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].dst = "11:22:33:44:55:66"
        testutils.verify_packet(self, pkt, PORT1)


class SimpleTunnelingPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/psa-tunneling.p4"

    def runTest(self):
        pkt = Ether(dst="11:11:11:11:11:11") / testutils.simple_ip_only_packet(ip_dst="192.168.1.1")

        exp_pkt = Ether(dst="11:11:11:11:11:11") / MPLS(label=20, cos=5, s=1, ttl=64) / testutils.simple_ip_only_packet(
            ip_dst="192.168.1.1")

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, exp_pkt, PORT1)

        testutils.send_packet(self, PORT0, exp_pkt)
        testutils.verify_packet(self, pkt, PORT1)


class PSACloneI2E(P4EbpfTest):

    p4_file_path = "p4testdata/clone-i2e.p4"

    def runTest(self):
        # create clone session table
        self.clone_session_create(8)
        # add egress_port=6 (PORT2), instance=1 as clone session member, cos = 0
        self.clone_session_add_member(clone_session=8, egress_port=6)
        # add egress_port=6 (PORT2), instance=2 as clone session member, cos = 1
        self.clone_session_add_member(clone_session=8, egress_port=6, instance=2, cos=1)

        pkt = testutils.simple_eth_packet(eth_dst='00:00:00:00:00:05')
        testutils.send_packet(self, PORT0, pkt)
        cloned_pkt = copy.deepcopy(pkt)
        cloned_pkt[Ether].type = 0xface
        testutils.verify_packet(self, cloned_pkt, PORT2)
        testutils.verify_packet(self, cloned_pkt, PORT2)
        pkt[Ether].src = "00:00:00:00:ca:fe"
        testutils.verify_packet(self, pkt, PORT1)

        pkt = testutils.simple_eth_packet(eth_dst='00:00:00:00:00:09')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_packet(self, pkt, PORT1)

    def tearDown(self):
        self.clone_session_delete(8)
        super(PSACloneI2E, self).tearDown()


class EgressTrafficManagerDropPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/etm-drop.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:44:55', eth_src='55:44:33:22:11:00')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)
        pkt[Ether].src = '00:44:33:22:FF:FF'
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)


class EgressTrafficManagerClonePSATest(P4EbpfTest):
    """
    1. Send packet to interface PORT1 (bpf ifindex = 5) with destination MAC address equals to aa:bb:cc:dd:ee:ff.
    2. Observe that:
      2.1. Original packet was sent back through interface PORT1 (bpf ifindex = 5).
           The packet should have destination MAC address set to '00:00:00:00:00:12'.
      2.2. Packet was cloned at egress and processed by egress pipeline at interface PORT2 (bpf ifindex = 6).
           The cloned packet should have destination MAC address set to '00:00:00:00:00:11'.
    """
    p4_file_path = "p4testdata/etm-clone-e2e.p4"

    def runTest(self):
        # create clone session table
        self.clone_session_create(8)
        # add egress_port=6 (PORT2), instance=1 as clone session member, cos = 0
        self.clone_session_add_member(clone_session=8, egress_port=6)

        pkt = testutils.simple_ip_packet(eth_dst='aa:bb:cc:dd:ee:ff', eth_src='55:44:33:22:11:00')
        testutils.send_packet(self, PORT1, pkt)
        pkt[Ether].dst = '00:00:00:00:00:11'
        testutils.verify_packet(self, pkt, PORT2)
        pkt[Ether].dst = '00:00:00:00:00:12'
        testutils.verify_packet(self, pkt, PORT1)

    def tearDown(self):
        self.clone_session_delete(8)
        super(EgressTrafficManagerClonePSATest, self).tearDown()


@xdp2tc_head_not_supported
class EgressTrafficManagerRecirculatePSATest(P4EbpfTest):
    """
    Test resubmit packet path. eBPF program should do following operation:
    1. In NORMAL path: In all packet set source MAC to starts with '00:44'.
        Test if destination MAC address ends with 'FE:F0' - in this case recirculate.
    2. In RECIRCULATE path destination MAC set to zero.
    Any packet modification should be done on egress.
    Open question: how to verify here that the eBPF program did above operations?
    """
    p4_file_path = "p4testdata/etm-recirc.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:44:55', eth_src='55:44:33:22:11:00')
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].src = '00:44:33:22:11:00'
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:FE:F0', eth_src='55:44:33:22:11:00')
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].dst = '00:00:00:00:00:00'
        pkt[Ether].src = '00:44:33:22:11:00'
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)


class MulticastPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/psa-multicast.p4"

    def runTest(self):
        self.multicast_group_create(group=8)
        self.multicast_group_add_member(group=8, egress_port=5)
        self.multicast_group_add_member(group=8, egress_port=6)

        pkt = testutils.simple_eth_packet(eth_dst='00:00:00:00:00:05')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        pkt = testutils.simple_eth_packet(eth_dst='00:00:00:00:00:08')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        testutils.verify_packet(self, pkt, PORT2)
        testutils.verify_no_other_packets(self)

    def tearDown(self):
        self.multicast_group_delete(8)
        super(MulticastPSATest, self).tearDown()


class SimpleLpmP4PSATest(P4EbpfTest):

    p4_file_path = "p4testdata/psa-lpm.p4"

    def runTest(self):
        # This command adds LPM entry 10.10.0.0/16 with action forwarding on port 6 (PORT2 in ptf)
        self.table_add(table="ingress_tbl_fwd_lpm", keys=["10.10.0.0/16"], action=1, data=[6])
        self.table_add(table="ingress_tbl_fwd_lpm", keys=["10.10.10.10/8"], action=0)
        pkt = testutils.simple_ip_packet(ip_src='1.1.1.1', ip_dst='10.10.11.11')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)

        self.table_add(table="ingress_tbl_fwd_lpm", keys=["192.168.2.1/24"], action=1, data=[5])
        pkt = testutils.simple_ip_packet(ip_src='1.1.1.1', ip_dst='192.168.2.1')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


class SimpleLpmP4TwoKeysPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/psa-lpm-two-keys.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(ip_src='1.2.3.4', ip_dst='10.10.11.11')
        # This command adds LPM entry 10.10.11.0/24 with action forwarding on port 6 (PORT2 in ptf)
        # Note that prefix value has to be a sum of exact fields size and lpm prefix
        self.table_add(table="ingress_tbl_fwd_exact_lpm", keys=["1.2.3.4", "10.10.11.0/24"], action=1, data=[6])

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)

        pkt = testutils.simple_ip_packet(ip_src='1.2.3.4', ip_dst='192.168.2.1')
        # This command adds LPM entry 192.168.2.1/24 with action forwarding on port 5 (PORT1 in ptf)
        self.table_add(table="ingress_tbl_fwd_exact_lpm", keys=["1.2.3.4", "192.168.2.1/24"], action=1, data=[5])
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


class ConstDefaultActionPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/action-const-default.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


class ConstEntryPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/const-entry.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


class ConstEntryAndActionPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/const-entry-and-action.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        # via default action
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        # via const entry
        testutils.send_packet(self, PORT2, pkt)
        testutils.verify_packet(self, pkt, PORT0)

        # via LPM const entry
        pkt[IP].dst = 0x11223344
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)
        pkt[IP].dst = 0x11223355
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT0)


class BridgedMetadataPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/bridged-metadata.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt[Ether].dst = 'FF:FF:FF:FF:FF:FF'
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)


class QoSPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/cos-psa.p4"

    def runTest(self):
        ip_pkt = testutils.simple_ip_packet()
        exp_ip_pkt = ip_pkt.copy()
        exp_ip_pkt[Ether].dst = "00:00:00:00:00:10"
        arp_pkt = testutils.simple_arp_packet()
        exp_arp_pkt = arp_pkt.copy()
        exp_arp_pkt[Ether].dst = "00:00:00:00:00:11"

        testutils.send_packet(self, PORT0, ip_pkt)
        testutils.verify_packet(self, exp_ip_pkt, PORT1)

        testutils.send_packet(self, PORT0, arp_pkt)
        testutils.verify_packet(self, exp_arp_pkt, PORT1)


class PacketInLengthPSATest(P4EbpfTest):
    """
    Sends 114 bytes packet and check this using packet_in.length() in a parser
    """
    p4_file_path = "p4testdata/packet_in-length.p4"

    def runTest(self):
        pkt = Ether(dst="11:11:11:11:11:11") / testutils.simple_ip_only_packet(
            ip_dst="192.168.1.1")

        # check if packet_in.length() works
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        pkt = pkt / "Message"

        # Packet bigger than 114B is rejected in parser
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_packet(self, pkt, PORT1)


class PacketInAdvancePSATest(P4EbpfTest):
    """
    This test checks if MPLS header is skipped using packet_in.advance()
    """
    p4_file_path = "p4testdata/packet_in-advance.p4"

    def runTest(self):
        pkt = Ether(dst="11:11:11:11:11:11") / MPLS(label=20, cos=5, s=1, ttl=64) / \
              testutils.simple_ip_only_packet(ip_dst="192.168.1.1")

        exp_pkt = Ether(dst="11:11:11:11:11:11") / \
                  testutils.simple_ip_only_packet(ip_dst="192.168.1.1")

        # check if MPLS is skipped in parser
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, exp_pkt, PORT1)


class CountersPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/counters.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:44:55',
                                         eth_src='00:AA:00:00:00:01',
                                         pktlen=100)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        self.counter_verify(name="ingress_test1_cnt", keys=[1], bytes=100)
        self.counter_verify(name="ingress_test2_cnt", keys=[1], packets=1)
        self.counter_verify(name="ingress_test3_cnt", keys=[1], bytes=100, packets=1)
        self.counter_verify(name="ingress_action_cnt", keys=[5], bytes=100, packets=1)

        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:44:55',
                                         eth_src='00:AA:00:00:01:FE',
                                         pktlen=199)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        self.counter_verify(name="ingress_test1_cnt", keys=[0x1fe], bytes=199)
        self.counter_verify(name="ingress_test2_cnt", keys=[0x1fe], packets=1)
        self.counter_verify(name="ingress_test3_cnt", keys=[0x1fe], bytes=199, packets=1)
        self.counter_verify(name="ingress_action_cnt", keys=[5], bytes=299, packets=2)
