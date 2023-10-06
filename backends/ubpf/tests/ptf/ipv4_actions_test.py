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
from ptf.testutils import send_packet, simple_ip_packet, verify_packets


class Ipv4Test(P4rtOVSBaseTest):
    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/test-ipv4-actions.o")
        self.add_bpf_prog_flow(1, 2)
        self.add_bpf_prog_flow(2, 1)


class Ipv4SetVersionTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="0 0 0 0 5 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1", version=4) / TCP() / "Ala a un chat"
        exp_pkt = Ether() / IP(src="192.168.1.1", version=5) / TCP() / "Ala a un chat"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetIhlTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="1 0 0 0 15 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_ihl=10)
        exp_pkt = simple_ip_packet(ip_src="192.168.1.1", ip_ihl=15)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetDiffservTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="2 0 0 0 255 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_tos=10)
        exp_pkt = simple_ip_packet(ip_src="192.168.1.1", ip_tos=255)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetIdentificationTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="3 0 0 0 211 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_id=10)
        exp_pkt = simple_ip_packet(ip_src="192.168.1.1", ip_id=211)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetFlagsTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="4 0 0 0 7 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1", flags=0) / TCP() / "Ala a un chat"
        exp_pkt = Ether() / IP(src="192.168.1.1", flags=7) / TCP() / "Ala a un chat"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetFragOffsetTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="5 0 0 0 13 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1", frag=0) / TCP() / "Ala ma kota"
        exp_pkt = Ether() / IP(src="192.168.1.1", frag=13) / TCP() / "Ala ma kota"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetTtlTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="6 0 0 0 60 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_ttl=64)
        exp_pkt = simple_ip_packet(ip_src="192.168.1.1", ip_ttl=60)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetProtocolTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="7 0 0 0 55 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1") / TCP() / "Ala ma kota"
        exp_pkt = Ether() / IP(src="192.168.1.1", proto=55) / TCP() / "Ala ma kota"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetSrcTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="8 0 0 0 2 2 168 192 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1")
        exp_pkt = simple_ip_packet(ip_src="192.168.2.2")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetDstTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="9 0 0 0 2 2 168 192 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_dst="192.168.1.2")
        exp_pkt = simple_ip_packet(ip_src="192.168.1.1", ip_dst="192.168.2.2")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetSrcDstTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="10 0 0 0 10 10 10 10 10 10 10 10")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_dst="192.168.1.2")
        exp_pkt = simple_ip_packet(ip_src="10.10.10.10", ip_dst="10.10.10.10")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetIhlDiffservTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="11 0 0 0 15 26 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_src="192.168.1.1", ip_ihl=10, ip_tos=0)
        exp_pkt = simple_ip_packet(ip_src="192.168.1.1", ip_ihl=15, ip_tos=26)

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetFragmentOffsetFlagTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="12 0 0 0 13 0 7 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1", frag=0, flags=0) / TCP() / "Ala ma kota"
        exp_pkt = Ether() / IP(src="192.168.1.1", frag=13, flags=7) / TCP() / "Ala ma kota"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetFlagsTtlTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="13 0 0 0 7 50 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1", flags=0, ttl=64) / TCP() / "Ala ma kota"
        exp_pkt = Ether() / IP(src="192.168.1.1", flags=7, ttl=50) / TCP() / "Ala ma kota"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class Ipv4SetFragOffsetSrcTest(Ipv4Test):
    def setUp(self):
        Ipv4Test.setUp(self)

        self.update_bpf_map(
            map_id=0, key="1 1 168 192", value="14 0 0 0 255 31 0 0 255 255 255 255"
        )

    def runTest(self):
        pkt = Ether() / IP(src="192.168.1.1", frag=0) / TCP() / "Ala ma kota"
        exp_pkt = Ether() / IP(src="255.255.255.255", frag=8191) / TCP() / "Ala ma kota"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")
        mask.set_do_not_care_scapy(TCP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])
