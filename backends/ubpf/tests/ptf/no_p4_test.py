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
from ptf.testutils import send_packet, simple_icmp_packet, verify_packets


class NormalOvsActionTest(P4rtOVSBaseTest):
    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.add_flow_normal()

    def runTest(self):
        pkt = simple_icmp_packet()
        exp_pkt = simple_icmp_packet()

        send_packet(self, (0, 1), pkt)
        verify_packets(self, exp_pkt, device_number=0, ports=[2])
