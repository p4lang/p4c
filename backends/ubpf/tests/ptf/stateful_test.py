from ptf.packet import TCP, IP, Ether

from base_test import P4rtOVSBaseTest
from ptf.testutils import send_packet, verify_packets, verify_no_packet


class StatefulTest(P4rtOVSBaseTest):

    def setUp(self):
        P4rtOVSBaseTest.setUp(self)

        self.del_flows()
        self.unload_bpf_program()
        self.load_bpf_program(path_to_program="build/oko-test-stateful.o")
        self.add_bpf_prog_flow(1, 2)
        self.add_bpf_prog_flow(2, 1)

    def send_client_packet(self, packet):
        send_packet(self, (0, 1), packet)
        verify_packets(self, packet, device_number=0, ports=[2])

    def send_client_packet_no_receive(self, packet):
        send_packet(self, (0, 1), packet)
        verify_no_packet(self, packet, port_id=2)

    def send_server_packet(self, packet):
        send_packet(self, (0, 2), packet)
        verify_packets(self, packet, device_number=0, ports=[1])

    def send_server_packet_no_receive(self, packet):
        send_packet(self, (0, 2), packet)
        verify_no_packet(self, packet, port_id=1)

    def verify_connection_state(self, conn_state, conn_srv="1 1 168 192"):
        conn_state_dump_map = self.dump_bpf_map(map_id=0)
        conn_srv_dump_map = self.dump_bpf_map(map_id=1)

        conn_state_key = conn_state_dump_map.split("\n")[2]
        conn_state_value = conn_state_dump_map.split("\n")[3]

        assert ("172 192 20 5" in conn_state_key)
        assert ("%s 0 0 0" % conn_state in conn_state_value)

        conn_srv_key = conn_srv_dump_map.split("\n")[2]
        conn_srv_value = conn_srv_dump_map.split("\n")[3]

        assert ("172 192 20 5" in conn_srv_key)
        assert (conn_srv in conn_srv_value)


class TcpSynStateTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)

        self.verify_connection_state(conn_state=1)


class TcpSynAckStateTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")
        server_packet = Ether() / IP(src="192.168.1.1", dst="192.168.1.10")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)
        # TCP syn ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="SA", chksum=0)
        self.send_server_packet(packet=pkt)

        self.verify_connection_state(conn_state=2)


class TcpEstablishedStateTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")
        server_packet = Ether() / IP(src="192.168.1.1", dst="192.168.1.10")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)
        # TCP syn ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="SA", chksum=0)
        self.send_server_packet(packet=pkt)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet(packet=pkt)

        self.verify_connection_state(conn_state=3)


class TcpServerFinAckStateTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")
        server_packet = Ether() / IP(src="192.168.1.1", dst="192.168.1.10")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)
        # TCP syn ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="SA", chksum=0)
        self.send_server_packet(packet=pkt)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet(packet=pkt)
        # TCP fin ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="FA", chksum=0)
        self.send_server_packet(packet=pkt)

        self.verify_connection_state(conn_state=0, conn_srv="0 0 0 0")


class SimpleTcpConnectionFlowTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")
        server_packet = Ether() / IP(src="192.168.1.1", dst="192.168.1.10")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)
        self.verify_connection_state(conn_state=1)
        # TCP syn ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="SA", chksum=0)
        self.send_server_packet(packet=pkt)
        self.verify_connection_state(conn_state=2)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet(packet=pkt)
        self.verify_connection_state(conn_state=3)
        # TCP data client
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0) / "Data data data"
        self.send_client_packet(packet=pkt)
        # TCP data server
        pkt = server_packet / TCP(sport=55, dport=80, flags="A", chksum=0) / "Data data data"
        self.send_server_packet(packet=pkt)
        # TCP fin ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="FA", chksum=0)
        self.send_client_packet(packet=pkt)
        self.verify_connection_state(conn_state=0, conn_srv="0 0 0 0")
        # TCP ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_server_packet(packet=pkt)
        # TCP fin ack
        pkt = server_packet / TCP(sport=55, dport=80, flags="FA", chksum=0)
        self.send_server_packet(packet=pkt)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet(packet=pkt)


class SimpleTcpAckAttackFlowTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet_no_receive(packet=pkt)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet_no_receive(packet=pkt)
        # TCP ack
        pkt = client_packet / TCP(sport=55, dport=80, flags="A", chksum=0)
        self.send_client_packet_no_receive(packet=pkt)


class SimpleTcpSynAttackFlowTest(StatefulTest):

    def setUp(self):
        StatefulTest.setUp(self)

        self.update_bpf_map(map_id=0, key="107 114 46 7", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="107 114 46 7", value="0 0 0 0")

        self.update_bpf_map(map_id=0, key="172 192 20 5", value="0 0 0 0")
        self.update_bpf_map(map_id=1, key="172 192 20 5", value="0 0 0 0")

    def runTest(self):
        client_packet = Ether() / IP(src="192.168.1.10", dst="192.168.1.1")

        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet(packet=pkt)
        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet_no_receive(packet=pkt)
        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet_no_receive(packet=pkt)
        # TCP syn
        pkt = client_packet / TCP(sport=55, dport=80, flags="S", chksum=0)
        self.send_client_packet_no_receive(packet=pkt)
