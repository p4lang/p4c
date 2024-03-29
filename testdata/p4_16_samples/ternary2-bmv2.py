# P4Runtime PTF test for ternary2-bmv2
# p4testgen seed: 12

import logging
import os
import sys
from functools import wraps

from p4.v1 import p4runtime_pb2_grpc
from ptf import config
from ptf import testutils as ptfutils
from ptf.mask import Mask
from ptf.packet import *
from ptf.thriftutils import *

directory = os.getcwd()
directory = directory.split("/")
workspaceFolder = ""
for i in range(len(directory) - 1):
    workspaceFolder += directory[i] + "/"
sys.path.insert(1, workspaceFolder + 'backends/p4tools/modules/testgen/targets/bmv2/backend/ptf')
import base_test as bt

logger = logging.getLogger('ternary2-bmv2')
logger.addHandler(logging.StreamHandler())


class AbstractTest(bt.P4RuntimeTest):
    @bt.autocleanup
    def setUp(self):
        bt.P4RuntimeTest.setUp(self)
        success = bt.P4RuntimeTest.updateConfig(self)
        assert success

    def tearDown(self):
        bt.P4RuntimeTest.tearDown(self)

    def setupCtrlPlane(self):
        pass

    def sendPacket(self):
        pass

    def verifyPackets(self):
        pass

    def runTestImpl(self):
        self.setupCtrlPlane()
        logger.info("Sending Packet ...")
        self.sendPacket()
        logger.info("Verifying Packet ...")
        self.verifyPackets()
        logger.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=2)


