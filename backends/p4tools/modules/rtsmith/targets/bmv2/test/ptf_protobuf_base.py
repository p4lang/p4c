# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Replay every generated request, including updates, and verify packet forwarding."""

from pathlib import Path

import base_test as bt
from google.protobuf import text_format
from p4.v1 import p4runtime_pb2
from ptf import testutils


class SetUpControlPlaneTest(bt.P4RuntimeTest):
    def setUp(self):
        super().setUp()
        assert self.updateConfig()

    def runTest(self):
        directory = Path(__file__).resolve().parent
        requests = [directory / "initial_config.txtpb"]
        requests += sorted(
            directory.glob("update_*.txtpb"), key=lambda path: int(path.stem.split("_")[-1])
        )
        for path in requests:
            request = text_format.Parse(path.read_text(), p4runtime_pb2.WriteRequest())
            if not request.updates:
                continue
            request.device_id = self.device_id
            request.election_id.low = 1
            # Send without autocleanup: subsequent requests can delete earlier inserts.
            self.stub.Write(request)
        packet = testutils.simple_tcp_packet()
        testutils.send_packet(self, 0, packet)
        testutils.verify_packet(self, packet, 1)
