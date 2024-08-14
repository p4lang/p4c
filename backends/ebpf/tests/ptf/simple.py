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

from pathlib import Path
from typing import Optional
from ptf.mask import Mask
from ptf import testutils
from common import P4EbpfTest, DP_PORTS

FILE_DIR = Path(__file__).resolve().parent

PORT0 = 0
PORT1 = 1
PORT2 = 2
PORT3 = 3
PORT4 = 4
PORT5 = 5
ALL_PORTS = [PORT0, PORT1, PORT2, PORT3]


class SimpleTest(P4EbpfTest):
    p4_file_path = FILE_DIR.joinpath("../../psa/examples/simple.p4")
    p4info_reference_file_path = FILE_DIR.joinpath("../../psa/examples/simple.p4info.txtpb")

    def configure_port(self, port_id: int, vlan_id: Optional[int] = None) -> None:
        if vlan_id is None:
            self.table_add(table="ingress_tbl_ingress_vlan", key=[port_id, 0], action=1)
            self.table_add(table="egress_tbl_vlan_egress", key=[port_id], action=1)
        else:
            self.table_add(table="ingress_tbl_ingress_vlan", key=[port_id, 1], action=0)
            self.table_add(table="egress_tbl_vlan_egress", key=[port_id], action=2, data=[vlan_id])

    def setUp(self) -> None:
        super(SimpleTest, self).setUp()
        # self.configure_port(port_id=DP_PORTS[0])
        # self.configure_port(port_id=DP_PORTS[5])
        # self.configure_port(port_id=DP_PORTS[1], vlan_id=1)
        # self.configure_port(port_id=DP_PORTS[2], vlan_id=1)
        # self.configure_port(port_id=DP_PORTS[4], vlan_id=1)
        # self.configure_port(port_id=DP_PORTS[3], vlan_id=2)

    def runTest(self) -> None:
        # check no connectivity if switching rules are not installed
        pkt = testutils.simple_udp_packet(eth_dst="00:00:00:00:00:03")
        testutils.send_packet(self, PORT0, pkt)
        # testutils.verify_packet(self, pkt, PORT0)
        testutils.verify_any_packet_any_port(self, [pkt], ALL_PORTS)
