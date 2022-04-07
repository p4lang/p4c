from common import *

import random

from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP

PORT0 = 0
PORT1 = 1
PORT2 = 2
ALL_PORTS = [PORT0, PORT1, PORT2]


@xdp2tc_head_not_supported
class ChecksumCRC32MultipleUpdatesPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/checksum-updates-crc32.p4"

    def runTest(self):
        pkt = Ether() / "1234567890000"
        exp_pkt = Ether() / bytes.fromhex('313233343536373839 cbf43926')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, ALL_PORTS)


@xdp2tc_head_not_supported
class ChecksumCRC16MultipleUpdatesPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/checksum-updates-crc16.p4"

    def runTest(self):
        pkt = Ether() / "1234567890000"
        exp_pkt = Ether() / bytes.fromhex('313233343536373839 BB3D')
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, ALL_PORTS)
