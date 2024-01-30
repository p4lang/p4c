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

        self.table_add(table="ingress_tbl_fwd", key=[DP_PORTS[0]], action=1, data=[DP_PORTS[1]])
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)  # Checks action run

        self.register_verify(name="ingress_reg", index=[DP_PORTS[1]], expected_value=["0x5"])

        # After next action run in register should be stored a new value - 15
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        self.register_verify(
            name="ingress_reg",
            index=["{}".format(hex(DP_PORTS[1]))],
            expected_value=["0xf"],
        )


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
    p4info_reference_file_path = "p4testdata/register-apply.p4info.txtpb"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        self.register_verify(name="ingress_reg", index=["0x5"], expected_value=["0x5"])

        # After next action run in register should be stored a new value - 15
        testutils.send_packet(self, PORT0, pkt)
        self.register_verify(name="ingress_reg", index=["0x5"], expected_value=["0xf"])


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
        self.register_verify(
            name="ingress_reg",
            index=["{}".format(hex(DP_PORTS[1]))],
            expected_value=["0x10"],
        )


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
        self.register_verify(name="ingress_reg", index=["0x5"], expected_value=["0x10"])


class RegisterStructsPSATest(P4EbpfTest):
    """
    Test register with key and value as a struct.
    1. Send a packet on PORT0
    2. P4 application will read a value (0),
    add 5 and write it to the register
    3. Verify a value stored in register (5)
    4. Set register value to port (0xDA), srcAddr (0x55), dstAddr (0x0)
    5. Verify a dstAddr field change to 0x0D
    """

    p4_file_path = "p4testdata/register-structs.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        testutils.send_packet(self, PORT0, pkt)
        self.register_verify(
            name="ingress_reg",
            index=["0x5", "0xffffffffffff"],
            expected_value=["0x0", "0x5", "0x0"],
        )

        self.register_set(
            name="ingress_reg",
            index=["0x5", "0xffffffffffff"],
            value=["0xDA", "0x55", "0x0"],
        )

        testutils.send_packet(self, PORT0, pkt)
        self.register_verify(
            name="ingress_reg",
            index=["0x5", "0xffffffffffff"],
            expected_value=["0xDA", "0x55", "0x0D"],
        )


class RegisterWideIndexData(P4EbpfTest):
    """
    Test Register with index and value types wider than 64 bits. The test is executed twice - when Register entry
    doesn't exist and when it exists. Only 104 MSB bits of IPv6 address are modified.
    1. Send packet.
    2. P4 program will swap IPv6 src address with value stored in Register entry at IPv6 dst address.
    3. Validate values.
    """

    p4_file_path = "p4testdata/wide-field-register.p4"

    def runTest(self):
        addr = "0000:1111:2222:3333:4444:5555:6767:7676"
        reg_key = "0x1111222233334444555567"
        pkt = testutils.simple_ipv6ip_packet(ipv6_dst=addr, ipv6_src="1:1111::0")

        # Test default value
        exp_pkt = testutils.simple_ipv6ip_packet(ipv6_dst=addr, ipv6_src="0::0")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)
        self.register_verify(
            name="ingress_reg",
            index=[reg_key],
            expected_value=["0x1_1111_0000_0000_0000_0000_00"],
        )

        # Test with register set
        self.register_set(
            name="ingress_reg", index=[reg_key], value=[0x1_0000_0000_0000_0000_0011_22]
        )
        exp_pkt = testutils.simple_ipv6ip_packet(ipv6_dst=addr, ipv6_src="1::11:2200:0")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)
        self.register_verify(
            name="ingress_reg",
            index=[reg_key],
            expected_value=["0x1_1111_0000_0000_0000_0000_00"],
        )
