#!/usr/bin/env python
# Copyright 2022-present Orange
# Copyright 2022-present Open Networking Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from common import *

from scapy.contrib.mpls import MPLS
from scapy.layers.inet import IP, UDP, TCP
from scapy.layers.l2 import Ether, Dot1Q
from scapy.layers.ppp import PPPoE, PPP

ETH_TYPE_ARP = 0x0806
ETH_TYPE_IPV4 = 0x0800
ETH_TYPE_VLAN = 0x8100
ETH_TYPE_QINQ = 0x88a8
ETH_TYPE_PPPOE = 0x8864
ETH_TYPE_MPLS_UNICAST = 0x8847

PPPOE_CODE_SESSION_STAGE = 0x00

PPPOED_CODE_PADI = 0x09
PPPOED_CODE_PADO = 0x07
PPPOED_CODE_PADR = 0x19
PPPOED_CODE_PADS = 0x65
PPPOED_CODE_PADT = 0xa7

PPPOED_CODES = (
    PPPOED_CODE_PADI,
    PPPOED_CODE_PADO,
    PPPOED_CODE_PADR,
    PPPOED_CODE_PADS,
    PPPOED_CODE_PADT,
)

PORT_TYPE_OTHER     = 0x00
PORT_TYPE_EDGE      = 0x01
PORT_TYPE_INFRA     = 0x02
PORT_TYPE_INTERNAL  = 0x03

FORWARDING_TYPE_BRIDGING = 0
FORWARDING_TYPE_UNICAST_IPV4 = 2
FORWARDING_TYPE_MPLS = 1

DEFAULT_VLAN = 4096
HOST1_MAC = "00:00:00:00:00:01"
SWITCH_MAC = "01:02:03:04:05:06"
VLAN_ID_3 = 300
MPLS_LABEL_2 = 200
DEFAULT_MPLS_TTL = 64
CLIENT_IP = "10.0.0.1"
SERVER_IP = "192.168.0.2"

s_tag = vlan_id_outer = 888
c_tag = vlan_id_inner = 777
line_id = 99
pppoe_session_id = 0xbeac
core_router_mac = HOST1_MAC

PORT0 = 0
PORT1 = 1


def pkt_route(pkt, mac_dst):
    new_pkt = pkt.copy()
    new_pkt[Ether].src = pkt[Ether].dst
    new_pkt[Ether].dst = mac_dst
    return new_pkt


def pkt_add_vlan(pkt, vlan_vid=10, vlan_pcp=0, dl_vlan_cfi=0):
    return Ether(src=pkt[Ether].src, dst=pkt[Ether].dst) / \
           Dot1Q(prio=vlan_pcp, id=dl_vlan_cfi, vlan=vlan_vid) / \
           pkt[Ether].payload


def pkt_add_inner_vlan(pkt, vlan_vid=10, vlan_pcp=0, dl_vlan_cfi=0):
    assert Dot1Q in pkt
    return Ether(src=pkt[Ether].src, dst=pkt[Ether].dst, type=ETH_TYPE_VLAN) / \
           Dot1Q(prio=pkt[Dot1Q].prio, id=pkt[Dot1Q].id, vlan=pkt[Dot1Q].vlan) / \
           Dot1Q(prio=vlan_pcp, id=dl_vlan_cfi, vlan=vlan_vid) / \
           pkt[Dot1Q].payload


def pkt_add_pppoe(pkt, type, code, session_id):
    return Ether(src=pkt[Ether].src, dst=pkt[Ether].dst) / \
           PPPoE(version=1, type=type, code=code, sessionid=session_id) / \
           PPP(proto=0x0021) / pkt[Ether].payload


def pkt_add_mpls(pkt, label, ttl, cos=0, s=1):
    return Ether(src=pkt[Ether].src, dst=pkt[Ether].dst) / \
           MPLS(label=label, cos=cos, s=s, ttl=ttl) / \
           pkt[Ether].payload


def pkt_decrement_ttl(pkt):
    if IP in pkt:
        pkt[IP].ttl -= 1
    return pkt


