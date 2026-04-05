from functools import wraps
from typing import Any, List

import google.protobuf.text_format
import grpc
from p4.config.v1 import p4info_pb2
from p4.v1 import p4runtime_pb2, p4runtime_pb2_grpc
from ptf import config
from ptf import testutils as ptfutils
from ptf.base_tests import BaseTest
from ptf.mask import Mask

from tools import testutils
from tools.ptf import base_test as bt


class AbstractTest(bt.P4RuntimeTest):
    @bt.autocleanup
    def setUp(self) -> None:
        bt.P4RuntimeTest.setUp(self)
        success = bt.P4RuntimeTest.updateConfig(self)
        assert success

    def tearDown(self) -> None:
        bt.P4RuntimeTest.tearDown(self)

    def createWriteRequest(self) -> p4runtime_pb2.WriteRequest:
        req = p4runtime_pb2.WriteRequest()
        election_id = req.election_id
        election_id.high = 0
        election_id.low = 1
        req.device_id = self.device_id
        return req

    def setupCtrlPlane(self):
        initial_config_path = ptfutils.test_param_get("initial_config_file")
        assert initial_config_path is not None
        req = self.createWriteRequest()
        with open(initial_config_path, "r", encoding="utf-8") as initial_config_file:
            google.protobuf.text_format.Merge(
                initial_config_file.read(), req, allow_unknown_field=True
            )
        testutils.log.info("Initial configuration %s", req)
        return self.write_request(req)

    def sendCtrlPlaneUpdate(self) -> None:
        pass

    def runTestImpl(self) -> None:
        testutils.log.info("Configuring control plane...")
        self.setupCtrlPlane()

        testutils.log.info("Sending control plane updates...")
        self.sendCtrlPlaneUpdate()


class SetUpControlPlaneTest(AbstractTest):
    def runTest(self) -> None:
        self.runTestImpl()
