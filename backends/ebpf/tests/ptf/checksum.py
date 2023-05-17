import random

from common import *
from scapy.layers.inet import IP, UDP
from scapy.layers.l2 import Ether

PORT0 = 0
PORT1 = 1
PORT2 = 2
ALL_PORTS = [PORT0, PORT1, PORT2]


@xdp2tc_head_not_supported
class ChecksumCRC32MultipleUpdatesPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/checksum-updates-crc32.p4"

    def runTest(self):
        pkt = Ether() / "1234567890000"
        exp_pkt = Ether() / bytes.fromhex("313233343536373839 cbf43926")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)


@xdp2tc_head_not_supported
class ChecksumCRC16MultipleUpdatesPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/checksum-updates-crc16.p4"

    def runTest(self):
        pkt = Ether() / "1234567890000"
        exp_pkt = Ether() / bytes.fromhex("313233343536373839 BB3D")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)


class InternetChecksumPSATest(P4EbpfTest):
    """
    Test if checksum in IP header (or any other using Ones Complement algorithm)
    is computed correctly.
    1. Generate IP packet with random values in header.
    2. Verify that packet is forwarded. Data plane will decrement TTL twice and change
     source IP address.
    """

    p4_file_path = "p4testdata/internet-checksum.p4"

    def random_ip(self):
        return ".".join(str(random.randint(0, 255)) for _ in range(4))

    def runTest(self):
        for _ in range(1):
            # test checksum computation
            pkt = testutils.simple_udp_packet(
                pktlen=random.randint(100, 512),
                ip_src=self.random_ip(),
                ip_dst=self.random_ip(),
                ip_ttl=random.randint(3, 255),
                ip_id=random.randint(0, 0xFFFF),
            )

            pkt[IP].flags = random.randint(0, 7)
            pkt[IP].frag = random.randint(0, 0x1FFF)
            testutils.send_packet(self, PORT0, pkt)
            pkt[IP].ttl = pkt[IP].ttl - 2
            pkt[IP].src = "10.0.0.1"
            pkt[IP].chksum = None
            pkt[UDP].chksum = None
            testutils.verify_packet_any_port(self, pkt, PTF_PORTS)


@xdp2tc_head_not_supported
class HashCRC16PSATest(P4EbpfTest):
    p4_file_path = "p4testdata/hash-crc16.p4"

    def runTest(self):
        pkt = Ether() / "12345678900"
        exp_pkt = Ether() / bytes.fromhex("313233343536373839 bb3d")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)


@xdp2tc_head_not_supported
class HashInActionPSATest(P4EbpfTest):
    p4_file_path = "p4testdata/hash-action.p4"

    def runTest(self):
        pkt = Ether() / "12345678900"
        exp_pkt = Ether() / bytes.fromhex("313233343536373839 bb3d")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)


@xdp2tc_head_not_supported
class HashCRC32PSATest(P4EbpfTest):
    p4_file_path = "p4testdata/hash-crc32.p4"

    def runTest(self):
        pkt = Ether() / "1234567890000"
        exp_pkt = Ether() / bytes.fromhex("313233343536373839 cbf43926")
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)


@xdp2tc_head_not_supported
class HashRangePSATest(P4EbpfTest):
    p4_file_path = "p4testdata/hash-range.p4"

    def runTest(self):
        res = 50 + (0xBB3D % 200)
        pkt = Ether() / "1234567890"
        exp_pkt = Ether() / bytes.fromhex("313233343536373839 {}".format(format(res, "x")))
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)


class HashWideField(P4EbpfTest):
    # Tests support for fields wider than 64 bits
    p4_file_path = "p4testdata/hash-wide-field.p4"

    def runTest(self):
        #      data0          data1        data2
        data = "1234567890" + "abcdefgh" + "XZ"
        pkt = Ether() / data / (" " * 50)  # data + room for check values
        #                                         crc32    crc16 ones_compl range
        exp_pkt = Ether() / data / bytes.fromhex("6f7bf033 12e9  0c0b       5b") / (" " * 41)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet_any_port(self, exp_pkt, PTF_PORTS)