class BNGTest(P4EbpfTest):

    p4_file_path = "../psa/examples/bng.p4"
    session_installed = False

    def setUp(self):
        super(BNGTest, self).setUp()

    def tearDown(self):
        super(BNGTest, self).tearDown()

    def setup_port(self, port_id, vlan_id, port_type, double_tagged=False, inner_vlan_id=0):
        if double_tagged:
            self.set_ingress_port_vlan(ingress_port=port_id, vlan_id=vlan_id,
                                       vlan_valid=True, inner_vlan_id=inner_vlan_id, port_type=port_type)
        else:
            self.set_ingress_port_vlan(ingress_port=port_id,
                                       vlan_valid=False, internal_vlan_id=vlan_id, port_type=port_type)
            self.set_egress_vlan(egress_port=port_id, vlan_id=vlan_id, push_vlan=False)

    def set_ingress_port_vlan(self, ingress_port,
                              vlan_valid=False,
                              vlan_id=0,
                              internal_vlan_id=0,
                              inner_vlan_id=None,
                              port_type=PORT_TYPE_EDGE,
                              ):
        vlan_valid_ = 1 if vlan_valid else 0
        if vlan_valid:
            action_id = 2 # permit
            action_data = [port_type]
        else:
            action_id = 3 # permit_with_internal_vlan
            action_data = [vlan_id, port_type]

        key_vlan_id = "{}^0xffff".format(vlan_id) if vlan_valid else "0^0"
        key_inner_vlan_id = "{}^0xffff".format(inner_vlan_id) if inner_vlan_id is not None else "0^0"
        keys = [ingress_port, key_vlan_id, key_inner_vlan_id, vlan_valid_]
        self.table_add(table="ingress_ingress_port_vlan", key=keys, action=action_id, data=action_data)

    def set_egress_vlan(self, egress_port, vlan_id, push_vlan=False):
        action_id = 1 if push_vlan else 2
        self.table_add(table="egress_egress_vlan", key=[vlan_id, egress_port],
                       action=action_id)

    def set_forwarding_type(self, ingress_port, eth_dstAddr, ethertype=ETH_TYPE_IPV4,
                            fwd_type=FORWARDING_TYPE_UNICAST_IPV4):
        if ethertype == ETH_TYPE_IPV4:
            key_eth_type = "0^0"
            key_ip_eth_type = ETH_TYPE_IPV4
        elif ethertype == ETH_TYPE_PPPOE:
            key_eth_type = "{}^0xffff".format(ETH_TYPE_PPPOE)
            key_ip_eth_type = ETH_TYPE_IPV4
        elif ethertype == ETH_TYPE_MPLS_UNICAST:
            key_eth_type = "{}^0xffff".format(ETH_TYPE_MPLS_UNICAST)
            key_ip_eth_type = ETH_TYPE_IPV4
        key_eth_dst = "{}^0xffffffffffff".format(eth_dstAddr) if eth_dstAddr is not None else "0^0"

        matches = [key_eth_dst, ingress_port, key_eth_type, key_ip_eth_type]

        self.table_add(table="ingress_fwd_classifier", key=matches, action=1, # set_forwarding_type
                       data=[fwd_type])

    def add_forwarding_routing_v4_entry(self, ipv4_dstAddr, ipv4_pLen,
                                        egress_port, smac, dmac):
        self.table_add(table="ingress_routing_v4", key=["{}/{}".format(ipv4_dstAddr, ipv4_pLen)],
                       action=1, data=[egress_port, smac, dmac])

    def add_next_vlan(self, port_id, new_vlan_id):
        self.table_add(table="ingress_next_vlan", key=[port_id],
                       action=1, data=[new_vlan_id])

    def add_next_double_vlan(self, port_id, new_vlan_id, new_inner_vlan_id):
        self.table_add(table="ingress_next_vlan", key=[port_id],
                       action=2, data=[new_vlan_id, new_inner_vlan_id])

    def set_upstream_pppoe_cp_table(self, pppoe_codes=()):
        for code in pppoe_codes:
            self.table_add(table="ingress_t_pppoe_cp", key=[code], action=1, priority=1)

    def set_line_map(self, s_tag, c_tag, line_id):
        assert line_id != 0
        self.table_add(table="ingress_t_line_map", key=[s_tag, c_tag], action=1, data=[line_id])

    def setup_line_v4(self, s_tag, c_tag, line_id, ipv4_addr,
                      pppoe_session_id, enabled=True):
        assert s_tag != 0
        assert c_tag != 0
        assert line_id != 0
        assert pppoe_session_id != 0

        # line map common to up and downstream
        self.set_line_map(s_tag=s_tag, c_tag=c_tag, line_id=line_id)
        # Upstream
        if enabled:
            # Enable upstream termination.
            self.table_add(table="ingress_t_pppoe_term_v4", key=[line_id, str(ipv4_addr), pppoe_session_id], action=1)

        # Downstream
        if enabled:
            self.table_add(table="ingress_t_line_session_map", key=[line_id],
                           action=1, data=[pppoe_session_id])
        else:
            self.table_add(table="ingress_t_line_session_map", key=[line_id],
                           action=2)