class Test1(AbstractTest):
    # Date generated: 2023-01-17-17:00:00.528
    # Current statement coverage: 0.5

    def setupCtrlPlane(self):
        self.table_add(
            (
                'ingress.ex1',
                [
                    self.Ternary('hdrs.extra[0].h', 0x93F7, 0xC3B7),
                ],
            ),
            (
                'ingress.setbyte',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )
        self.table_add(
            (
                'ingress.test1',
                [
                    self.Ternary('hdrs.data.f1', 0xBFD67EF3, 0xDD4B9E1C),
                ],
            ),
            (
                'ingress.setb1',
                [
                    ('port', 0x002),
                    ('val', 0x00),
                ],
            ),
            1,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\xBF\xD6\x1F\xB2\x00\x00\x00\x00\x00\x00\x00\x00\xA3\xB7\x00\x77\x07\xF9\x84\x8B\x16\xAA\x73\x98\x5B\xE9\x47\x5F\xBD\xE0\xA3\x12\xCE\xEE\xC4\xC2\xF4\x47\xB4\xF1\xD0\x17\x2C\xD9\x17\x8C\xF8\x35\x60\x49\xB0\x8D\x4F\x0D\xC4\xE3\x71\x4C\x02\x97\x83\x0E\x1C\x7A\xE4\x58\x9B\x0F\xA6\x88\xBB\x71\xA1\x96\x09\x3B\xB4\x37\xEA\x9A\x75\x83\x45\xA5\x4F\x2D\x96\x9B\x14\x3F\x2C\x86\xBE\x0E\x99\xE5\x0D\xE1\xC4\x50\x53\xFE\x61\x76\x20\xF2\xEF\x94\xAD\x32\x66\x81\xAD\xA8\xB8\xA2\xF3\xA3\xB6\x7E\x24\x08\x39\x92\xC9\xE4\xD7\xA4\x45\x03\xED\xE8\xEB\x2F\xC3\x46\xF1\x27\x50\xB1\x8B\x03\xFF\x1D\x15\x00\x94\x43\x9B\x36\xA5\xCD\xB3\x44\x70\x92\x42\x59\x00\x92\x89\x1A\x3A\x0C\x70\x4B\x35\x0C\x58\x10\x64\x69\x04\x41\xE6\x6A\x97\x33\xA2\xCE\x06\xAF\xF3\x85\xF3\x74\x5B\x80\x2D\xAA\x72\x0E\x15\xCA\x46\x91\x98\x01\xF7\x38\x89\xCA\x74\x71\x75\x86\xF6\x4C\xD7\xF1\xED\x03\xB6\x2E\x79\x07\x24\xF7\x12\xB9\xD2\x0F\x2B\xA2\xDE\x00\xA0\xD1\x4D\x57\x2B\xBB\x86\x79\xF5\x43\x8F\x20\xB3\x15\x92\xE4\xC5\x26\xFE\x88\x69\x09\x00\xC9\x9D\x92\x1C\xBA\x14\x77\x2B\x89\xE1\xEF\xA7\xA4\xED\x5F\xAC\x51\xEB\x05\x44\x3A\xD4\x76\x90\xEC\xB7\xED\xCD\x78\xB6\x6C\x8E\xB3\x79\xBC\xD0\xF1\x63\x6B\x53\xFC\x54\x4D\xE2\x4E\x3B\xF6\x6C\xC0\x3A\x44\xC1\x61\xCA\x2D\x74\xD8\x9C\xED\x3E\xE6\x82\xFE\xC1\x5F\xC3\xCA\x17\xDA\x6D\x66\x8F\xCB\x46\x9B\xB6\xCC\xC6\x73\x88\xF2\xAA\x6B\xF6\x21\xE0\x68\x19\x94\x3C\x8B\xA3\x80\xAF\xD3\x33\xCE\x6B\x1F\x21\x6D\x35\xC4\x79\x48\xCB\x2C\x3F\x62\xB5\xF3\x47\xBF\x44\xFC\x8F\x45\x73\x9F\xE4\x9C\xF5\x37\xB7\x5C\xCB\xFA\x62\x03\xA5\x09\x94\xED\x2C\x55\xAB\x75\xEB\xDD\xC1\x06\x41\x53\xDB\x97\xC4\x2A\x14\x05\xE0\xB3\xDF\x0F\x04\x0D\x8E\x3B\x38\xBB\xAE\x24\xE1\x24\xF0\xC2\xF6\x4A\x22\x5C\x19\xA1\x1E\x09\x4F\xD0\xF0\x67\xD8\x33\xE1\x92\xBB\x78\xA2\x83\xD9\x7F\xFC\x66\xBB\xAB\xE4\x52\x1B\xB3\xF3\xDC\x6E\x4F\x55\x72\xA0\x39\x2E\x12\xF0\x1D\xC4\x11\x9E\xF1\x74\x16\xAD\x22\x9D\x22\x49\xCD\x1F\xBA\x6D\x05\x53\xCA\x7B\x90\x2A\x04\x86\xC3\xBA\x3B\xD4\x74\xBF\xC0\xD0\x29\x24\xBB\xB2\xF5\x2D\xB5\xD6\xC4\xA4\x70\x92\xAF\x2F\xED\x60\x7C\x41\xB1\xBF\xE8\x42\xAC\x77\x8D\x52\x56\x9E\x25\xE1\x9D\xA1\x85\x6F\x11\xA3\x30\x9B\xBC\x64\x00\x94\x0F\xBE\xE9\x9C\x25\x23\xDA\x4C\xE0\x59\x7B\x45\x90\x52\xF1\xB8\xBD\x0D\xFB\xD0\x4C\xCC\x9B\x28\x44\x68\xAD\x0E\x2D\x75\x48\xA8\xF9\x80\x5F\x72\x14\x4A\x23\x20\x26\x76\xB9\xD8\x21\xCB\xF4\xFA\xA1\xA2\x15\x20\x51\xD9\x08\x8E\x4E\x8D\x4B\xA1\xBF\x85'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 2
        exp_pkt = Mask(
            b'\xBF\xD6\x1F\xB2\x00\x00\x00\x00\x00\x00\x00\x00\xA3\xB7\x00\x77\x07\xF9\x84\x8B\x16\xAA\x73\x98\x5B\xE9\x47\x5F\xBD\xE0\xA3\x12\xCE\xEE\xC4\xC2\xF4\x47\xB4\xF1\xD0\x17\x2C\xD9\x17\x8C\xF8\x35\x60\x49\xB0\x8D\x4F\x0D\xC4\xE3\x71\x4C\x02\x97\x83\x0E\x1C\x7A\xE4\x58\x9B\x0F\xA6\x88\xBB\x71\xA1\x96\x09\x3B\xB4\x37\xEA\x9A\x75\x83\x45\xA5\x4F\x2D\x96\x9B\x14\x3F\x2C\x86\xBE\x0E\x99\xE5\x0D\xE1\xC4\x50\x53\xFE\x61\x76\x20\xF2\xEF\x94\xAD\x32\x66\x81\xAD\xA8\xB8\xA2\xF3\xA3\xB6\x7E\x24\x08\x39\x92\xC9\xE4\xD7\xA4\x45\x03\xED\xE8\xEB\x2F\xC3\x46\xF1\x27\x50\xB1\x8B\x03\xFF\x1D\x15\x00\x94\x43\x9B\x36\xA5\xCD\xB3\x44\x70\x92\x42\x59\x00\x92\x89\x1A\x3A\x0C\x70\x4B\x35\x0C\x58\x10\x64\x69\x04\x41\xE6\x6A\x97\x33\xA2\xCE\x06\xAF\xF3\x85\xF3\x74\x5B\x80\x2D\xAA\x72\x0E\x15\xCA\x46\x91\x98\x01\xF7\x38\x89\xCA\x74\x71\x75\x86\xF6\x4C\xD7\xF1\xED\x03\xB6\x2E\x79\x07\x24\xF7\x12\xB9\xD2\x0F\x2B\xA2\xDE\x00\xA0\xD1\x4D\x57\x2B\xBB\x86\x79\xF5\x43\x8F\x20\xB3\x15\x92\xE4\xC5\x26\xFE\x88\x69\x09\x00\xC9\x9D\x92\x1C\xBA\x14\x77\x2B\x89\xE1\xEF\xA7\xA4\xED\x5F\xAC\x51\xEB\x05\x44\x3A\xD4\x76\x90\xEC\xB7\xED\xCD\x78\xB6\x6C\x8E\xB3\x79\xBC\xD0\xF1\x63\x6B\x53\xFC\x54\x4D\xE2\x4E\x3B\xF6\x6C\xC0\x3A\x44\xC1\x61\xCA\x2D\x74\xD8\x9C\xED\x3E\xE6\x82\xFE\xC1\x5F\xC3\xCA\x17\xDA\x6D\x66\x8F\xCB\x46\x9B\xB6\xCC\xC6\x73\x88\xF2\xAA\x6B\xF6\x21\xE0\x68\x19\x94\x3C\x8B\xA3\x80\xAF\xD3\x33\xCE\x6B\x1F\x21\x6D\x35\xC4\x79\x48\xCB\x2C\x3F\x62\xB5\xF3\x47\xBF\x44\xFC\x8F\x45\x73\x9F\xE4\x9C\xF5\x37\xB7\x5C\xCB\xFA\x62\x03\xA5\x09\x94\xED\x2C\x55\xAB\x75\xEB\xDD\xC1\x06\x41\x53\xDB\x97\xC4\x2A\x14\x05\xE0\xB3\xDF\x0F\x04\x0D\x8E\x3B\x38\xBB\xAE\x24\xE1\x24\xF0\xC2\xF6\x4A\x22\x5C\x19\xA1\x1E\x09\x4F\xD0\xF0\x67\xD8\x33\xE1\x92\xBB\x78\xA2\x83\xD9\x7F\xFC\x66\xBB\xAB\xE4\x52\x1B\xB3\xF3\xDC\x6E\x4F\x55\x72\xA0\x39\x2E\x12\xF0\x1D\xC4\x11\x9E\xF1\x74\x16\xAD\x22\x9D\x22\x49\xCD\x1F\xBA\x6D\x05\x53\xCA\x7B\x90\x2A\x04\x86\xC3\xBA\x3B\xD4\x74\xBF\xC0\xD0\x29\x24\xBB\xB2\xF5\x2D\xB5\xD6\xC4\xA4\x70\x92\xAF\x2F\xED\x60\x7C\x41\xB1\xBF\xE8\x42\xAC\x77\x8D\x52\x56\x9E\x25\xE1\x9D\xA1\x85\x6F\x11\xA3\x30\x9B\xBC\x64\x00\x94\x0F\xBE\xE9\x9C\x25\x23\xDA\x4C\xE0\x59\x7B\x45\x90\x52\xF1\xB8\xBD\x0D\xFB\xD0\x4C\xCC\x9B\x28\x44\x68\xAD\x0E\x2D\x75\x48\xA8\xF9\x80\x5F\x72\x14\x4A\x23\x20\x26\x76\xB9\xD8\x21\xCB\xF4\xFA\xA1\xA2\x15\x20\x51\xD9\x08\x8E\x4E\x8D\x4B\xA1\xBF\x85'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)

    def runTest(self):
        self.runTestImpl()


class Test2(AbstractTest):
    # Date generated: 2023-01-17-17:00:00.537
    # Current statement coverage: 0.67

    def setupCtrlPlane(self):
        self.table_add(
            (
                'ingress.ex1',
                [
                    self.Ternary('hdrs.extra[0].h', 0x0000, 0x0000),
                ],
            ),
            (
                'ingress.act3',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )
        self.table_add(
            (
                'ingress.tbl3',
                [
                    self.Ternary('hdrs.data.f2', 0x3E8EBBE3, 0x52FBFAEE),
                ],
            ),
            (
                'ingress.setbyte',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x3F\x8E\xBB\xE2\x00\x00\x00\x00\xA4\xE4\x51'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(b'\x00\x00\x00\x00\x3F\x8E\xBB\xE2\x00\x00\x00\x00\xA4\xE4\x51')
        ptfutils.verify_packet(self, exp_pkt, eg_port)

    def runTest(self):
        self.runTestImpl()


class Test3(AbstractTest):
    # Date generated: 2023-01-17-17:00:00.547
    # Current statement coverage: 0.67

    def setupCtrlPlane(self):
        self.table_add(
            (
                'ingress.test1',
                [
                    self.Ternary('hdrs.data.f1', 0xB4DFD4DF, 0x759EAE4E),
                ],
            ),
            (
                'ingress.setb1',
                [
                    ('port', 0x006),
                    ('val', 0x00),
                ],
            ),
            1,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\xBC\xBE\x94\x6F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xA5\x4A'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 6
        exp_pkt = Mask(b'\xBC\xBE\x94\x6F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xA5\x4A')
        ptfutils.verify_packet(self, exp_pkt, eg_port)

    def runTest(self):
        self.runTestImpl()


class Test4(AbstractTest):
    # Date generated: 2023-01-17-17:00:00.567
    # Current statement coverage: 0.83

    def setupCtrlPlane(self):
        self.table_add(
            (
                'ingress.ex1',
                [
                    self.Ternary('hdrs.extra[0].h', 0xB3FF, 0xF9F6),
                ],
            ),
            (
                'ingress.act2',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )
        self.table_add(
            (
                'ingress.tbl2',
                [
                    self.Ternary('hdrs.data.f2', 0x3FB8BFE5, 0x3DBFE2CC),
                ],
            ),
            (
                'ingress.setbyte',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )
        self.table_add(
            (
                'ingress.test1',
                [
                    self.Ternary('hdrs.data.f1', 0x77E8CEF9, 0x8AA21772),
                ],
            ),
            (
                'ingress.setb1',
                [
                    ('port', 0x005),
                    ('val', 0x00),
                ],
            ),
            1,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x27\xE4\xC6\xF4\xFD\xB8\xA2\xC6\x00\x00\x00\x00\xB5\xF6\x00\x59\xB9\x45\x18\x6E\x39\xCF\xB1\xDF\x25\xFB\x43\xBB\x58\x9A\xA6\x4C\x39\xC4\x72\xEF\x51\x91\x52\x1F\x8F\x4A\x11\x64\xB5\xE4\x1C\x6D\xE5\xEC\x2D\x9F\x1D\x20\x8B\xAA\xEB\x9A\xD6\xFA\x80\xEE\x89\xBD\x1B\x41\xB0\xA4\x64\xCE\x7F\x42\x78\x5E\xD6\x99\x06\x40\xC7\xC2\x4A\x4E\x3C\x60\x11\xE7\x57\xBA\x7E\xF5\x52\x82\x46\x20\xFE\x7D\x18\xBA\x02\xC1\xBF\x02\x94\x64\x1A\xED\x69\x7B\x1D\xE6\x90\xBD\x56\xBA\x24\x47\x69\x3E\xB9\x26\x99\xAA\xE9\x66\xCA\x5C\xE9\xF6\x1F\x4D\x87\xE0\xD5\x6C\x69\x10\x48\x64\x64\xEE\xDA\x92\x05\x8E\x20\x33\xDD\xED\x71\xB4\xD4\x02\x5B\xDE\x51\xA7\xF9\xD0\x13\xE9\x4B\x78\x55\xA7\x01\x36\x8B\xA0\x4D\xA0\xD3\xF9\x88\x65\x99\x2D\x2D\x4A\x5B\x17\x04\xA4\x7E\x4D\x59\xBE\xAB\x69\x41\xCD\x88\x30\xC8\x8B\xC8\xD4\x74\xCC\x5A\x3C\x5D\x4D\x32\x3F\x49\xDE\x7B\xE7\xB3\x84\x5B\x01\x3E\xAE\x54\xB3\x90\xDF\x1C\x5C\x45\xA8\x52\x2C\x5F\x6C\x49\xA2\x44\x1B\x21\x81\x4F\x33\x6C\x3E\xF8\x8D\x1D\x21\xA2\xBE\x33\xCF\x9F\x0C\xE6\xAE\x24\x74\xB4\x1B\x72\x9E\xF6\xDA\xA6\xF8\x07\x45\x46\x3A\xEF\x1D\x97\xFF\x24\x39\xAA\xDA\x57\xCE\x69\x49\x1C\x69\x5D\x8A\x6D\x33\xB7\x62\x04\x4F\x93\xCA\xF3\xE9\x0A\x84\xE2\x0B\x71\x60\x76\x37\xB8\x70\xF9\xFE\x10\x4A\x04\x18\x20\x78\x7D\x2D\x10\x0B\x11\x59\x88\x6B\x09\x32\xB3\x9B\x79\xD8\x27\x5C\x4D\xD1\x7A\xDB\xF5\x65\x09\x85\x30\x60\x96\xCF\x15\xE9\x9C\xE2\xBD\x55\x7F\x34\xF9\xAF\xB4\xF1\x1F\x2A\x0F\x77\x19\x87\x7A\x81\x58\xE3\xE8\xC9\xC7\xE1\x6C\xA0\x50\x06\xDA\x34\x49\xDC\xC6\xE9\x85\xD4\x29\xF3\x29\x5F\x45\x1A\x87\xED\xEE\xD3\x83\x91\xAA\x6C\x8B\xD4\x20\xBB\x51\x76\x48\xEE\xDE\x7A\x6B\x99\x01\x8D\xF1\x11\xB5\x9B\x90\x92\x87\x0C\x7C\x71\x4C\x26\xC3\xD7\x0C\x1F\x18\xC5\x28\xE6\x70\x01\x20\x58\x74\x89\xE3\xFD\xB2\xEA\x2E\x71\xDF\x9E\xD4\x74\x2A\xD4\x34\x03\xDD\x57\x33\x82\x54\x94\x8F\xD8\x1A\xEC\x42\x91\x13\x5B\x0D\x22\x57\x5A\x26\x27\x86\x47\x04\x05\x78\x21\x34\x6D\x74\x76\xF2\x95\x21\xAE\xF1\x29\xED\xFA\xE9\x1A\x03\x30\xE1\xAC\x0E\x92\x0C\x43\xC8\xF4\xB6\xC6\xD6\xDF\xAB\x7A\x9D\x34\x94\x83\xC2\xBE\x76\xB5\xFE\x1F\xE6\x94\xDF\x1B\xD9\xC8\xD5\xDD\xBB\xF3\x5C\x4B\xA7\x50\xD3\xF4\x15\xE9\x8E\x95\x9B\x75\xBE\xC5\xA3\xD6\x2D\xB4\x5C\xDB\xD6\x85\xF5\x99\x8C\xED\x50\xEA\x75\x63\xF8\x1D\xD1\xC3\xC2\xA1\x55\x8B\xA0\x74\xCA\x72\xB9\x76\xF0\x3F\xA7\x3D\xEA\x3C\x7E\x95\xB9\xF7\xCB\x86\xC4\x4C\x30\x51\xA6\x3E\x95\x2B\xCA\x4E\xC8\xB1\x96\x2C\xB2\x44\xE5\x5E\xBE\x1B\x59\xEB\xE2\xD4\x75\x41\x60\xDF\x44\x21\x25\x8A\x02\xD1\x61\x8C\x48\x84\x59\xA4\x43\x88\xEC\x18\xBE\x05\xED\x24\xBB\x1F\x4B\xAB\x68\x8B\x99\x83\x3B\x48\x83\x7D\x62\x78\x84\x3F\xD6\x49\xBE\xF3\x19\x0A\x5E\x60\x3C\x03\x92\x3A\xF6\xB7\x10\x12\xF7\xC5\xDF\x2F\xA8\xDA\x60\x03\x8C\x7F\x8A\x9E\xB8\x95\x4F\xDD\x1E\xD6\xE6\x5F\x6B\xC0\xF0\x29\x30\xE1\xFE\x8E\x2B\xA6\x31\xB5\xA9\x26\x5A\x60\xCE\xF7\x1D\x6E\xEC\x4C\x98\x6B\xE0\x7E\x40\x73\xBE\xE4\xBD\x14\xB3\xA4\x63\xE8\x5A\xBD\xB0\x3E\xF2\xE9\x6C\x63\x4D\xA5\x49\xCD\x44\xED\x6C\x2D\xBE\x0E\x7B\x82\x1B\x73\xCE\xE3\x99\x32\xE9\xB3\x0C\x40\xB5\xE7\xCA\xA6\x9E\x2A\x15\xD3\x60\x0A\x1B\x95\xF2\xAA\x79\x21\x88\x9B\xD2\x7C\x4C\xFA\x0E\x18\x22\x9A\xF7\xE8\x80\x8C\xDC\xFA\x49\xAD\x31\xF4\xA2\x2D\x02\xB9\x09\x6B\xC5\x00\xBD\xBB\xD1\x49\x0F\x3B\xA4\x4F\xB4\xB5\xF7\xFC\x7C\xA7\x90\xD9\xDF\x00\xC7\x6B\x76\xA9\x45\x47\x9E\xF4\xC5\xC9\x0A\x58\xF5\x60\xF9\x89\xC1\x0E\xD1\xBF\x46\x89\xF9\xB8\x38\xDB\xAE\x3D\x3C\x1D\x80\x14\xC7\xE4\xD4\xC5\x33\x2E\x5C\xE6\x5F\x38\xCA\x2B\xAF\xA7\x27\xBE\x26\xC4\xA6\x27\xAE\xD1\xD0\x45\xBB\x51\x99\x09\xB6\x84\x18\x6E\xF2\x39\x69\x64\x43\xCE\x1F\x24\xF9\x5A\x7D\x51\xDC\xF8\x3E\xBB\x8D\xAA\x79\x1F\x7D\xB0\x40\x82\x65\x95\xE6\xBA\xBB\x9A\xC7\x1C\x29\x35\x08\x24\x90\x62\xB4\xFC\xBE\xD3\xD7\x3A\x14\xD3\x8C\x45\x15\x4D\xD3\x38\xCF\xFD\x5C\x5B\xC4\x50\xCE\x60\xDD\xDB\xF3\x7D\x43\x4C\x9F\x79\x1D\x11\x9E\xB4\x78\x60\x12\x59\xFE\xA0\x6B\x5F\xB1\xEB\xE2\x1F\xB6\xBD\xB4\x59\x4D\x2E\xF7\x9D\x0A\x15\x7A\x84\x79\x14\x18\xA9\x25\xF5\x88\x91\xBC\x91\x29\xCE\x34\xB9\xF9\x50\x41\xCB\x83\x2E\x2E\x2B\x49\xFF\x45\xD5\x7F\x66\x1B\x5F\xB8\x9E\xB3\xED\x94\xEE\x76\x05\xB7\x23\x91\x96\x4B\x7B\xE2\x8C\x20'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 5
        exp_pkt = Mask(
            b'\x27\xE4\xC6\xF4\xFD\xB8\xA2\xC6\x00\x00\x00\x00\xB5\xF6\x00\x59\xB9\x45\x18\x6E\x39\xCF\xB1\xDF\x25\xFB\x43\xBB\x58\x9A\xA6\x4C\x39\xC4\x72\xEF\x51\x91\x52\x1F\x8F\x4A\x11\x64\xB5\xE4\x1C\x6D\xE5\xEC\x2D\x9F\x1D\x20\x8B\xAA\xEB\x9A\xD6\xFA\x80\xEE\x89\xBD\x1B\x41\xB0\xA4\x64\xCE\x7F\x42\x78\x5E\xD6\x99\x06\x40\xC7\xC2\x4A\x4E\x3C\x60\x11\xE7\x57\xBA\x7E\xF5\x52\x82\x46\x20\xFE\x7D\x18\xBA\x02\xC1\xBF\x02\x94\x64\x1A\xED\x69\x7B\x1D\xE6\x90\xBD\x56\xBA\x24\x47\x69\x3E\xB9\x26\x99\xAA\xE9\x66\xCA\x5C\xE9\xF6\x1F\x4D\x87\xE0\xD5\x6C\x69\x10\x48\x64\x64\xEE\xDA\x92\x05\x8E\x20\x33\xDD\xED\x71\xB4\xD4\x02\x5B\xDE\x51\xA7\xF9\xD0\x13\xE9\x4B\x78\x55\xA7\x01\x36\x8B\xA0\x4D\xA0\xD3\xF9\x88\x65\x99\x2D\x2D\x4A\x5B\x17\x04\xA4\x7E\x4D\x59\xBE\xAB\x69\x41\xCD\x88\x30\xC8\x8B\xC8\xD4\x74\xCC\x5A\x3C\x5D\x4D\x32\x3F\x49\xDE\x7B\xE7\xB3\x84\x5B\x01\x3E\xAE\x54\xB3\x90\xDF\x1C\x5C\x45\xA8\x52\x2C\x5F\x6C\x49\xA2\x44\x1B\x21\x81\x4F\x33\x6C\x3E\xF8\x8D\x1D\x21\xA2\xBE\x33\xCF\x9F\x0C\xE6\xAE\x24\x74\xB4\x1B\x72\x9E\xF6\xDA\xA6\xF8\x07\x45\x46\x3A\xEF\x1D\x97\xFF\x24\x39\xAA\xDA\x57\xCE\x69\x49\x1C\x69\x5D\x8A\x6D\x33\xB7\x62\x04\x4F\x93\xCA\xF3\xE9\x0A\x84\xE2\x0B\x71\x60\x76\x37\xB8\x70\xF9\xFE\x10\x4A\x04\x18\x20\x78\x7D\x2D\x10\x0B\x11\x59\x88\x6B\x09\x32\xB3\x9B\x79\xD8\x27\x5C\x4D\xD1\x7A\xDB\xF5\x65\x09\x85\x30\x60\x96\xCF\x15\xE9\x9C\xE2\xBD\x55\x7F\x34\xF9\xAF\xB4\xF1\x1F\x2A\x0F\x77\x19\x87\x7A\x81\x58\xE3\xE8\xC9\xC7\xE1\x6C\xA0\x50\x06\xDA\x34\x49\xDC\xC6\xE9\x85\xD4\x29\xF3\x29\x5F\x45\x1A\x87\xED\xEE\xD3\x83\x91\xAA\x6C\x8B\xD4\x20\xBB\x51\x76\x48\xEE\xDE\x7A\x6B\x99\x01\x8D\xF1\x11\xB5\x9B\x90\x92\x87\x0C\x7C\x71\x4C\x26\xC3\xD7\x0C\x1F\x18\xC5\x28\xE6\x70\x01\x20\x58\x74\x89\xE3\xFD\xB2\xEA\x2E\x71\xDF\x9E\xD4\x74\x2A\xD4\x34\x03\xDD\x57\x33\x82\x54\x94\x8F\xD8\x1A\xEC\x42\x91\x13\x5B\x0D\x22\x57\x5A\x26\x27\x86\x47\x04\x05\x78\x21\x34\x6D\x74\x76\xF2\x95\x21\xAE\xF1\x29\xED\xFA\xE9\x1A\x03\x30\xE1\xAC\x0E\x92\x0C\x43\xC8\xF4\xB6\xC6\xD6\xDF\xAB\x7A\x9D\x34\x94\x83\xC2\xBE\x76\xB5\xFE\x1F\xE6\x94\xDF\x1B\xD9\xC8\xD5\xDD\xBB\xF3\x5C\x4B\xA7\x50\xD3\xF4\x15\xE9\x8E\x95\x9B\x75\xBE\xC5\xA3\xD6\x2D\xB4\x5C\xDB\xD6\x85\xF5\x99\x8C\xED\x50\xEA\x75\x63\xF8\x1D\xD1\xC3\xC2\xA1\x55\x8B\xA0\x74\xCA\x72\xB9\x76\xF0\x3F\xA7\x3D\xEA\x3C\x7E\x95\xB9\xF7\xCB\x86\xC4\x4C\x30\x51\xA6\x3E\x95\x2B\xCA\x4E\xC8\xB1\x96\x2C\xB2\x44\xE5\x5E\xBE\x1B\x59\xEB\xE2\xD4\x75\x41\x60\xDF\x44\x21\x25\x8A\x02\xD1\x61\x8C\x48\x84\x59\xA4\x43\x88\xEC\x18\xBE\x05\xED\x24\xBB\x1F\x4B\xAB\x68\x8B\x99\x83\x3B\x48\x83\x7D\x62\x78\x84\x3F\xD6\x49\xBE\xF3\x19\x0A\x5E\x60\x3C\x03\x92\x3A\xF6\xB7\x10\x12\xF7\xC5\xDF\x2F\xA8\xDA\x60\x03\x8C\x7F\x8A\x9E\xB8\x95\x4F\xDD\x1E\xD6\xE6\x5F\x6B\xC0\xF0\x29\x30\xE1\xFE\x8E\x2B\xA6\x31\xB5\xA9\x26\x5A\x60\xCE\xF7\x1D\x6E\xEC\x4C\x98\x6B\xE0\x7E\x40\x73\xBE\xE4\xBD\x14\xB3\xA4\x63\xE8\x5A\xBD\xB0\x3E\xF2\xE9\x6C\x63\x4D\xA5\x49\xCD\x44\xED\x6C\x2D\xBE\x0E\x7B\x82\x1B\x73\xCE\xE3\x99\x32\xE9\xB3\x0C\x40\xB5\xE7\xCA\xA6\x9E\x2A\x15\xD3\x60\x0A\x1B\x95\xF2\xAA\x79\x21\x88\x9B\xD2\x7C\x4C\xFA\x0E\x18\x22\x9A\xF7\xE8\x80\x8C\xDC\xFA\x49\xAD\x31\xF4\xA2\x2D\x02\xB9\x09\x6B\xC5\x00\xBD\xBB\xD1\x49\x0F\x3B\xA4\x4F\xB4\xB5\xF7\xFC\x7C\xA7\x90\xD9\xDF\x00\xC7\x6B\x76\xA9\x45\x47\x9E\xF4\xC5\xC9\x0A\x58\xF5\x60\xF9\x89\xC1\x0E\xD1\xBF\x46\x89\xF9\xB8\x38\xDB\xAE\x3D\x3C\x1D\x80\x14\xC7\xE4\xD4\xC5\x33\x2E\x5C\xE6\x5F\x38\xCA\x2B\xAF\xA7\x27\xBE\x26\xC4\xA6\x27\xAE\xD1\xD0\x45\xBB\x51\x99\x09\xB6\x84\x18\x6E\xF2\x39\x69\x64\x43\xCE\x1F\x24\xF9\x5A\x7D\x51\xDC\xF8\x3E\xBB\x8D\xAA\x79\x1F\x7D\xB0\x40\x82\x65\x95\xE6\xBA\xBB\x9A\xC7\x1C\x29\x35\x08\x24\x90\x62\xB4\xFC\xBE\xD3\xD7\x3A\x14\xD3\x8C\x45\x15\x4D\xD3\x38\xCF\xFD\x5C\x5B\xC4\x50\xCE\x60\xDD\xDB\xF3\x7D\x43\x4C\x9F\x79\x1D\x11\x9E\xB4\x78\x60\x12\x59\xFE\xA0\x6B\x5F\xB1\xEB\xE2\x1F\xB6\xBD\xB4\x59\x4D\x2E\xF7\x9D\x0A\x15\x7A\x84\x79\x14\x18\xA9\x25\xF5\x88\x91\xBC\x91\x29\xCE\x34\xB9\xF9\x50\x41\xCB\x83\x2E\x2E\x2B\x49\xFF\x45\xD5\x7F\x66\x1B\x5F\xB8\x9E\xB3\xED\x94\xEE\x76\x05\xB7\x23\x91\x96\x4B\x7B\xE2\x8C\x20'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)

    def runTest(self):
        self.runTestImpl()


class Test5(AbstractTest):
    # Date generated: 2023-01-17-17:00:00.574
    # Current statement coverage: 0.83

    def setupCtrlPlane(self):
        self.table_add(
            (
                'ingress.ex1',
                [
                    self.Ternary('hdrs.extra[0].h', 0x0000, 0x0000),
                ],
            ),
            ('ingress.noop', []),
            1,
        )
        self.table_add(
            (
                'ingress.test1',
                [
                    self.Ternary('hdrs.data.f1', 0x6AFFF7F3, 0x507BD47B),
                ],
            ),
            (
                'ingress.setb1',
                [
                    ('port', 0x004),
                    ('val', 0x00),
                ],
            ),
            1,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\xC2\xFF\xF5\xF3\x00\x00\x00\x00\x00\x00\x00\x00\xFD\x34\x2A'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 4
        exp_pkt = Mask(b'\xC2\xFF\xF5\xF3\x00\x00\x00\x00\x00\x00\x00\x00\xFD\x34\x2A')
        ptfutils.verify_packet(self, exp_pkt, eg_port)

    def runTest(self):
        self.runTestImpl()


class Test6(AbstractTest):
    # Date generated: 2023-01-17-17:00:00.579
    # Current statement coverage: 1

    def setupCtrlPlane(self):
        self.table_add(
            (
                'ingress.ex1',
                [
                    self.Ternary('hdrs.extra[0].h', 0x0000, 0x0000),
                ],
            ),
            (
                'ingress.act1',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )
        self.table_add(
            (
                'ingress.tbl1',
                [
                    self.Ternary('hdrs.data.f2', 0xBF594DC7, 0x7B4A6D67),
                ],
            ),
            (
                'ingress.setbyte',
                [
                    ('val', 0x00),
                ],
            ),
            1,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\xBB\x6D\x5F\x57\x00\x00\x00\x00\x27\xED\x9C'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(b'\x00\x00\x00\x00\xBB\x6D\x5F\x57\x00\x00\x00\x00\x27\xED\x9C')
        ptfutils.verify_packet(self, exp_pkt, eg_port)

    def runTest(self):
        self.runTestImpl()
