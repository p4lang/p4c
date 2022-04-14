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
from scapy.layers.inet import IP, UDP

PORT0 = 0
PORT1 = 1
PORT2 = 2
ALL_PORTS = [PORT0, PORT1, PORT2]

class RegisterActionPSATest(P4EbpfTest):
    """
    Test register used in an action.
    0. Install table rule that forwards packets from PORT0 to PORT1
    1. Send a packet on PORT0
    2. P4 application will read a value (undefined means 0 at current implementation),
    add 5 and write it to the register
    3. Verify a packet at PORT1
    4. Verify a value stored in register (5)
    5. Send a packed on PORT0 second time
    6. P4 application will read a 5, add 10 and write it to the register
    7. Verify a value stored in register (15)
    """

    p4_file_path = "p4testdata/register-action.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        self.table_add(table="ingress_tbl_fwd", keys=[4], action=1, data=[5])
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)  # Checks action run

        self.verify_map_entry(name="ingress_reg", key="hex 05 00 00 00", expected_value="05 00 00 00")

        # After next action run in register should be stored a new value - 15
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        self.verify_map_entry(name="ingress_reg", key="hex 05 00 00 00", expected_value="0f 00 00 00")


class RegisterApplyPSATest(P4EbpfTest):
    """
    Test register used in the control block.
    1. Send a packet on PORT0
    2. P4 application will read a value (undefined means 0 at current implementation),
    add 5 and write it to the register
    3. Verify a value stored in register (5)
    4. Send a packed on PORT0 second time
    5. P4 application will read a 5, add 10 and write it to the register
    6. Verify a value stored in register (15)
    """
    p4_file_path = "p4testdata/register-apply.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        self.verify_map_entry(name="ingress_reg", key="hex 05 00 00 00", expected_value="05 00 00 00")

        # After next action run in register should be stored a new value - 15
        testutils.send_packet(self, PORT0, pkt)
        self.verify_map_entry(name="ingress_reg", key="hex 05 00 00 00", expected_value="0f 00 00 00")


class RegisterDefaultPSATest(P4EbpfTest):
    """
    Test register used with default value.
    1. Send a packet on PORT0
    2. P4 application will read a value (6),
    add 10 and write it to the register
    3. Verify a value stored in register (16)
    """
    p4_file_path = "p4testdata/register-default.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        self.verify_map_entry(name="ingress_reg", key="hex 05 00 00 00", expected_value="10 00 00 00")


class RegisterBigKeyPSATest(P4EbpfTest):
    """
    Test register used with default value and 64 bit key (using hash map).
    1. Send a packet on PORT0
    2. P4 application will read a value (6),
    add 10 and write it to the register
    3. Verify a value stored in register (16)
    """
    p4_file_path = "p4testdata/register-big-key.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        self.verify_map_entry(name="ingress_reg", key="hex 05 00 00 00 00 00 00 00", expected_value="10 00 00 00")


class RegisterStructsPSATest(P4EbpfTest):
    """
    Test register with key and value as a struct.
    1. Send a packet on PORT0
    2. P4 application will read a value (0),
    add 5 and write it to the register
    3. Verify a value stored in register (5)
    """
    p4_file_path = "p4testdata/register-structs.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        self.verify_map_entry(name="ingress_reg",
                              key="hex 05 00 00 00 00 00 00 00 ff ff ff ff ff ff 00 00",
                              expected_value="00 00 00 00 05 00 00 00 00 00 00 00")
