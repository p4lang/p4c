#!/usr/bin/env python

# Copyright 2019 Orange
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from base_test import P4rtOVSBaseTest
from ptf.mask import Mask
from ptf.packet import IP, TCP, Ether
from ptf.testutils import (
    send_packet,
    simple_ip_only_packet,
    simple_ip_packet,
    simple_mpls_packet,
    verify_packets,
)


class SimpleActionsTest(P4rtOVSBaseTest):
    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/test-simple-actions.o")
        self.add_bpf_prog_flow(1, 2)
        self.add_bpf_prog_flow(2, 1)


class IpModifySrcAddressTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="8 0 0 0 5 1 168 192 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_dst="192.168.1.1", ip_src="192.168.1.2")

        exp_pkt = simple_ip_packet(ip_dst="192.168.1.1", ip_src="192.168.1.5")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsModifyStackTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="5 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"s": 0}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"s": 1}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsDecrementTtlTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="0 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"ttl": 10}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"ttl": 9}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetLabelTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="1 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"label": 5}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"label": 1}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetLabelDecrementTtlTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="2 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"ttl": 10, "label": 5}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"ttl": 9, "label": 1}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetModifyTcTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="3 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"label": 5, "tc": 5}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"label": 5, "tc": 1}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetLabelTcTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="4 0 0 0 2 0 0 0 2 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"label": 5, "tc": 5}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"label": 2, "tc": 2}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class ChangeIpVersionTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="6 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(dst="192.168.1.1", version=4) / TCP() / "Ala has a cat"

        exp_pkt = Ether() / IP(dst="192.168.1.1", version=6) / TCP() / "Ala has a cat"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class IpSwapAddressTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="7 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(dst="192.168.1.1", src="192.168.1.2") / TCP() / "Ala has a cat"

        exp_pkt = Ether() / IP(dst="192.168.1.2", src="192.168.1.1") / TCP() / "Ala has a cat"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class NoActionPacketTest(SimpleActionsTest):
    def setUp(self):
        SimpleActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="10 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(
            mpls_tags=[{"label": 1}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1", ip_src="192.168.1.2"),
        )

        exp_pkt = simple_mpls_packet(
            mpls_tags=[{"label": 1}],
            inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1", ip_src="192.168.1.2"),
        )

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "ttl")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])
