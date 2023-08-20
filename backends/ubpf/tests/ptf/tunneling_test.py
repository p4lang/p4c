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
from ptf.packet import MPLS, Ether
from ptf.testutils import send_packet, simple_ip_only_packet, verify_packets


class TunnelingTest(P4rtOVSBaseTest):
    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/test-tunneling.o")
        self.add_bpf_prog_flow(1, 2)
        self.add_bpf_prog_flow(2, 1)


class MplsDownstreamTest(TunnelingTest):
    def setUp(self):
        TunnelingTest.setUp(self)

        self.update_bpf_map(map_id=1, key="1 1 168 192", value="0 0 0 0")

    def runTest(self):
        pkt = Ether(dst="11:11:11:11:11:11") / simple_ip_only_packet(ip_dst="192.168.1.1")
        exp_pkt = (
            Ether(dst="11:11:11:11:11:11")
            / MPLS(label=20, cos=5, s=1, ttl=64)
            / simple_ip_only_packet(ip_dst="192.168.1.1")
        )

        send_packet(self, (0, 1), pkt)
        verify_packets(self, exp_pkt, device_number=0, ports=[2])


class MplsUpstreamTest(TunnelingTest):
    def setUp(self):
        TunnelingTest.setUp(self)

        self.update_bpf_map(map_id=0, key="20 0 0 0", value="0 0 0 0")

    def runTest(self):
        pkt = (
            Ether(dst="11:11:11:11:11:11")
            / MPLS(label=20, cos=5, s=1, ttl=64)
            / simple_ip_only_packet(ip_dst="192.168.1.1")
        )
        exp_pkt = Ether(dst="11:11:11:11:11:11") / simple_ip_only_packet(ip_dst="192.168.1.1")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, exp_pkt, device_number=0, ports=[2])
