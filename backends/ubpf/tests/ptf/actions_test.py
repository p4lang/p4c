from ptf.mask import Mask
from ptf.packet import IP, Ether, TCP

from base_test import P4rtOVSBaseTest
from ptf.testutils import send_packet, verify_packets, simple_mpls_packet, simple_ip_only_packet, simple_ip_packet


class BaseActionsTest(P4rtOVSBaseTest):

    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/oko-test-actions.o")
        self.add_bpf_prog_flow(1,2)
        self.add_bpf_prog_flow(2,1)


class IpModifySrcAddressTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="8 0 0 0 5 1 168 192 0 0 0 0")

    def runTest(self):
        pkt = simple_ip_packet(ip_dst="192.168.1.1", ip_src="192.168.1.2")

        exp_pkt = simple_ip_packet(ip_dst="192.168.1.1", ip_src="192.168.1.5")

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, "chksum")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsModifyStackTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="5 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"s": 0}],
                                 inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"s": 1}],
                                     inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsDecrementTtlTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="0 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"ttl": 10}],
                                 inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"ttl": 9}],
                                     inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetLabelTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="1 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"label": 5}], inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"label": 1}], inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetLabelDecrementTtlTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="2 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"ttl": 10, "label": 5}],
                                 inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"ttl": 9, "label": 1}],
                                     inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetModifyTcTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="3 0 0 0 1 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"label": 5, "tc": 5}],
                                 inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"label": 5, "tc": 1}],
                                     inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class MplsSetLabelTcTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="4 0 0 0 2 0 0 0 2 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"label": 5, "tc": 5}],
                                 inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"label": 2, "tc": 2}],
                                     inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class ChangeIpVersionTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="6 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(dst="192.168.1.1", version=4) / TCP() / "Ala has a cat"

        exp_pkt = Ether() / IP(dst="192.168.1.1", version=6) / TCP() / "Ala has a cat"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'chksum')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class IpSwapAddressTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="7 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = Ether() / IP(dst="192.168.1.1", src="192.168.1.2") / TCP() / "Ala has a cat"

        exp_pkt = Ether() / IP(dst="192.168.1.2", src="192.168.1.1") / TCP() / "Ala has a cat"

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])


class NoActionPacketTest(BaseActionsTest):

    def setUp(self):
        BaseActionsTest.setUp(self)

        self.update_bpf_map(map_id=0, key="1 1 168 192", value="10 0 0 0 0 0 0 0 0 0 0 0")

    def runTest(self):
        pkt = simple_mpls_packet(mpls_tags=[{"label": 1}],
                                 inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1", ip_src="192.168.1.2"))

        exp_pkt = simple_mpls_packet(mpls_tags=[{"label": 1}],
                                     inner_frame=simple_ip_only_packet(ip_dst="192.168.1.1", ip_src="192.168.1.2"))

        mask = Mask(exp_pkt)
        mask.set_do_not_care_scapy(IP, 'ttl')

        send_packet(self, (0, 1), pkt)
        verify_packets(self, mask, device_number=0, ports=[2])
