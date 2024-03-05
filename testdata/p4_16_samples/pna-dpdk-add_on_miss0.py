#!/usr/bin/env python3

# Copyright 2023 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Andy Fingerhut, andy.fingerhut@gmail.com

import time
from enum import Enum
from pathlib import Path

from ptf import testutils as ptfutils
from ptf.mask import Mask
from ptf.packet import *

# The base_test.py path is relative to the test file, this should be improved
FILE_DIR = Path(__file__).resolve().parent
BASE_TEST_PATH = FILE_DIR.joinpath("../../backends/dpdk/base_test.py")
sys.path.append(str(BASE_TEST_PATH))
import base_test as bt

# This global variable ensure that the forwarding pipeline will only be pushed once in one tes
PIPELINE_PUSHED = False


class AbstractTest(bt.P4RuntimeTest):
    EnumColor = Enum("EnumColor", ["GREEN", "YELLOW", "RED"], start=0)

    def setUp(self):
        bt.P4RuntimeTest.setUp(self)
        global PIPELINE_PUSHED
        if not PIPELINE_PUSHED:
            success = bt.P4RuntimeTest.updateConfig(self)
            assert success
            PIPELINE_PUSHED = True
        packet_wait_time = ptfutils.test_param_get("packet_wait_time")
        if not packet_wait_time:
            self.packet_wait_time = 0.1
        else:
            self.packet_wait_time = float(packet_wait_time)

    def tearDown(self):
        bt.P4RuntimeTest.tearDown(self)

    def setupCtrlPlane(self):
        pass

    def sendPacket(self):
        pass

    def verifyPackets(self):
        pass

    @bt.autocleanup
    def runTestImpl(self):
        self.setupCtrlPlane()
        bt.testutils.log.info("Sending Packet ...")
        self.sendPacket()
        bt.testutils.log.info("Verifying Packet ...")
        self.verifyPackets()


#############################################################
# Define a few small helper functions that help construct
# parameters for the table_add() method.
#############################################################


class OneEntryTest(AbstractTest):
    def entry_count(self, table_name):
        req, table = self.make_table_read_request(table_name)
        n = 0
        for response in self.response_dump_helper(req):
            n += len(response.entities)
        return n

    def delete_all_table_entries(self, table_name):
        req, _ = self.make_table_read_request(table_name)
        for response in self.response_dump_helper(req):
            for entity in response.entities:
                assert entity.HasField("table_entry")
                self.delete_table_entry(entity.table_entry)
                return
            return

    def sendPacket(self):
        in_dmac = "ee:30:ca:9d:1e:00"
        in_smac = "ee:cd:00:7e:70:00"
        ip_src_addr = "1.1.1.1"
        ip_dst_addr = "2.2.2.2"
        ig_port = 0
        sport = 59597
        dport = 7503
        pkt_in = ptfutils.simple_tcp_packet(
            eth_src=in_smac,
            eth_dst=in_dmac,
            ip_src=ip_src_addr,
            ip_dst=ip_dst_addr,
            tcp_sport=sport,
            tcp_dport=dport,
        )

        # Send in a first packet that should experience a miss on
        # table ct_tcp_table, cause a new entry to be added by the
        # data plane with a 30-second expiration time, and be
        # forwarded with a change to its source MAC address that the
        # add_on_miss0.p4 program uses to indicate that a miss
        # occurred.
        bt.testutils.log.info("Sending packet #1")
        ptfutils.send_packet(self, ig_port, pkt_in)
        first_pkt_time = time.time()

    def verifyPackets(self):
        in_dmac = "ee:30:ca:9d:1e:00"
        in_smac = "ee:cd:00:7e:70:00"
        ip_src_addr = "1.1.1.1"
        ip_dst_addr = "2.2.2.2"
        sport = 59597
        dport = 7503
        # add_on_miss0.p4 replaces least significant 8 bits of source
        # MAC address with 0xf1 on a hit of table ct_tcp_table, or
        # 0xa5 on a miss.
        out_smac_for_miss = in_smac[:-2] + "a5"
        eg_port = 1
        exp_pkt_for_miss = ptfutils.simple_tcp_packet(
            eth_src=out_smac_for_miss,
            eth_dst=in_dmac,
            ip_src=ip_src_addr,
            ip_dst=ip_dst_addr,
            tcp_sport=sport,
            tcp_dport=dport,
        )

        # Check hit not tested for now for simplicity
        # out_smac_for_hit = in_smac[:-2] + "f1"
        # exp_pkt_for_hit = \
        #     ptfutils.simple_tcp_packet(eth_src=out_smac_for_hit, eth_dst=in_dmac,
        #                          ip_src=ip_src_addr, ip_dst=ip_dst_addr,
        #                          tcp_sport=sport, tcp_dport=dport)

        ptfutils.verify_packets(self, exp_pkt_for_miss, [eg_port])
        bt.testutils.log.info("packet experienced a miss in ct_tcp_table as expected")

        bt.testutils.log.info("Attempting to delete all entries in ipv4_host")
        # TODO: This code does not seem functional?
        # self.delete_all_table_entries("MainControlImpl.ipv4_host")
        # assert self.entry_count("MainControlImpl.ipv4_host") == 0

    def setupCtrlPlane(self):
        ig_port = 0
        eg_port = 1

        ip_src_addr = 16843009  # "1.1.1.1"
        ip_dst_addr = 33686018  # "2.2.2.2"
        table_name = "MainControlImpl.ipv4_host"

        assert self.entry_count(table_name) == 0
        bt.testutils.log.info("Attempting to add entries to ipv4_host")
        self.table_add(
            (table_name, [self.Exact("hdr.ipv4.dst_addr", ip_src_addr)]),
            ("MainControlImpl.send", [("port", ig_port)]),
        )
        self.table_add(
            (table_name, [self.Exact("hdr.ipv4.dst_addr", ip_dst_addr)]),
            ("MainControlImpl.send", [("port", eg_port)]),
        )
        entry_count = self.entry_count(table_name)
        bt.testutils.log.info("Now ipv4_host contains %d entries" "", entry_count)
        assert entry_count == 2

    def runTest(self):
        self.runTestImpl()
