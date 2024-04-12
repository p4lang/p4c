# P4Runtime PTF test for small_sample
# p4testgen seed: none

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
pipeline_pushed = False


class AbstractTest(bt.P4RuntimeTest):
    EnumColor = Enum("EnumColor", ["GREEN", "YELLOW", "RED"], start=0)

    def setUp(self):
        bt.P4RuntimeTest.setUp(self)
        global pipeline_pushed
        if not pipeline_pushed:
            success = bt.P4RuntimeTest.updateConfig(self)
            assert success
            pipeline_pushed = True
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


class Test0(AbstractTest):
    # Date generated: 2023-08-28-12:04:22.715
    '''
    # Current statement coverage: nan
    [Parser] MainParserImpl
    [State] start
    [ExtractSuccess] hdr.ethernet@0 | Condition: |*packetLen_bits(bit<32>)| >= 112; | Extract Size: 112 -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [State] parse_ipv4
    [ExtractSuccess] hdr.ipv4@112 | Condition: |*packetLen_bits(bit<32>)| >= 272; | Extract Size: 160 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [State] accept
    [Control PreControlImpl start]
    [Control MainControlImpl start]
    [Internal If Statement]: 1; -> 1; -> true
    [Table Branch: MainControlImpl.ipv4_da | Key(s): |pktvar_14(bit<32>)|| Chosen action: MainControlImpl.next_hop]
    [send_to_port executed]
    [Control MainDeparserImpl start]
    [Emit] hdr.ethernet -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [Emit] hdr.ipv4 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [Prepending the emit buffer to the program packet]
    [Internal If Statement]: 0; -> 0; -> false
    '''

    def setupCtrlPlane(self):
        # Simple noop that is always called as filler.
        pass
        self.table_add(
            (
                'MainControlImpl.ipv4_da',
                [
                    self.Exact('hdr.ipv4.dstAddr', 0x00000000),
                ],
            ),
            (
                'MainControlImpl.next_hop',
                [
                    ('vport', 0x00000000),
                ],
            ),
            None,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)

    def runTest(self):
        self.runTestImpl()


class Test1(AbstractTest):
    # Date generated: 2023-08-28-12:04:22.715
    '''
    # Current statement coverage: nan
    [Parser] MainParserImpl
    [State] start
    [ExtractSuccess] hdr.ethernet@0 | Condition: |*packetLen_bits(bit<32>)| >= 112; | Extract Size: 112 -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [State] parse_ipv4
    [ExtractSuccess] hdr.ipv4@112 | Condition: |*packetLen_bits(bit<32>)| >= 272; | Extract Size: 160 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [State] accept
    [Control PreControlImpl start]
    [Control MainControlImpl start]
    [Internal If Statement]: 1; -> 1; -> true
    [Table Branch: MainControlImpl.ipv4_da | Key(s): |pktvar_14(bit<32>)|| Chosen action: MainControlImpl.next_hop]
    [send_to_port executed]
    [Control MainDeparserImpl start]
    [Emit] hdr.ethernet -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [Emit] hdr.ipv4 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [Prepending the emit buffer to the program packet]
    [Internal If Statement]: 0; -> 0; -> false
    '''

    def setupCtrlPlane(self):
        # Simple noop that is always called as filler.
        pass
        self.table_add(
            (
                'MainControlImpl.ipv4_da',
                [
                    self.Exact('hdr.ipv4.dstAddr', 0x00000000),
                ],
            ),
            (
                'MainControlImpl.next_hop',
                [
                    ('vport', 0x00000000),
                ],
            ),
            None,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)

    def runTest(self):
        self.runTestImpl()


class Test2(AbstractTest):
    # Date generated: 2023-08-28-12:04:22.715
    '''
    # Current statement coverage: nan
    [Parser] MainParserImpl
    [State] start
    [ExtractSuccess] hdr.ethernet@0 | Condition: |*packetLen_bits(bit<32>)| >= 112; | Extract Size: 112 -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [State] parse_ipv4
    [ExtractSuccess] hdr.ipv4@112 | Condition: |*packetLen_bits(bit<32>)| >= 272; | Extract Size: 160 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [State] accept
    [Control PreControlImpl start]
    [Control MainControlImpl start]
    [Internal If Statement]: 1; -> 1; -> true
    [Table Branch: MainControlImpl.ipv4_da | Key(s): |pktvar_14(bit<32>)|| Chosen action: MainControlImpl.next_hop]
    [send_to_port executed]
    [Control MainDeparserImpl start]
    [Emit] hdr.ethernet -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [Emit] hdr.ipv4 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [Prepending the emit buffer to the program packet]
    [Internal If Statement]: 0; -> 0; -> false
    '''

    def setupCtrlPlane(self):
        # Simple noop that is always called as filler.
        pass
        self.table_add(
            (
                'MainControlImpl.ipv4_da',
                [
                    self.Exact('hdr.ipv4.dstAddr', 0x00000000),
                ],
            ),
            (
                'MainControlImpl.next_hop',
                [
                    ('vport', 0x00000000),
                ],
            ),
            None,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)

    def runTest(self):
        self.runTestImpl()


