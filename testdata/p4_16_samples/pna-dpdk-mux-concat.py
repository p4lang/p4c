# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

import base_test as bt
from ptf import testutils


class MuxConcatTest(bt.P4RuntimeTest):
    def setUp(self):
        super().setUp()
        assert self.updateConfig()

    @bt.autocleanup
    def runTest(self):
        # Exercise both branches of (value == 1 ? 7w2 : 7w0) ++ 1w1.
        for value, result in ((1, 5), (0, 1)):
            packet = bytes([value]) + bytes(63)
            expected = bytes([result]) + bytes(63)
            testutils.send_packet(self, 0, packet)
            testutils.verify_packet(self, expected, 1)
            testutils.verify_no_other_packets(self)