class PPPoEUpstreamTest(BNGTest):

    def doRunTest(self, pkt):
        # Input is the given packet with double VLAN tags and PPPoE headers.
        pppoe_pkt = pkt_add_pppoe(pkt, type=1, code=PPPOE_CODE_SESSION_STAGE,
                                  session_id=pppoe_session_id)
        pppoe_pkt = pkt_add_vlan(pppoe_pkt, vlan_vid=vlan_id_outer)
        pppoe_pkt = pkt_add_inner_vlan(pppoe_pkt, vlan_vid=vlan_id_inner)

        # Build expected packet from the input one, we expect it to be routed as
        # if it was without VLAN tags and PPPoE headers.
        exp_pkt = pkt.copy()
        exp_pkt = pkt_route(exp_pkt, core_router_mac)
        exp_pkt = pkt_decrement_ttl(exp_pkt)

        testutils.send_packet(self, PORT0, pppoe_pkt)
        testutils.verify_packet(self, exp_pkt, PORT1)
        testutils.verify_no_other_packets(self)

    def runTest(self):
        self.set_upstream_pppoe_cp_table(PPPOED_CODES)
        self.setup_line_v4(
            s_tag=s_tag, c_tag=c_tag, line_id=line_id, ipv4_addr=CLIENT_IP,
            pppoe_session_id=pppoe_session_id, enabled=True)
        self.session_installed = True
        # Setup port 1: packets on this port are double tagged packets
        self.setup_port(4, vlan_id=s_tag, port_type=PORT_TYPE_EDGE, double_tagged=True, inner_vlan_id=c_tag)
        # Setup port 2
        self.setup_port(5, vlan_id=s_tag, port_type=PORT_TYPE_INFRA)

        self.set_forwarding_type(4, SWITCH_MAC, ETH_TYPE_PPPOE,
                                 FORWARDING_TYPE_UNICAST_IPV4)
        self.add_forwarding_routing_v4_entry(SERVER_IP, 24, 5, SWITCH_MAC, HOST1_MAC)
        self.add_next_vlan(5, s_tag)
        print("")
        for pkt_type in ["tcp", "udp", "icmp"]:
            print("Testing %s packet..." \
                  % pkt_type)
            pkt = getattr(testutils, "simple_%s_packet" % pkt_type)(
                pktlen=120)
            pkt[Ether].dst = SWITCH_MAC
            pkt[IP].src = CLIENT_IP
            self.doRunTest(pkt)


class PPPoEDownstreamTest(BNGTest):

    def doRunTest(self, pkt):
        # Build expected packet from the input one, we expect it to be routed
        # and encapsulated in double VLAN tags and PPPoE.
        exp_pkt = pkt_add_pppoe(pkt, type=1, code=PPPOE_CODE_SESSION_STAGE, session_id=pppoe_session_id)
        exp_pkt = pkt_add_vlan(exp_pkt, vlan_vid=vlan_id_outer)
        exp_pkt = pkt_add_inner_vlan(exp_pkt, vlan_vid=vlan_id_inner)
        exp_pkt = pkt_route(exp_pkt, HOST1_MAC)
        exp_pkt = pkt_decrement_ttl(exp_pkt)

        testutils.send_packet(self, PORT1, pkt)
        testutils.verify_packet(self, exp_pkt, PORT0)
        testutils.verify_no_other_packets(self)

    def runTest(self):
        # Setup port 1: packets on this port are double tagged packets
        self.setup_port(4, vlan_id=VLAN_ID_3, port_type=PORT_TYPE_EDGE, double_tagged=True, inner_vlan_id=c_tag)
        # Setup port 2
        self.setup_port(5, vlan_id=s_tag, port_type=PORT_TYPE_INFRA)
        self.set_forwarding_type(5, SWITCH_MAC, ETH_TYPE_IPV4,
                                 FORWARDING_TYPE_UNICAST_IPV4)
        self.add_forwarding_routing_v4_entry(CLIENT_IP, 24, 4, SWITCH_MAC, HOST1_MAC)
        self.add_next_double_vlan(4, s_tag, c_tag)
        self.setup_line_v4(
            s_tag=s_tag, c_tag=c_tag, line_id=line_id, ipv4_addr=CLIENT_IP,
            pppoe_session_id=pppoe_session_id, enabled=True)
        self.meter_update(name="ingress_m_besteff", index=line_id,
                          pir=250000, pbs=2500, cir=250000, cbs=2500)

        print("")
        for pkt_type in ["tcp", "udp", "icmp"]:
            print("Testing %s packet..." \
                  % pkt_type)
            pkt = getattr(testutils, "simple_%s_packet" % pkt_type)(
                pktlen=120)
            pkt[Ether].dst = SWITCH_MAC
            pkt[IP].dst = CLIENT_IP
            self.doRunTest(pkt)
