from ptf.packet import MPLS, Ether

from base_test import P4rtOVSBaseTest
from ptf.testutils import send_packet, verify_packets, simple_ip_only_packet


class TunnelingTest(P4rtOVSBaseTest):

    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/tunneling.o")
        self.add_bpf_prog_flow(1,2)
        self.add_bpf_prog_flow(2,1)


class MplsDownstreamTest(TunnelingTest):

    def setUp(self):
        TunnelingTest.setUp(self)

        self.update_bpf_map(map_id=1, key="1 1 168 192", value="0 0 0 0")

    def runTest(self):
        pkt = Ether(dst="11:11:11:11:11:11") / simple_ip_only_packet(ip_dst="192.168.1.1")
        exp_pkt = Ether(dst="11:11:11:11:11:11") / MPLS(label=20, cos=5, s=1, ttl=64) / simple_ip_only_packet(
            ip_dst="192.168.1.1")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, exp_pkt, device_number=0, ports=[2])


class MplsUpstreamTest(TunnelingTest):

    def setUp(self):
        TunnelingTest.setUp(self)

        self.update_bpf_map(map_id=0, key="20 0 0 0", value="0 0 0 0")

    def runTest(self):
        pkt = Ether(dst="11:11:11:11:11:11") / MPLS(label=20, cos=5, s=1, ttl=64) / simple_ip_only_packet(
            ip_dst="192.168.1.1")
        exp_pkt = Ether(dst="11:11:11:11:11:11") / simple_ip_only_packet(ip_dst="192.168.1.1")

        send_packet(self, (0, 1), pkt)
        verify_packets(self, exp_pkt, device_number=0, ports=[2])
