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

from scapy.contrib.gtp import GTP_U_Header
from scapy.layers.inet import IP, UDP
from scapy.layers.l2 import Ether

GTPU_UDP_PORT=2152

#link number
N3_PORT=4
N6_PORT=5
N9_PORT=6

#PTF
N3_PTF_PORT=0
N6_PTF_PORT=1
N9_PTF_PORT=2

# source/destination interfaces of the UPF
ACCESS = 0
CORE= 1
SGi_LAN = 2

#MAC addresses
UPF_N3_MAC = "00:00:00:00:00:01"
UPF_N6_MAC = "00:00:00:00:00:02"
UPF_N9_MAC = "00:00:00:00:00:03"

UPLINK_N6_ROUTER_MAC="00:00:00:00:00:04"
UPLINK_N9_ROUTER_MAC="00:00:00:00:00:05"
DOWNLINK_N3_ROUTER_MAC="00:00:00:00:00:06"

UPF_N3_IP="172.20.16.105"
UPF_N6_IP="172.20.16.106"
UPF_N9_IP="172.22.16.99"
RAN_N3_IP= "172.20.16.99"
CORE_UPF_N9_IP="172.55.8.9"


SEID=0
UL_N6_TEID=1234
UL_N9_TEID=5678
DL_TEID=4321
UE_IP="10.10.10.10"
UPLINK_N6_FAR_FORWARD_ID=12
UPLINK_N9_FAR_FORWARD_ID=14
DOWNLINK_FAR_ENCAP_FORWARD_ID=13

SERVER1_IP="192.168.2.21"
SERVER2_IP="5.5.5.5"
SERVER2_UDP_PORT=6970

def pkt_gtpu_encap(pkt,teid,ip_dst,ip_src):
    return Ether(src=pkt[Ether].src, dst=pkt[Ether].dst) / \
           IP(src=ip_src,dst=ip_dst)/UDP(dport=GTPU_UDP_PORT)/GTP_U_Header(teid=teid)/pkt[Ether].payload

def pkt_route(pkt, mac_src, mac_dst):
    new_pkt = pkt.copy()
    new_pkt[Ether].src = mac_src
    new_pkt[Ether].dst = mac_dst
    return new_pkt

