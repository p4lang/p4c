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
        # change default action
        self.table_set_default(table="ingress_tbl_fwd", action=1, data=[6])
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)


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
        self.table_add(table="ingress_tbl_fwd_lpm", key=["10.10.0.0/16"], action=1, data=[6])
        self.table_add(table="ingress_tbl_fwd_lpm", key=["10.10.10.10/8"], action=0)
        pkt = testutils.simple_ip_packet(ip_src='1.1.1.1', ip_dst='10.10.11.11')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)

        self.table_add(table="ingress_tbl_fwd_lpm", key=["192.168.2.1/24"], action=1, data=[5])
        pkt = testutils.simple_ip_packet(ip_src='1.1.1.1', ip_dst='192.168.2.1')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


class SimpleLpmP4TwokeyPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/psa-lpm-two-keys.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(ip_src='1.2.3.4', ip_dst='10.10.11.11')
        # This command adds LPM entry 10.10.11.0/24 with action forwarding on port 6 (PORT2 in ptf)
        # Note that prefix value has to be a sum of exact fields size and lpm prefix
        self.table_add(table="ingress_tbl_fwd_exact_lpm", key=["1.2.3.4", "10.10.11.0/24"], action=1, data=[6])

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)

        pkt = testutils.simple_ip_packet(ip_src='1.2.3.4', ip_dst='192.168.2.1')
        # This command adds LPM entry 192.168.2.1/24 with action forwarding on port 5 (PORT1 in ptf)
        self.table_add(table="ingress_tbl_fwd_exact_lpm", key=["1.2.3.4", "192.168.2.1/24"], action=1, data=[5])
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


class DigestPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/digest.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_src="fa:fb:fc:fd:fe:f0")
        testutils.send_packet(self, PORT0, pkt)
        testutils.send_packet(self, PORT0, pkt)
        testutils.send_packet(self, PORT0, pkt)

        digests = self.digest_get("IngressDeparserImpl_mac_learn_digest")
        if len(digests) != 3:
            self.fail("Expected 3 digest messages, got {}".format(len(digests)))
        for d in digests:
            if d["srcAddr"] != "0xfafbfcfdfef0" or d["ingress_port"] != "0x4":
                self.fail("Digest map stored wrong values: mac->{}, port->{}".format(d["srcAddr"], d["ingress_port"]))

                
class CountersPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/counters.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:44:55',
                                         eth_src='00:AA:00:00:00:01',
                                         pktlen=100)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        self.counter_verify(name="ingress_test1_cnt", key=[1], bytes=100)
        self.counter_verify(name="ingress_test2_cnt", key=[1], packets=1)
        self.counter_verify(name="ingress_test3_cnt", key=[1], bytes=100, packets=1)
        self.counter_verify(name="ingress_action_cnt", key=[5], bytes=100, packets=1)

        pkt = testutils.simple_ip_packet(eth_dst='00:11:22:33:44:55',
                                         eth_src='00:AA:00:00:01:FE',
                                         pktlen=199)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        self.counter_verify(name="ingress_test1_cnt", key=[0x1fe], bytes=199)
        self.counter_verify(name="ingress_test2_cnt", key=[0x1fe], packets=1)
        self.counter_verify(name="ingress_test3_cnt", key=[0x1fe], bytes=199, packets=1)
        self.counter_verify(name="ingress_action_cnt", key=[5], bytes=299, packets=2)


class DirectCountersPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/direct-counters.p4"

    def runTest(self):
        self.table_add(table="ingress_tbl1", key=["10.0.0.0"], action=1)
        self.table_add(table="ingress_tbl2", key=["10.0.0.1"], action=2)
        self.table_add(table="ingress_tbl2", key=["10.0.0.2"], action=3)

        for i in range(3):
            pkt = testutils.simple_ip_packet(pktlen=100, ip_src='10.0.0.{}'.format(i))
            testutils.send_packet(self, PORT0, pkt)
            testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        self.table_verify(table="ingress_tbl1", key=["10.0.0.0"], action=1,
                          counters={"ingress_test3_cnt": {"bytes": 100, "packets": 1}})
        self.table_verify(table="ingress_tbl2", key=["10.0.0.1"], action=2,
                          counters={"ingress_test2_cnt": {"packets": 1},
                                    "ingress_test3_cnt": {"bytes": 0, "packets": 0}})
        self.table_verify(table="ingress_tbl2", key=["10.0.0.2"], action=3,
                          counters={"ingress_test2_cnt": {"packets": 1},
                                    "ingress_test3_cnt": {"bytes": 100, "packets": 1}})


