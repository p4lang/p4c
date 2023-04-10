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
from ptf.packet import TCP, Ether, IPv6
from ptf.testutils import send_packet, simple_tcpv6_packet, verify_packets


class Ipv6Test(P4rtOVSBaseTest):
    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/test-ipv6-actions.o")
        self.add_bpf_prog_flow(1, 2)
        self.add_bpf_prog_flow(2, 1)


class Ipv6ModifyDstAddrTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
        )

    def runTest(self):
        pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::2")
        exp_pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::3")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv6SwapAddrTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        )

    def runTest(self):
        pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::2")
        exp_pkt = simple_tcpv6_packet(ipv6_src="2001::2", ipv6_dst="2001::1")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv6SetFlowLabelTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="2 0 0 0 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        )

    def runTest(self):
        pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::2", ipv6_fl=0)
        exp_pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::2", ipv6_fl=5)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv6SetTrafficClassFlowLabelTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="3 0 0 0 3 0 0 0 5 0 0 0 0 0 0 0 0 0 0 0",
        )

    def runTest(self):
        pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::2", ipv6_fl=0, ipv6_tc=0)
        exp_pkt = simple_tcpv6_packet(ipv6_src="2001::1", ipv6_dst="2001::2", ipv6_fl=5, ipv6_tc=3)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv6SetIPv6VersionTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="4 0 0 0 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        )

    def runTest(self):
        pkt = Ether(dst="00:01:02:03:04:05", src="00:06:07:08:09:0a")
        pkt /= IPv6(src="2001::1", dst="2001::2", fl=0, tc=0, hlim=64)
        pkt /= TCP(sport=1234, dport=80, flags="S")
        pkt /= "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"

        exp_pkt = Ether(dst="00:01:02:03:04:05", src="00:06:07:08:09:0a")
        exp_pkt /= IPv6(src="2001::1", dst="2001::2", fl=0, tc=0, hlim=64, version=5)
        exp_pkt /= TCP(sport=1234, dport=80, flags="S")
        exp_pkt /= "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv6SetNextHeaderTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        )

    def runTest(self):
        pkt = Ether(dst="00:01:02:03:04:05", src="00:06:07:08:09:0a")
        pkt /= IPv6(src="2001::1", dst="2001::2", fl=0, tc=0, hlim=64, nh=59)
        pkt /= TCP(sport=1234, dport=80, flags="S")
        pkt /= "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"

        exp_pkt = Ether(dst="00:01:02:03:04:05", src="00:06:07:08:09:0a")
        exp_pkt /= IPv6(src="2001::1", dst="2001::2", fl=0, tc=0, hlim=64, nh=0)
        exp_pkt /= TCP(sport=1234, dport=80, flags="S")
        exp_pkt /= "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv6SetHopLimitTest(Ipv6Test):
    def setUp(self):
        Ipv6Test.setUp(self)

        self.update_bpf_map(
            map_id=0,
            key="1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 32",
            value="6 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        )

    def runTest(self):
        pkt = Ether(dst="00:01:02:03:04:05", src="00:06:07:08:09:0a")
        pkt /= IPv6(src="2001::1", dst="2001::2", fl=0, tc=0, hlim=64)
        pkt /= TCP(sport=1234, dport=80, flags="S")
        pkt /= "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"

        exp_pkt = Ether(dst="00:01:02:03:04:05", src="00:06:07:08:09:0a")
        exp_pkt /= IPv6(src="2001::1", dst="2001::2", fl=0, tc=0, hlim=7)
        exp_pkt /= TCP(sport=1234, dport=80, flags="S")
        exp_pkt /= "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])