class Test3(AbstractTest):
    # Date generated: 2023-08-28-12:04:22.715
    '''
    # Current statement coverage: nan
    [Parser] MainParserImpl
    [State] start
    [ExtractSuccess] hdr.ethernet@0 | Condition: |*packetLen_bits(bit<32>)| >= 112; | Extract Size: 112 -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [State] parse_ipv4
    [ExtractSuccess] hdr.ipv4@112 | Condition: |*packetLen_bits(bit<32>)| >= 272; | Extract Size: 160 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [State] accept
    [Control PreControlImpl start]
    [Control MainControlImpl start]
    [Internal If Statement]: 1; -> 1; -> true
    [Table Branch: MainControlImpl.ipv4_da | Key(s): |pktvar_14(bit<32>)|| Chosen action: MainControlImpl.next_hop]
    [send_to_port executed]
    [Control MainDeparserImpl start]
    [Emit] hdr.ethernet -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [Emit] hdr.ipv4 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [Prepending the emit buffer to the program packet]
    [Internal If Statement]: 0; -> 0; -> false
    '''

    def setupCtrlPlane(self):
        # Simple noop that is always called as filler.
        pass
        self.table_add(
            (
                'MainControlImpl.ipv4_da',
                [
                    self.Exact('hdr.ipv4.dstAddr', 0x00000000),
                ],
            ),
            (
                'MainControlImpl.next_hop',
                [
                    ('vport', 0x00000000),
                ],
            ),
            None,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)

    def runTest(self):
        self.runTestImpl()


class Test4(AbstractTest):
    # Date generated: 2023-08-28-12:04:22.715
    '''
    # Current statement coverage: nan
    [Parser] MainParserImpl
    [State] start
    [ExtractSuccess] hdr.ethernet@0 | Condition: |*packetLen_bits(bit<32>)| >= 112; | Extract Size: 112 -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [State] parse_ipv4
    [ExtractSuccess] hdr.ipv4@112 | Condition: |*packetLen_bits(bit<32>)| >= 272; | Extract Size: 160 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [State] accept
    [Control PreControlImpl start]
    [Control MainControlImpl start]
    [Internal If Statement]: 1; -> 1; -> true
    [Table Branch: MainControlImpl.ipv4_da | Key(s): |pktvar_14(bit<32>)|| Chosen action: MainControlImpl.next_hop]
    [send_to_port executed]
    [Control MainDeparserImpl start]
    [Emit] hdr.ethernet -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [Emit] hdr.ipv4 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [Prepending the emit buffer to the program packet]
    [Internal If Statement]: 0; -> 0; -> false
    '''

    def setupCtrlPlane(self):
        # Simple noop that is always called as filler.
        pass
        self.table_add(
            (
                'MainControlImpl.ipv4_da',
                [
                    self.Exact('hdr.ipv4.dstAddr', 0x00000000),
                ],
            ),
            (
                'MainControlImpl.next_hop',
                [
                    ('vport', 0x00000000),
                ],
            ),
            None,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)

    def runTest(self):
        self.runTestImpl()


class Test5(AbstractTest):
    # Date generated: 2023-08-28-12:04:22.715
    '''
    # Current statement coverage: nan
    [Parser] MainParserImpl
    [State] start
    [ExtractSuccess] hdr.ethernet@0 | Condition: |*packetLen_bits(bit<32>)| >= 112; | Extract Size: 112 -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [State] parse_ipv4
    [ExtractSuccess] hdr.ipv4@112 | Condition: |*packetLen_bits(bit<32>)| >= 272; | Extract Size: 160 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [State] accept
    [Control PreControlImpl start]
    [Control MainControlImpl start]
    [Internal If Statement]: 1; -> 1; -> true
    [Table Branch: MainControlImpl.ipv4_da | Key(s): |pktvar_14(bit<32>)|| Chosen action: MainControlImpl.next_hop]
    [send_to_port executed]
    [Control MainDeparserImpl start]
    [Emit] hdr.ethernet -> hdr.ethernet.dstAddr = 0x0000_0000_0000 | hdr.ethernet.srcAddr = 0x0000_0000_0000 | hdr.ethernet.etherType = 0x0800
    [Emit] hdr.ipv4 -> hdr.ipv4.version = 0x0 | hdr.ipv4.ihl = 0x0 | hdr.ipv4.diffserv = 0x00 | hdr.ipv4.totalLen = 0x0000 | hdr.ipv4.identification = 0x0000 | hdr.ipv4.flags = 0x0 | hdr.ipv4.fragOffset = 0x0000 | hdr.ipv4.ttl = 0x00 | hdr.ipv4.protocol = 0x00 | hdr.ipv4.hdrChecksum = 0x0000 | hdr.ipv4.srcAddr = 0x0000_0000 | hdr.ipv4.dstAddr = 0x0000_0000
    [Prepending the emit buffer to the program packet]
    [Internal If Statement]: 0; -> 0; -> false
    '''

    def setupCtrlPlane(self):
        # Simple noop that is always called as filler.
        pass
        self.table_add(
            (
                'MainControlImpl.ipv4_da',
                [
                    self.Exact('hdr.ipv4.dstAddr', 0x00000000),
                ],
            ),
            (
                'MainControlImpl.next_hop',
                [
                    ('vport', 0x00000000),
                ],
            ),
            None,
        )

    def sendPacket(self):
        ig_port = 0
        pkt = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        ptfutils.send_packet(self, ig_port, pkt)

    def verifyPackets(self):
        eg_port = 0
        exp_pkt = Mask(
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        )
        ptfutils.verify_packet(self, exp_pkt, eg_port)
        bt.testutils.log.info("Verifying no other packets ...")
        ptfutils.verify_no_other_packets(self, self.device_id, timeout=self.packet_wait_time)

    def runTest(self):
        self.runTestImpl()