class ParserValueSetPSATest(P4EbpfTest):
    """
    Test value_set implementation. P4 application will pass packet, which IP destination
    address contains value_set and destination port 80.
    """
    p4_file_path = "p4testdata/pvs.p4"

    def runTest(self):
        pkt = testutils.simple_udp_packet(ip_dst='10.0.0.1', udp_dport=80)

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        self.update_map("IngressParserImpl_pvs", '1 0 0 10', '0 0 0 0')

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt[IP].dst = '8.8.8.8'
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)


# xdp2tc=head is not supported because pkt_len of test packet (Ethernet+RandomHeader) < 34 B
@xdp2tc_head_not_supported
class RandomPSATest(P4EbpfTest):
    """
    Read random data generated by data plane.
    Verify that random values are in the expected range.
    """
    p4_file_path = "p4testdata/random.p4"

    class RandomHeader(Packet):
        name = "random"
        fields_desc = [
            IntField("f1", 0),
            ShortField("f2", 0),
            ShortField("f3", 0)
        ]

    def setUp(self):
        super(RandomPSATest, self).setUp()
        bind_layers(Ether, self.RandomHeader, type=0x801)

    def tearDown(self):
        split_layers(Ether, self.RandomHeader, type=0x801)
        super(RandomPSATest, self).tearDown()

    def verify_range(self, value, min_value, max_value):
        if value < min_value or value > max_value:
            self.fail("Value {} out of range [{}, {}]".format(value, min_value, max_value))

    def runTest(self):
        self.table_add(table="MyIC_tbl_fwd", key=[4], action=1, data=[5])
        pkt = Ether() / self.RandomHeader()
        mask = Mask(pkt)
        mask.set_do_not_care_scapy(self.RandomHeader, "f1")
        mask.set_do_not_care_scapy(self.RandomHeader, "f2")
        mask.set_do_not_care_scapy(self.RandomHeader, "f3")
        sequence = [[], [], []]
        for _ in range(10):
            testutils.send_packet(self, PORT0, pkt)
            (_, recv_pkt) = testutils.verify_packet_any_port(self, mask, ALL_PORTS)
            recv_pkt = Ether(recv_pkt)
            self.verify_range(value=recv_pkt[self.RandomHeader].f1, min_value=0x80_00_00_01, max_value=0x80_00_00_05)
            sequence[0].append(recv_pkt[self.RandomHeader].f1)
            self.verify_range(value=recv_pkt[self.RandomHeader].f2, min_value=0, max_value=127)
            sequence[1].append(recv_pkt[self.RandomHeader].f2)
            self.verify_range(value=recv_pkt[self.RandomHeader].f3, min_value=256, max_value=259)
            sequence[2].append(recv_pkt[self.RandomHeader].f3)
        logger.info("f1 sequence: {}".format(sequence[0]))
        logger.info("f2 sequence: {}".format(sequence[1]))
        logger.info("f3 sequence: {}".format(sequence[2]))


class VerifyPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/verify.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, pkt, ALL_PORTS)

        pkt[Ether].src = '00:00:00:00:00:00'
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        pkt[Ether].src = '00:A0:00:00:00:01'
        pkt[Ether].type = 0x1111
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)

        # explicit transition to reject state
        pkt[Ether].type = 0xFF00
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_no_other_packets(self)