class UPFTest(P4EbpfTest):
    p4_file_path = "../psa/examples/upf.p4"

    def setup_pfcp_session(self,seid, teid, ue_ip):
        self.table_add(table="ingress_upf_ingress_session_lookup_by_teid", key=[teid], action=1,data=[seid])
        self.table_add(table="ingress_upf_ingress_session_lookup_by_ue_ip", key=[ue_ip], action=1,data=[seid])

    def runTest(self):
        self.table_add(table="ingress_upf_ingress_source_interface_lookup_by_port", key=[N3_PORT], action=1, data=[ACCESS])
        self.table_add(table="ingress_upf_ingress_source_interface_lookup_by_port", key=[N9_PORT], action=1, data=[CORE])
        self.table_add(table="ingress_upf_ingress_source_interface_lookup_by_port", key=[N6_PORT], action=1, data=[SGi_LAN])

        self.table_add(table="ingress_ip_forward_ipv4_lpm", key=[SGi_LAN,"0/0"], action=1,
                       data=[UPF_N6_MAC,UPLINK_N6_ROUTER_MAC,N6_PORT])
        self.table_add(table="ingress_ip_forward_ipv4_lpm", key=[ACCESS,"172.20.16.99/32"], action=1,
                       data=[UPF_N3_MAC,DOWNLINK_N3_ROUTER_MAC,N3_PORT])
        self.table_add(table="ingress_ip_forward_ipv4_lpm", key=[CORE,"0/0"], action=1,
                       data=[UPF_N9_MAC,UPLINK_N9_ROUTER_MAC,N9_PORT])

        # setup unique PFCP session
        self.setup_pfcp_session(SEID,UL_N6_TEID,UE_IP)

        # add PDR+FAR rules for uplink N3->N6 traffic: decap GTPU-encapped packet and forward it to SGiLAN dest
        # PDR SDF filter set to passthrough: 'permit out ip from 0.0.0.0/0  to assigned'
        self.table_add(table="ingress_upf_ingress_pdr_lookup",
                       key=[SEID,"{}^0xffffffff".format(UE_IP),"0^0","0^0","0^0","0^0",ACCESS],
                       action=1,data=[UPLINK_N6_FAR_FORWARD_ID],priority=1)
        self.table_add(table="ingress_upf_ingress_far_lookup", key=[UPLINK_N6_FAR_FORWARD_ID], action=1,data=[SGi_LAN])

        # add PDR+FAR rules for uplink N3->N9 traffic: decap N3 GTPU, encap N9 GTPU and forward to N9 (core UPF)
        # PDR SDF filter set to: 'permit out 17 from 5.5.5.5/32 6970 to assigned'
        # this PDR has higher priority than the previous one
        self.table_add(table="ingress_upf_process_ingress_l4port_ingress_l4_dst_port", key=[SERVER2_UDP_PORT], action=2,data=[1])
        self.table_add(table="ingress_upf_process_ingress_l4port_ingress_l4_src_port", key=[SERVER2_UDP_PORT], action=2,data=[1])
        self.table_add(table="ingress_upf_ingress_pdr_lookup",
                       key=[SEID,"{}^0xffffffff".format(UE_IP),"{}^0xffffffff".format(SERVER2_IP),"0x11^0xff","0^0","0x01^0xff",ACCESS],
                       action=1,data=[UPLINK_N9_FAR_FORWARD_ID],priority=2)
        self.table_add(table="ingress_upf_ingress_far_lookup", key=[UPLINK_N9_FAR_FORWARD_ID], action=2,data=[CORE,UL_N9_TEID,CORE_UPF_N9_IP,UPF_N9_IP])

        # add PDR+FAR rules for downlink N6->N3 traffic: encap packet and forward it to basestation
        self.table_add(table="ingress_upf_ingress_pdr_lookup",
                       key=[SEID,"0^0","{}^0xffffffff".format(UE_IP),"0^0","0^0","0^0",SGi_LAN],
                       action=1,data=[DOWNLINK_FAR_ENCAP_FORWARD_ID])
        self.table_add(table="ingress_upf_ingress_far_lookup", key=[DOWNLINK_FAR_ENCAP_FORWARD_ID], action=2,
                       data=[ACCESS,DL_TEID,RAN_N3_IP,UPF_N3_IP])

        # uplink Access(N3) -> SgiLAN(N6) test
        # send GTPU-encapped packet to N3, expect inner packet from N6
        pkt=testutils.simple_udp_packet(eth_dst=UPF_N3_MAC,ip_src=UE_IP,ip_dst=SERVER1_IP,pktlen=20)
        exp_pkt = pkt_route(pkt, UPF_N6_MAC,UPLINK_N6_ROUTER_MAC)
        exp_pkt[IP].ttl -= 1

        pkt=pkt_gtpu_encap(pkt,UL_N6_TEID,UPF_N3_IP,RAN_N3_IP)
        testutils.send_packet(self, N3_PTF_PORT, pkt)
        testutils.verify_packet(self, exp_pkt, N6_PTF_PORT)
        testutils.verify_no_other_packets(self)

        # uplink Access(N3) -> Core(N9) test
        # send GTPU-encapped packet to N3, expect re-encapped packet from N9
        pkt=testutils.simple_udp_packet(eth_dst=UPF_N3_MAC,ip_src=UE_IP,ip_dst=SERVER2_IP,udp_dport=SERVER2_UDP_PORT,pktlen=20)
        exp_pkt=pkt.copy()
        exp_pkt[IP].ttl -= 1
        exp_pkt = pkt_gtpu_encap(exp_pkt,UL_N9_TEID,CORE_UPF_N9_IP,UPF_N9_IP)
        exp_pkt = pkt_route(exp_pkt, UPF_N9_MAC,UPLINK_N9_ROUTER_MAC)
        exp_pkt[IP].id=0x1513
        exp_pkt[UDP].chksum=0

        pkt=pkt_gtpu_encap(pkt,UL_N6_TEID,UPF_N3_IP,RAN_N3_IP)
        testutils.send_packet(self, N3_PTF_PORT, pkt)
        testutils.verify_packet(self, exp_pkt, N9_PTF_PORT)
        testutils.verify_no_other_packets(self)

        # downlink SgiLAN(N6) -> Access(N3) test
        # send IP packet to UE IP via N6, expect GTPU-encapped packet from N3
        pkt=testutils.simple_udp_packet(eth_dst=UPF_N6_MAC,ip_src=SERVER1_IP,ip_dst=UE_IP)
        exp_pkt=pkt.copy()
        exp_pkt[IP].ttl -= 1
        exp_pkt = pkt_gtpu_encap(exp_pkt,DL_TEID,RAN_N3_IP,UPF_N3_IP)
        exp_pkt = pkt_route(exp_pkt, UPF_N3_MAC,DOWNLINK_N3_ROUTER_MAC)
        exp_pkt[IP].id=0x1513
        exp_pkt[UDP].chksum=0

        testutils.send_packet(self, N6_PTF_PORT, pkt)
        testutils.verify_packet(self, exp_pkt, N3_PTF_PORT)
        testutils.verify_no_other_packets(self)
