# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

import base_test as bt
from ptf import testutils


class PortsAndDropTest(bt.P4RuntimeTest):
    def setUp(self):
        super().setUp()
        assert self.updateConfig()

    def runTest(self):
        # P4Runtime device 1 is independent of PTF dataplane device 0.
        for ingress, value, egress in ((0, 1, 1), (1, 2, 0), (0, 0, None), (1, 0, None)):
            packet = bytes([value]) + bytes(511)
            testutils.send_packet(self, ingress, packet)
            if egress is not None:
                testutils.verify_packet(self, packet, egress)
            testutils.verify_no_other_packets(self)