class PSATernaryTest(P4EbpfTest):

    p4_file_path = "p4testdata/psa-ternary.p4"

    def runTest(self):
        # flow rules for 'tbl_ternary_0'
        # 1. ipv4.srcAddr=1.2.3.4/0xffffff00 => action 0 priority 1
        # 2. ipv4.srcAddr=1.2.3.4/0xffff00ff => action 1 priority 10
        self.table_add(table="ingress_tbl_ternary_0", key=["1.2.3.4^0xffffff00"], action=0, priority=1)
        self.table_add(table="ingress_tbl_ternary_0", key=["1.2.3.4^0xffff00ff"], action=1, priority=10)

        # flow rules for 'tbl_ternary_1'
        # 1. ipv4.diffserv=0x00/0x00, ipv4.dstAddr=192.168.2.1/24 => action 0 priority 1
        # 2. ipv4.diffserv=0x00/0xff, ipv4.dstAddr=192.168.2.1/24 => action 1 priority 10
        self.table_add(table="ingress_tbl_ternary_1", key=["192.168.2.1/24", "0^0"], action=0, priority=1)
        self.table_add(table="ingress_tbl_ternary_1", key=["192.168.2.1/24", "0^0xFF"], action=1, priority=10)

        # flow rules 'tbl_ternary_2':
        # 1. ipv4.protocol=0x11, ipv4.diffserv=0x00/0x00, ipv4.dstAddr=192.168.2.1/16 => action 0 priority 1
        # 2. ipv4.protocol=0x11, ipv4.diffserv=0x00/0xff, ipv4.dstAddr=192.168.2.1/16 => action 1 priority 10
        self.table_add(table="ingress_tbl_ternary_2", key=["192.168.2.1/16", "0x11", "0^0"], action=0, priority=1)
        self.table_add(table="ingress_tbl_ternary_2", key=["192.168.2.1/16", "0x11", "0^0xFF"], action=1, priority=10)

        # flow rules 'tbl_ternary_3':
        # 1. ipv4.protocol=0x7, ipv4.diffserv=selector, ipv4.dstAddr=0xffffffff^0xffffffff => action 0 priority 1
        # 2. ipv4.protocol=0x7, ipv4.diffserv=selector, ipv4.dstAddr=0x0^0x0 => action 1 priority 10
        ref1 = self.action_selector_add_action(selector="ingress_as", action=0, data=[])
        ref2 = self.action_selector_add_action(selector="ingress_as", action=1, data=[])
        self.table_add(table="ingress_tbl_ternary_3", key=["0xffffffff^0xffffffff", "0x7"], references=[ref1], priority=1)
        self.table_add(table="ingress_tbl_ternary_3", key=["0x0^0x0", "0x7"], references=[ref2], priority=10)

        # flow rules 'tbl_ternary_4':
        # 2. hdr.ethernet.srcAddr=00:00:33:44:55:00^00:00:FF:FF:FF:00 => action 1 priority 10
        self.table_add(table="ingress_ap", key=[0x10], action=1, data=[])
        self.table_add(table="ingress_tbl_ternary_4", key=["00:00:33:44:55:00^00:00:FF:FF:FF:00"],
                       references=["0x10"], priority=10)

        pkt = testutils.simple_udp_packet(eth_src="11:22:33:44:55:66", ip_src='1.2.3.4', ip_dst='192.168.2.1')
        testutils.send_packet(self, PORT0, pkt)
        pkt[Ether].type = 0x1122
        pkt[IP].proto = 0x7
        pkt[IP].tos = 0x5
        pkt[IP].chksum = 0xb3e7
        pkt[IP].src = '17.17.17.17'
        pkt[IP].dst = '255.255.255.255'
        pkt[UDP].chksum = 0x044D
        testutils.verify_packet(self, pkt, PORT1)


class ActionDefaultTernaryPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/action-default-ternary.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # Test default action for ternary match
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)


class ConstEntryTernaryPSATest(P4EbpfTest):

    p4_file_path = "p4testdata/const-entry-ternary.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        pkt[IP].src = 0x33333333

        # via ternary const entry
        pkt[Ether].src = "55:55:55:55:55:11"  # mask is 0xFFFFFFFFFF00
        pkt[IP].dst = 0x11229900  # mask is 0xFFFF00FF
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT2)
        pkt[Ether].src = "77:77:77:77:11:11"  # mask is 0xFFFFFFFF0000
        pkt[IP].dst = 0x11993355  # mask is 0xFF00FFFF
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
