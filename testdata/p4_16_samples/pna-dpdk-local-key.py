# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

import base_test as bt
from ptf import testutils


class LocalKeyTest(bt.P4RuntimeTest):
    def setUp(self):
        super().setUp()
        assert self.updateConfig()

    @bt.autocleanup
    def runTest(self):
        self.table_add(
            ("MainControl.lookup", [self.Exact("key", 1), self.Exact("hdr.h.$valid$", 1)]),
            ("MainControl.forward", [("port", 1)]),
        )
        for value in (1, 2):
            packet = bytes([value]) + bytes(511)
            testutils.send_packet(self, 0, packet)
            if value == 1:
                testutils.verify_packet(self, packet, 1)
            testutils.verify_no_other_packets(self)
