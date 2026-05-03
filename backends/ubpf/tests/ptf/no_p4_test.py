#!/usr/bin/env python

# Copyright 2019 Orange
# SPDX-FileCopyrightText: 2019 Orange
#
# SPDX-License-Identifier: Apache-2.0

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
